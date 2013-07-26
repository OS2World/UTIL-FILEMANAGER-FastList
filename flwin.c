/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 Fast Win (For OS/2)

 Original release:
          Russ Weathersby
          August 1996
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
*/
/*------------------------------Includes-----------------------------------*/
#define INCL_WIN
#define INCL_GPI
#define INCL_DOS
#include <os2.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <process.h>
#include <limits.h>
#include "list.h"
#include "flwin.h"

/*-------------------------------Defines-----------------------------------*/

/*-------------------------Structs and Typedefs----------------------------*/
typedef struct {
  HWND   wnd;
  ULONG  msg;
  MPARAM mp1;
  MPARAM mp2;
} MSGPARMS;

/*--------------------------Function Declarations--------------------------*/
static MRESULT EXPENTRY WndProc( HWND, ULONG, MPARAM, MPARAM );

static MRESULT ActMsgCreate( MSGPARMS *, WNDATTR * );
static MRESULT ActMsgDestroy( MSGPARMS *, WNDATTR * );
static MRESULT ActMsgChar( MSGPARMS *, WNDATTR * );
static MRESULT ActMsgPaint( MSGPARMS *, WNDATTR * );
static MRESULT ActMsgSize( MSGPARMS *, WNDATTR * );
static MRESULT ActMsgButton1Click( MSGPARMS *, WNDATTR * );
static MRESULT ActMsgButton1DblClk( MSGPARMS *, WNDATTR * );
static MRESULT ActMsgBeginSelect( MSGPARMS *, WNDATTR * );
static MRESULT ActMsgMouseMove( MSGPARMS *, WNDATTR * );
static MRESULT ActMsgBeginDrag( MSGPARMS *, WNDATTR * );
static MRESULT ActMsgChord( MSGPARMS *, WNDATTR * );
static MRESULT ActMsgPresParamChanged( MSGPARMS *, WNDATTR * );
static MRESULT ActMsgVScroll( MSGPARMS *, WNDATTR * );
static MRESULT ActMsgUser1( MSGPARMS *, WNDATTR * );
static MRESULT ActMsgUser2( MSGPARMS *, WNDATTR * );
static MRESULT ActMsgUser3( MSGPARMS *, WNDATTR * );
static MRESULT ActMsgUser4( MSGPARMS *, WNDATTR * );

static void  InvalidateLines( WNDATTR *, int, int );

/*-------------------------------Globals-----------------------------------*/

/*-----------------------------Start of Code-------------------------------*/

/*---------------------------------------------------------------------------
                               FLCreateWindow
---------------------------------------------------------------------------*/
HWND EXTFUNC FLCreateWindow( CREATEOPS *userops )
{
 HWND  hwndClient,
       hwndFrame;

  if (userops == NULL)
     return NULLHANDLE;

  WinRegisterClass( userops->hab,
                    "FASTLIST",
                    WndProc,
                    CS_SIZEREDRAW,
                    sizeof( WNDATTR * ) );

  hwndFrame = WinCreateStdWindow( HWND_DESKTOP,
                                  0,
                                  &( userops->flags ),
                                  "FASTLIST",
                                  "FastList",
                                  0L,
                                  NULLHANDLE,
                                  ID_FLWIN_RES,
                                  &hwndClient );

  WinPostMsg( hwndClient, FLM_SETWINDOW, MPFROMP( userops ), 0 );

  return hwndFrame;
}

/*---------------------------------------------------------------------------
                                  WndProc
---------------------------------------------------------------------------*/
MRESULT EXPENTRY WndProc( HWND wnd, ULONG msg, MPARAM mp1, MPARAM mp2 )
{
  MSGPARMS  MsgParms;
  WNDATTR  *WndAttr;
  MRESULT   MRes;

   MsgParms.wnd = wnd;
   MsgParms.msg = msg;
   MsgParms.mp1 = mp1;
   MsgParms.mp2 = mp2;
   WndAttr = WinQueryWindowPtr( wnd, QWL_USER );

   switch (msg) {
     case WM_ERASEBACKGROUND:
           MRes = MRFROMSHORT( TRUE );
           break;
     case WM_CREATE:
           MRes = ActMsgCreate( &MsgParms, WndAttr );
           break;
     case WM_DESTROY :
           MRes = ActMsgDestroy( &MsgParms, WndAttr );
           break;
     case WM_CHAR:
           MRes = ActMsgChar( &MsgParms, WndAttr );
           break;
     case WM_BUTTON1CLICK:
           MRes = ActMsgButton1Click( &MsgParms, WndAttr );
           break;
     case WM_BUTTON1DBLCLK:
           MRes = ActMsgButton1DblClk( &MsgParms, WndAttr );
           break;
     case WM_BEGINSELECT:
           MRes = ActMsgBeginSelect( &MsgParms, WndAttr );
           break;
     case WM_ENDSELECT:
           WndAttr->select = NO_SELECT;
           MRes = MRFROMSHORT( FALSE );
           break;
     case WM_BEGINDRAG:
           MRes = ActMsgBeginDrag( &MsgParms, WndAttr );
           break;
     case WM_CHORD:
           MRes = ActMsgChord( &MsgParms, WndAttr );
           break;
     case WM_MOUSEMOVE :
           MRes = WinDefWindowProc( wnd, msg, mp1, mp2 );
           if (WndAttr->select != NO_SELECT)
              MRes = ActMsgMouseMove( &MsgParms, WndAttr );
           break;
     case WM_SIZE :
           MRes = ActMsgSize( &MsgParms, WndAttr );
           break;
     case WM_VSCROLL:
           MRes = ActMsgVScroll( &MsgParms, WndAttr );
           break;
     case WM_PRESPARAMCHANGED:
           MRes = ActMsgPresParamChanged( &MsgParms, WndAttr );
           break;
     case WM_PAINT:
           MRes = ActMsgPaint( &MsgParms, WndAttr );
           break;
     case FLM_SETWINDOW:
           MRes = ActMsgUser1( &MsgParms, WndAttr );
           break;
     case FLM_CORRECTWINDOW:
           MRes = ActMsgUser2( &MsgParms, WndAttr );
           break;
     case FLM_CLEAR:
           MRes = ActMsgUser3( &MsgParms, WndAttr );
           break;
     case FLM_CLEARMARKED:
           MRes = ActMsgUser4( &MsgParms, WndAttr );
           break;
     default:
           MRes = WinDefWindowProc( wnd, msg, mp1, mp2 );
           break;
   }

  return MRes;
}

/*-------------------------------------------------------------------------
                                ActMsgCreate
--------------------------------------------------------------------------*/
MRESULT ActMsgCreate( MSGPARMS *MsgParms, WNDATTR *WndAttr )
{
 WNDATTR  *attr;

   attr = ( WNDATTR * ) malloc( sizeof( WNDATTR ) );

   if (attr) {
      attr->list = LPListCreate();
      if (attr->list) {
         attr->current = attr->list;
         attr->Frame = WinQueryWindow( MsgParms->wnd, QW_PARENT );
         attr->Client = MsgParms->wnd;
         attr->Scroll = WinWindowFromID( attr->Frame, FID_VERTSCROLL );
         attr->prevscrollpos = attr->scrollpos = 0;
         attr->xpos = 0;
         attr->select = NO_SELECT;
         attr->escape = FALSE;

         WinSetWindowPtr( MsgParms->wnd, QWL_USER, attr );

         return MRFROMSHORT( FALSE );
      } else {
         free( attr );
      }
   }

   return MRFROMSHORT( TRUE );
}
/*-------------------------------------------------------------------------
                               ActMsgDestroy
--------------------------------------------------------------------------*/
MRESULT ActMsgDestroy( MSGPARMS *MsgParms, WNDATTR *WndAttr )
{
   if (WndAttr != ( WNDATTR * )NULL) {
      LPListDestroy( WndAttr->list );
      free( WndAttr );
   }

   return MRFROMSHORT( FALSE );
}
/*-------------------------------------------------------------------------
                                ActMsgChar
--------------------------------------------------------------------------*/
MRESULT ActMsgChar( MSGPARMS *MsgParms, WNDATTR *WndAttr )
{
 RECTL   rect;
 UCHAR   vkey;
 USHORT  flags;
 int     newx, dir, max;

   vkey = SHORT2FROMMP( MsgParms->mp2 );
   flags = SHORT1FROMMP( MsgParms->mp1 );
   if (flags & KC_KEYUP)
      return MRFROMSHORT( FALSE );

   switch (vkey) {
     case VK_PAGEUP: case VK_PAGEDOWN: case VK_UP: case VK_DOWN:
         WinPostMsg( WndAttr->Scroll, WM_CHAR,
                     MsgParms->mp1, MsgParms->mp2 );
         break;
     case VK_HOME:
         if (WndAttr->scrollpos) {
            WndAttr->scrollpos = 1;
            WinInvalidateRect( MsgParms->wnd, NULL, TRUE );
         }
         break;
     case VK_END:
         max = LPListLines( WndAttr->list ) - WndAttr->winlines + 1;
         WndAttr->scrollpos = max( max, 0 );
         WinInvalidateRect( MsgParms->wnd, NULL, TRUE );
         break;
     case VK_LEFT: case VK_RIGHT:
         newx = WndAttr->xpos;
         dir = (vkey == VK_LEFT) ? 1 : -1;
         WinQueryWindowRect( MsgParms->wnd, &rect );

         if (flags & KC_CTRL)
            newx += ( rect.xRight - rect.xLeft ) * dir;
         else
            newx += WndAttr->fontwidth * dir;

         newx = min( newx, 0 );
         max = LPListMaxLen( WndAttr->list ) * WndAttr->fontwidth;
         newx = max( newx, -max );

         WinScrollWindow( MsgParms->wnd,
                          newx - WndAttr->xpos,
                          0, NULL, NULL, NULLHANDLE, NULL,
                          SW_INVALIDATERGN );

         WndAttr->xpos = newx;
         break;
     case VK_ESC:
         WinPostMsg( MsgParms->wnd, WM_QUIT, 0, 0 );
         WndAttr->escape = TRUE;
         break;
     case VK_DELETE:
         if (LPListMarkedLines( WndAttr->list ))
            WinPostMsg( MsgParms->wnd, FLM_CLEARMARKED, 0, 0 );
         break;
     default:
         break;
   }

   return MRFROMSHORT( FALSE );
}
/*-------------------------------------------------------------------------
                             ActMsgButton1Click
--------------------------------------------------------------------------*/
MRESULT ActMsgButton1Click( MSGPARMS *MsgParms, WNDATTR *WndAttr )
{
 LPELEM *elem;
 USHORT  y, offset;

   y = SHORT2FROMMP( MsgParms->mp1 );
   y /= WndAttr->fontmax;
   offset = WndAttr->winlines - y - 1;

   elem = LPElemTrack( WndAttr->list, WndAttr->current, offset );

   if (elem != WndAttr->list) {
      if (elem->nextmk)
         LPElemUnmark( WndAttr->list, elem );
      else
         LPElemMark( WndAttr->list, elem );

      InvalidateLines( WndAttr, y, y );
   }

   return MRFROMSHORT( FALSE );
}
/*-------------------------------------------------------------------------
                           ActMsgButton1DblClk
--------------------------------------------------------------------------*/
MRESULT ActMsgButton1DblClk( MSGPARMS *MsgParms, WNDATTR *WndAttr )
{
 LPELEM *elem;
 USHORT  y, mark;

   y = SHORT2FROMMP( MsgParms->mp1 );
   y /= WndAttr->fontmax;
   InvalidateLines( WndAttr, y, 0 );

   y = WndAttr->winlines - y - 1;
   elem = LPElemTrack( WndAttr->list, WndAttr->current, y );

   mark = (elem->nextmk != NULL);
   elem = elem->next;

   while (elem != WndAttr->list) {
      if (mark ^ (elem->nextmk == NULL))
         break;
      if (mark)
         LPElemMark( WndAttr->list, elem );
      else
         LPElemUnmark( WndAttr->list, elem );
      elem = elem->next;
   }

   return MRFROMSHORT( FALSE );
}
/*-------------------------------------------------------------------------
                             ActMsgBeginSelect
--------------------------------------------------------------------------*/
MRESULT ActMsgBeginSelect( MSGPARMS *MsgParms, WNDATTR *WndAttr )
{
 USHORT y;

   y = SHORT2FROMMP( MsgParms->mp1 );
   WndAttr->select = y / WndAttr->fontmax;

   return ActMsgButton1Click( MsgParms, WndAttr );
}
/*-------------------------------------------------------------------------
                             ActMsgMouseMove
--------------------------------------------------------------------------*/
MRESULT ActMsgMouseMove( MSGPARMS *MsgParms, WNDATTR *WndAttr )
{
 USHORT  y, select;

   y = SHORT2FROMMP( MsgParms->mp1 );
   select = y / WndAttr->fontmax;

   if (!(WinGetKeyState( HWND_DESKTOP, VK_BUTTON1 ) & 0x8000))
      select = WndAttr->select = NO_SELECT;

   if (select == WndAttr->select)
      return MRFROMSHORT( FALSE );

   WndAttr->select = select;

   return ActMsgButton1Click( MsgParms, WndAttr );
}

/*-------------------------------------------------------------------------
                                ActMsgChord
--------------------------------------------------------------------------*/
MRESULT ActMsgChord( MSGPARMS *MsgParms, WNDATTR *WndAttr )
{
 SWP  swp;
 LONG keystate;

   WinQueryWindowPos( WndAttr->Frame, &swp );

   keystate = WinGetKeyState( HWND_DESKTOP, VK_ALT );
   keystate |= WinGetKeyState( HWND_DESKTOP, VK_CTRL );

   if (keystate & 0x8000) {
      swp.fl = SWP_HIDE;
   } else {
      if (swp.fl & SWP_MAXIMIZE)
         swp.fl = SWP_RESTORE;
      else
         swp.fl = SWP_MAXIMIZE;
   }

   WinSetWindowPos( WndAttr->Frame, NULLHANDLE, 0, 0, 0, 0, swp.fl );

   return MRFROMSHORT( FALSE );
}
/*-------------------------------------------------------------------------
                            ActMsgBeginDrag
--------------------------------------------------------------------------*/
MRESULT ActMsgBeginDrag( MSGPARMS *MsgParms, WNDATTR *WndAttr )
{
 TRACKINFO trinf;
 SWP       swp;
 RECTL     rect;
 LONG      value;

   WinQueryWindowPos( WndAttr->Frame, &swp );
   trinf.rclTrack.xLeft = swp.x;
   trinf.rclTrack.xRight = swp.x + swp.cx;
   trinf.rclTrack.yBottom = swp.y;
   trinf.rclTrack.yTop = swp.y + swp.cy;
   trinf.ptlMaxTrackSize.x = trinf.ptlMinTrackSize.x = swp.cx;
   trinf.ptlMaxTrackSize.y = trinf.ptlMinTrackSize.y = swp.cy;

   value = WinQuerySysValue( HWND_DESKTOP, SV_CXSIZEBORDER );
   trinf.cxBorder = trinf.cyBorder = value;
   trinf.cxGrid = trinf.cyGrid = 0;
   trinf.cxKeyboard = trinf.cyKeyboard = 8;

   WinQueryWindowRect( HWND_DESKTOP, &rect );
   memcpy( &trinf.rclBoundary, &rect, sizeof( RECTL ) );

   trinf.fs = TF_MOVE;

   WinTrackRect( HWND_DESKTOP, NULLHANDLE, &trinf );

   WinSetWindowPos( WndAttr->Frame, NULLHANDLE,
                    trinf.rclTrack.xLeft,
                    trinf.rclTrack.yBottom,
                    0, 0,
                    SWP_MOVE );

   return MRFROMSHORT( FALSE );
}
/*-------------------------------------------------------------------------
                                 ActMsgSize
--------------------------------------------------------------------------*/
MRESULT ActMsgSize( MSGPARMS *MsgParms, WNDATTR *WndAttr )
{
 void   CorrectWinSize( HWND, WNDATTR * );
 RECTL  rect;
 USHORT bottomline, listlines;
 int    maxscrollpos;

   CorrectWinSize( MsgParms->wnd, WndAttr );

   WinQueryWindowRect( MsgParms->wnd, &rect );
   WndAttr->winlines = rect.yTop / WndAttr->fontmax;

   listlines = LPListLines( WndAttr->list );

   bottomline = WndAttr->scrollpos + WndAttr->winlines - 1;
   maxscrollpos = listlines - WndAttr->winlines + 1;

   if (bottomline > listlines && maxscrollpos > 1)
      WndAttr->scrollpos = maxscrollpos;
   if (listlines < WndAttr->winlines && listlines)
      WndAttr->scrollpos = 1;

   return MRFROMSHORT( FALSE );
}
/*-------------------------------------------------------------------------
                               CorrectWinSize
--------------------------------------------------------------------------*/
void CorrectWinSize( HWND wnd, WNDATTR *WndAttr )
{
 HPS         hps;
 SWP         swp;
 FONTMETRICS fm;
 int         grain;

   hps = WinGetPS( wnd );
   GpiQueryFontMetrics( hps, sizeof( FONTMETRICS ), &fm );
   WndAttr->fontmax = fm.lMaxBaselineExt;
   WndAttr->fontbase = fm.lMaxAscender;
   WndAttr->fontwidth = fm.lAveCharWidth;

   WinQueryWindowPos( wnd, &swp );
   grain = swp.cy % WndAttr->fontmax;
   if (grain) {
      WinQueryWindowPos( WndAttr->Frame, &swp );
      swp.y += grain;
      swp.cy -= grain;
      swp.fl |= SWP_MOVE;
      WinSetWindowPos( WndAttr->Frame,
                       swp.hwndInsertBehind,
                       swp.x, swp.y,
                       swp.cx, swp.cy,
                       swp.fl );
   }
   return;
}

/*-------------------------------------------------------------------------
                              ActMsgVScroll
--------------------------------------------------------------------------*/
MRESULT ActMsgVScroll( MSGPARMS *MsgParms, WNDATTR *WndAttr )
{
 MRESULT mres;
 int     newpos, minpos, maxpos, delta;

   newpos = WndAttr->scrollpos;

   switch (SHORT2FROMMP( MsgParms->mp2 )) {
       case  SB_LINEUP :
           newpos -= 1;
           break;
       case  SB_LINEDOWN :
           newpos += 1;
           break;
       case  SB_SLIDERTRACK:
           newpos = SHORT1FROMMP( MsgParms->mp2 );
           break;
       case  SB_PAGEUP :
           newpos -= WndAttr->winlines;
           break;
       case  SB_PAGEDOWN :
           newpos += WndAttr->winlines;
           break;
       default :
           break;
   }

   mres = WinSendMsg( WndAttr->Scroll, SBM_QUERYRANGE, 0, 0 );
   minpos = SHORT1FROMMR( mres );
   maxpos = SHORT2FROMMR( mres );
   newpos = max( newpos, minpos );
   newpos = min( newpos, maxpos );
   delta  = newpos - WndAttr->scrollpos;

   if (delta) {
      WndAttr->scrollpos = newpos;
      WinScrollWindow( MsgParms->wnd, 0,
                       delta * WndAttr->fontmax,
                       NULL, NULL, NULLHANDLE, NULL,
                       SW_INVALIDATERGN );
   }

   return MRFROMSHORT( FALSE );
}

/*-------------------------------------------------------------------------
                          ActMsgPresParamChanged
--------------------------------------------------------------------------*/
MRESULT ActMsgPresParamChanged( MSGPARMS *MsgParms, WNDATTR *WndAttr )
{
 SWP   swp;

   if (LONGFROMMP( MsgParms->mp1 ) == PP_FONTNAMESIZE) {
      WinQueryWindowPos( MsgParms->wnd, &swp );
      WinPostMsg( MsgParms->wnd, WM_SIZE,
                  MPFROM2SHORT( swp.cx, swp.cx ),
                  MPFROM2SHORT( swp.cy, swp.cy ) );
   }
   WinInvalidateRect( MsgParms->wnd, NULL, TRUE );

   WinQueryPresParam( MsgParms->wnd,
                      PP_FONTNAMESIZE,
                      0,
                      NULL,
                      sizeof WndAttr->font,
                      WndAttr->font,
                      0 );

   return MRFROMSHORT( FALSE );
}
/*-------------------------------------------------------------------------
                              ActMsgUser1

Purpose: Called after WM_CREATE to initialize various window
         parameters.
--------------------------------------------------------------------------*/
MRESULT ActMsgUser1( MSGPARMS *MsgParms, WNDATTR *WndAttr )
{
 CREATEOPS *create;
 ULONG      flags;

   create = PVOIDFROMMP( MsgParms->mp1 );

   WndAttr->hab = create->hab;

   LPListMaxLines( WndAttr->list ) = create->maxlines;
   LPListMaxLen( WndAttr->list ) = create->maxlen;

   flags = SWP_SHOW | SWP_SIZE;

   if (!( create->flags & FCF_SHELLPOSITION ))
      flags |= SWP_MOVE;
   if (create->maximize)
      flags |= SWP_MAXIMIZE;

   WinSetPresParam( MsgParms->wnd,
                    PP_FONTNAMESIZE,
                    sizeof create->font,
                    create->font );

   WinSetWindowPos( WndAttr->Frame, NULLHANDLE,
                    create->xpos, create->ypos,
                    create->winwidth, create->winheight,
                    flags );

   return MRFROMSHORT( FALSE );
}

/*-------------------------------------------------------------------------
                              ActMsgUser2

Purpose: Scroll window and reset scrollbar (if necessary)
Context: Called after lines appended.
--------------------------------------------------------------------------*/
MRESULT ActMsgUser2( MSGPARMS *MsgParms, WNDATTR *WndAttr )
{
 USHORT oldpos, newpos, delta, listlines, maxlines, lines_added;

   listlines = LPListLines( WndAttr->list );
   maxlines = LPListMaxLines( WndAttr->list );
   lines_added = SHORT1FROMMP( MsgParms->mp1 );

   if (listlines > WndAttr->winlines && WndAttr->scrollpos != 0) {
      oldpos = WndAttr->scrollpos;
      newpos = listlines - WndAttr->winlines + 1;
      WndAttr->scrollpos = WndAttr->prevscrollpos = newpos;
      WndAttr->current = LPElemTrack( WndAttr->list, WndAttr->list, newpos );

      delta = newpos - oldpos;
      if (delta == 0 && listlines == maxlines)
         delta = lines_added;

      WinScrollWindow( MsgParms->wnd, 0,
                       delta * WndAttr->fontmax,
                       NULL, NULL, NULLHANDLE, NULL,
                       SW_INVALIDATERGN );

      if ((listlines - lines_added) < WndAttr->winlines)
         InvalidateLines( WndAttr, lines_added, 0 );

   } else if (WndAttr->scrollpos != 0) {
      InvalidateLines( WndAttr,
                       WndAttr->winlines - listlines + lines_added,
                       WndAttr->winlines - listlines);
   } else {
      WndAttr->scrollpos = WndAttr->prevscrollpos = 1;
      WndAttr->current = WndAttr->list->next;
      WinInvalidateRect( WndAttr->Client, NULL, TRUE );
   }

   return MRFROMSHORT( FALSE );
}
/*-------------------------------------------------------------------------
                              ActMsgUser3
Clear window
--------------------------------------------------------------------------*/
MRESULT ActMsgUser3( MSGPARMS *MsgParms, WNDATTR *WndAttr )
{
   LPListClear( WndAttr->list );
   WndAttr->current = WndAttr->list;
   WndAttr->scrollpos = WndAttr->prevscrollpos = 0;
   WndAttr->xpos = 0;
   WinInvalidateRect( WndAttr->Client, NULL, TRUE );

  return MRFROMSHORT( FALSE );
}
/*-------------------------------------------------------------------------
                              ActMsgUser4
Clear Marked Lines and correct window
--------------------------------------------------------------------------*/
MRESULT ActMsgUser4( MSGPARMS *MsgParms, WNDATTR *WndAttr )
{
 int lines, newscrollpos, maxpos;
 LPELEM *elem;

   newscrollpos = 0;
   elem = WndAttr->list;
   do {
      elem = elem->next;
      if (elem->nextmk == NULL)
         newscrollpos++;
   } while (elem != WndAttr->current);

   LPListClearMarked( WndAttr->list );

   lines = LPListLines( WndAttr->list );

   maxpos = lines - WndAttr->winlines + 1;
   newscrollpos = min( newscrollpos, maxpos );

   if (newscrollpos > 0) {
      WndAttr->scrollpos = newscrollpos;
      WndAttr->prevscrollpos = 1;
      WndAttr->current = WndAttr->list->next;
   } else if (lines) {
      WndAttr->current = WndAttr->list->next;
      WndAttr->scrollpos = WndAttr->prevscrollpos = 1;
   } else {
      WndAttr->current = WndAttr->list;
      WndAttr->scrollpos = WndAttr->prevscrollpos = 0;
   }

   WinInvalidateRect( MsgParms->wnd, NULL, TRUE );

  return MRFROMSHORT( FALSE );
}
/*-------------------------------------------------------------------------
                              ActMsgPaint
--------------------------------------------------------------------------*/
MRESULT ActMsgPaint( MSGPARMS *MsgParms, WNDATTR *WndAttr )
{
 LPELEM  *elem;
 HPS      hps;
 RECTL    rect;
 POINTL   pos;
 USHORT   range, listlines, invalidoffset;
 UINT     fontsize;
 long     winsize;
 int      shift;

   hps = WinBeginPaint( MsgParms->wnd,
                        NULLHANDLE,
                        &rect );
   GpiErase( hps );

   shift = WndAttr->scrollpos - WndAttr->prevscrollpos;

   WndAttr->current = LPElemRotate( WndAttr->list,
                                    WndAttr->current,
                                    shift );

   WndAttr->prevscrollpos = WndAttr->scrollpos;

   fontsize = WndAttr->fontmax;

   range = rect.yTop - rect.yBottom + fontsize - 1;
   range /= fontsize;

   winsize = WndAttr->winlines * fontsize;

   invalidoffset = (winsize - rect.yTop) / fontsize;

   elem = LPElemTrack( WndAttr->list,
                       WndAttr->current,
                       invalidoffset );

   pos.y = winsize;
   pos.y -= invalidoffset * fontsize;
   pos.y -= WndAttr->fontbase;

 /* Left margin */
   pos.x = WndAttr->xpos;

   while (range-- && elem != WndAttr->list) {
      if (elem->nextmk) {
         rect.yTop = pos.y + WndAttr->fontbase;
         rect.yBottom = rect.yTop - fontsize;
         WinFillRect( hps, &rect, CLR_YELLOW );
      }
      GpiCharStringAt( hps, &pos, strlen( elem->data), elem->data );
      elem = elem->next;
      pos.y -= fontsize;
   }

   WinEndPaint( hps );

   listlines = LPListLines( WndAttr->list );
   range = listlines - WndAttr->winlines + 1;

  /* Post messages to scroll bar in this order so that the buttons */
  /* are updated properly.                                         */
   WinPostMsg( WndAttr->Scroll, SBM_SETSCROLLBAR,
               MPFROMSHORT( WndAttr->scrollpos ),
               MPFROM2SHORT( 1, range ) );
   WinPostMsg( WndAttr->Scroll, SBM_SETTHUMBSIZE,
               MPFROM2SHORT( WndAttr->winlines, listlines ), 0 );

   return MRFROMSHORT( FALSE );
}

/*-------------------------------------------------------------------------
                            InvalidateLines
--------------------------------------------------------------------------*/
void InvalidateLines( WNDATTR *wndattr, int top, int bottom )
{
 RECTL rect;

   WinQueryWindowRect( wndattr->Client, &rect );

   top = min( top, wndattr->winlines - 1 );
   bottom = max( 0, bottom );
   rect.yBottom = bottom * wndattr->fontmax;
   rect.yTop = (top + 1) * wndattr->fontmax;

   WinInvalidateRect( wndattr->Client, &rect, TRUE );
   return;
}
