/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 menu.c

 'Fast List' menu and command controls
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
*/
#define INCL_PM
#define INCL_BASE
#include <os2.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <process.h>
#include <ctype.h>
#include "list.h"
#include "flwin.h"
#include "menu.h"
#include "res.h"
#include "util.h"

/*--------------------------------Macros----------------------------------*/
#define FNEXT 0
#define FPREV 1

/*-------------------------Structs and Typedefs----------------------------*/

/*------------------------Function Declarations---------------------------*/
static void CmdAppendFile( WNDATTR * );
static void CmdSaveAs( WNDATTR * );
static void CmdCopy( WNDATTR * );
static void CmdPaste( WNDATTR * );
static void CmdUnmark( WNDATTR * );
static void CmdAccum( WNDATTR * );
static void CmdFind( WNDATTR * );
static void CmdMarkFind( WNDATTR * );
static void CmdMate( WNDATTR * );
static void CmdSpawn( WNDATTR * );
static void CmdQuit( WNDATTR * );

static MRESULT EXPENTRY FindWndProc( HWND, ULONG, MPARAM, MPARAM );
static MRESULT EXPENTRY AboutWndProc( HWND, ULONG, MPARAM, MPARAM );
static MRESULT EXPENTRY MateWndProc( HWND, ULONG, MPARAM, MPARAM );

static int  FindIt( LPLIST *, LPELEM*, int );
static void MoveToMark( WNDATTR*, int );

static void SetAttr( USHORT, USHORT, BOOL );

/*------------------------------Globals-----------------------------------*/
static HWND   Popup;

static struct {
   char text[40];
   int  sense;
}FindDat;

static PFNWP  OldWndProc;

/* These are shared with flist.c */
HWND MateScrollWnd;
FILEDLG FileDlg;

extern char **_argv;

/*-------------------------------------------------------------------------
                              CreatePopup
--------------------------------------------------------------------------*/
void CreatePopup( HAB hab, HWND client, HWND frame )
{
 HACCEL haccel;

   if (Popup != NULLHANDLE)
      DestroyPopup();

   Popup = WinLoadMenu( client,
                        NULL,
                        ID_POPUP );

   haccel = WinLoadAccelTable( hab, NULLHANDLE, ID_ACCEL );
   WinSetAccelTable( hab, haccel, frame );

   return;
}
/*-------------------------------------------------------------------------
                              DestroyPopup
--------------------------------------------------------------------------*/
void DestroyPopup()
{
  WinDestroyWindow( Popup );
  Popup = NULLHANDLE;
  return;
}
/*-------------------------------------------------------------------------
                            ActMsgContextMenu
--------------------------------------------------------------------------*/
MRESULT ActMsgContextMenu( WNDATTR *wndattr, USHORT x, USHORT y )
{
 BOOL state;

   if (Popup == NULLHANDLE)
      return MRFROMSHORT( TRUE );

   if (MateScrollWnd != NULLHANDLE) {
      if (WinIsWindow( wndattr->hab, MateScrollWnd ) == FALSE)
         MateScrollWnd = NULLHANDLE;
   }

   state = (LPListMarkedLines( wndattr->list ) == 0);
   SetAttr( CMD_COPY, MIA_DISABLED, state );
   SetAttr( CMD_UNMARK, MIA_DISABLED, state );
   SetAttr( CMD_REARRANGE, MIA_DISABLED, state );

   WinOpenClipbrd( wndattr->hab );
   state = (WinQueryClipbrdData( wndattr->hab, CF_TEXT ) == 0);
   WinCloseClipbrd( wndattr->hab );
   SetAttr( CMD_PASTE, MIA_DISABLED, state );

   state = (WinQueryClipbrdViewer( wndattr->hab ) == wndattr->Client);
   SetAttr( CMD_ACCUM, MIA_CHECKED, state );

   state = (MateScrollWnd != NULLHANDLE);
   SetAttr( CMD_MATE, MIA_CHECKED, state );

   WinPopupMenu( wndattr->Frame,
                 wndattr->Frame,
                 Popup,
                 x, y, CMD_ABOUT,
                 PU_HCONSTRAIN | PU_VCONSTRAIN | PU_POSITIONONITEM |
                 PU_KEYBOARD | PU_MOUSEBUTTON1 | PU_MOUSEBUTTON2 );

   return MRFROMSHORT( FALSE );
}

/*-------------------------------------------------------------------------
                              ActMsgCommand
--------------------------------------------------------------------------*/
MRESULT ActMsgCommand( WNDATTR *wndattr, MPARAM mp1, MPARAM mp2 )
{
 USHORT cmd;
 int    offset;

   cmd = SHORT1FROMMP( mp1 );

   switch (cmd) {
      case CMD_APPEND:
         CmdAppendFile( wndattr );
         break;
      case CMD_SAVE:
         CmdSaveAs( wndattr );
         break;
      case CMD_COPY:
         CmdCopy( wndattr );
         break;
      case CMD_PASTE:
         CmdPaste( wndattr );
         break;
      case CMD_UNMARK:
         CmdUnmark( wndattr );
         break;
      case CMD_REARRANGE:
         CmdRearrange( wndattr );
         break;
      case CMD_ABOUT:
         CmdAbout( wndattr );
         break;
      case CMD_ACCUM:
         CmdAccum( wndattr );
         break;
      case CMD_FIND:
         CmdFind( wndattr );
         break;
      case CMD_MARKFIND:
         CmdMarkFind( wndattr );
         break;
      case CMD_FINDNEXT:
         offset = FindIt( wndattr->list, wndattr->current, FNEXT );
         CorrectScroll( wndattr, offset );
         break;
      case CMD_FINDPREV:
         offset = FindIt( wndattr->list, wndattr->current, FPREV );
         CorrectScroll( wndattr, offset );
         break;
      case CMD_NEXTMARK:
         MoveToMark( wndattr, FNEXT );
         break;
      case CMD_PREVMARK:
         MoveToMark( wndattr, FPREV );
         break;
      case CMD_MATE:
         CmdMate( wndattr );
         break;
      case CMD_SPAWN:
         CmdSpawn( wndattr );
         break;
      case CMD_QUIT:
         CmdQuit( wndattr );
         break;
      default:
         return MRFROMSHORT( TRUE );
         break;
   }

   return MRFROMSHORT( FALSE );
}

/*---------------------------------------------------------------------------
                                  SetAttr
---------------------------------------------------------------------------*/
void SetAttr( USHORT id, USHORT attr, BOOL value )
{
 USHORT setval;

   setval = value ? attr : 0;

   WinSendMsg( Popup, MM_SETITEMATTR,
               MPFROM2SHORT( id, TRUE ),
               MPFROM2SHORT( attr, setval ) );

   return;
}

/*---------------------------------------------------------------------------
                               CmdAppendFile
---------------------------------------------------------------------------*/
void CmdAppendFile( WNDATTR *wndattr )
{
 char *filename;


   FileDlg.cbSize = sizeof( FILEDLG );
   FileDlg.fl = FDS_CENTER | FDS_OPEN_DIALOG;

   WinFileDlg( HWND_DESKTOP, wndattr->Client, &FileDlg );

   filename = FileDlg.szFullFile;
   if (FileDlg.lReturn == DID_OK) {
      if (*filename != '\0') {
         AppendFile( wndattr, filename, TRUE, NULLHANDLE );
      }
   }

   return;
}

/*---------------------------------------------------------------------------
                                 CmdSaveAs
---------------------------------------------------------------------------*/
void CmdSaveAs( WNDATTR *wndattr )
{
 char *filename;

   FileDlg.cbSize = sizeof( FILEDLG );
   FileDlg.fl = FDS_CENTER | FDS_SAVEAS_DIALOG | FDS_ENABLEFILELB;

   WinFileDlg( HWND_DESKTOP, wndattr->Client, &FileDlg );

   filename = &( FileDlg.szFullFile );
   if (*filename == '\0' || FileDlg.lReturn != DID_OK)
      return;

   SaveList( filename, wndattr->list, FALSE, TRUE );

   return;
}
/*---------------------------------------------------------------------------
                                  CmdCopy
---------------------------------------------------------------------------*/
void CmdCopy( WNDATTR *wndattr )
{
 LPELEM *elem;
 ULONG   length;
 void   *text;

   length = 0;
   elem = wndattr->list->nextmk;
   while (elem != wndattr->list) {
      length += strlen( elem->data );
      elem = elem->nextmk;
   }
   if (length == 0)
      return;

   DosAllocSharedMem( &text, NULL, length,
                      PAG_COMMIT | OBJ_GIVEABLE | PAG_WRITE );
   elem = wndattr->list->next;
   while (elem != wndattr->list) {
      if (elem->nextmk != NULL) {
         strcat( text, elem->data );
         strcat( text, "\r\n" );
      };
      elem = elem->next;
   }

   WinOpenClipbrd( wndattr->hab );
   WinSetClipbrdData( wndattr->hab, ( ULONG )text, CF_TEXT, CFI_POINTER );
   WinCloseClipbrd( wndattr->hab );

   return;
}
/*---------------------------------------------------------------------------
                                  CmdPaste
---------------------------------------------------------------------------*/
void CmdPaste( WNDATTR *wndattr )
{
   PasteFromClipboard( wndattr->hab, wndattr->Client );

   return;
}

/*---------------------------------------------------------------------------
                                  CmdUnmark
---------------------------------------------------------------------------*/
void CmdUnmark( WNDATTR *wndattr )
{
 LPLIST *list;

  list = wndattr->list;
  while (list->nextmk != list) {
     LPElemUnmark( list, list->nextmk );
  }

  WinInvalidateRect( wndattr->Client, NULL, TRUE );

  return;
}
/*---------------------------------------------------------------------------
                                CmdRearrange
---------------------------------------------------------------------------*/
void CmdRearrange( WNDATTR *wndattr )
{
 LPELEM *elem, *mark;

  elem = wndattr->list->next;
  mark = wndattr->list->nextmk;

  while (elem != wndattr->list) {
     if (elem->nextmk != NULL) {
        LPElemExchange( wndattr->list, elem, mark );
        elem = mark;
        mark = mark->nextmk;
     }
     elem = elem->next;
  }

  wndattr->current = LPElemTrack( wndattr->list,
                                  wndattr->list,
                                  wndattr->scrollpos );

  WinInvalidateRect( wndattr->Client, NULL, TRUE );

  return;
}
/*---------------------------------------------------------------------------
                                  CmdAbout
---------------------------------------------------------------------------*/
void CmdAbout( WNDATTR *wndattr )
{
     WinDlgBox( HWND_DESKTOP,
                wndattr->Client,
                AboutWndProc,
                NULLHANDLE,
                ID_ABOUTDLG,
                NULL );
 return;
}
/*---------------------------------------------------------------------------
                                AboutWndProc
---------------------------------------------------------------------------*/
MRESULT EXPENTRY AboutWndProc( HWND wnd, ULONG msg, MPARAM mp1, MPARAM mp2 )
{
 WNDATTR *wndattr;
 MRESULT  MRes;
 HWND     frame, client;
 char     number[10];

   switch (msg) {
     case WM_INITDLG:
           PositionDialog( wnd );

           frame = WinQueryWindow( wnd, QW_OWNER );
           client = WinWindowFromID( frame, FID_CLIENT );
           wndattr = (WNDATTR*)WinQueryWindowPtr( client, QWL_USER );

           ltoa( LPListLines( wndattr->list ), number, 10 );
           WinSetDlgItemText( wnd, ID_LINES, number );
           ltoa( LPListMarkedLines( wndattr->list ), number, 10 );
           WinSetDlgItemText( wnd, ID_MARKED, number );
           ltoa( LPListMaxLines( wndattr->list ), number, 10 );
           WinSetDlgItemText( wnd, ID_MAXLINES, number );
           ltoa( LPListMaxLen( wndattr->list ), number, 10 );
           WinSetDlgItemText( wnd, ID_MAXLENGTH, number );

           break;
     default:
        MRes = WinDefDlgProc( wnd, msg, mp1, mp2 );
        break;
   }

  return MRes;
}

/*---------------------------------------------------------------------------
                                  CmdAccum
---------------------------------------------------------------------------*/
void CmdAccum( WNDATTR *wndattr )
{
 MRESULT mres;
 HWND    wnd;
 USHORT  state;

   mres = WinSendMsg( Popup,
                      MM_QUERYITEMATTR,
                      MPFROMSHORT( CMD_ACCUM ),
                      MPFROMSHORT( MIA_CHECKED ) );

   state = SHORT1FROMMR( mres );

   if (state != MIA_CHECKED)
      wnd = wndattr->Client;
   else
      wnd = NULLHANDLE;

   WinOpenClipbrd( wndattr->hab );
   WinSetClipbrdViewer( wndattr->hab, wnd );
   WinCloseClipbrd( wndattr->hab );

   return;
}

/*---------------------------------------------------------------------------
                                  CmdFind
---------------------------------------------------------------------------*/
void CmdFind( WNDATTR *wndattr )
{
 int offset;

    WinDlgBox( HWND_DESKTOP,
               wndattr->Client,
               FindWndProc,
               NULLHANDLE,
               ID_FINDDLG,
               NULL );

    if (strlen( FindDat.text )) {
        offset = FindIt( wndattr->list, wndattr->current, FNEXT );
        CorrectScroll( wndattr, offset );
    }

 return;
}
/*---------------------------------------------------------------------------
                               CmdMarkFind
---------------------------------------------------------------------------*/
void CmdMarkFind( WNDATTR *wndattr )
{
 LPELEM *current;
 int     offset;

    WinDlgBox( HWND_DESKTOP,
               wndattr->Client,
               FindWndProc,
               NULLHANDLE,
               ID_FINDDLG,
               NULL );

    if (strlen( FindDat.text )) {
        current = wndattr->list;
        offset = FindIt( wndattr->list, current, FNEXT );
        while (offset) {
            current = LPElemTrack( wndattr->list, current, offset );
            if (current != wndattr->list)
               LPElemMark( wndattr->list, current );
            offset = FindIt( wndattr->list, current, FNEXT );
        }
        WinInvalidateRect( wndattr->Client, NULL, TRUE );
    }

 return;
}
/*---------------------------------------------------------------------------
                                FindWndProc
---------------------------------------------------------------------------*/
MRESULT EXPENTRY FindWndProc( HWND wnd, ULONG msg, MPARAM mp1, MPARAM mp2 )
{
 MRESULT  MRes;
 HWND     entryfield;

   switch (msg) {
     case WM_INITDLG:
           PositionDialog( wnd );
           entryfield = WinWindowFromID( wnd, ID_FINDENTRY );
           WinSendMsg( entryfield, EM_SETTEXTLIMIT,
                       MPFROMSHORT( sizeof FindDat.text - 1 ), 0 );
           WinSetDlgItemText( wnd, ID_FINDENTRY, FindDat.text );
           WinSendMsg( entryfield, EM_SETSEL,
                       MPFROM2SHORT( 0, sizeof FindDat.text ), 0 );
           MRes = MRFROMSHORT( FALSE );
           break;
     case WM_CHAR:
        if (SHORT2FROMMP( mp2 ) == VK_NEWLINE) {
           WinQueryDlgItemText( wnd,
                                ID_FINDENTRY,
                                sizeof FindDat.text - 1,
                                FindDat.text );
           FindDat.sense = WinQueryButtonCheckstate( wnd, ID_FINDCHECK );
           WinDismissDlg( wnd, 0 );
        }
     default:
        MRes = WinDefDlgProc( wnd, msg, mp1, mp2 );
        break;
   }

  return MRes;
}

/*---------------------------------------------------------------------------
                                  FindIt
---------------------------------------------------------------------------*/
int FindIt( LPLIST *list, LPELEM *current, int direction )
{
 LPELEM *elem;
 int     delta;
 char    cmp[3];
 int     len;
 char   *ptr;

  len = strlen( FindDat.text );
  if (len == 0)
     return 0;

  if (direction == FNEXT)
     elem = current->next;
  else
     elem = current->prev;

  delta = 0;

  if (FindDat.sense) {
     while (elem != list) {
        delta += (direction == FNEXT) ? 1 : -1;
        if (strstr( elem->data, FindDat.text ))
           return delta;
        elem = (direction == FNEXT) ? elem->next : elem->prev;
     }
  } else {
     cmp[0] = toupper( FindDat.text[0] );
     cmp[1] = tolower( FindDat.text[0] );
     cmp[2] = '\0';
     while (elem != list) {
        delta += (direction == FNEXT) ? 1 : -1;
        ptr = elem->data;
        while (ptr = strpbrk( ptr, cmp )) {
           if (memicmp( FindDat.text, ptr, len ) == 0)
              return delta;
           ++ptr;
        }
        elem = (direction == FNEXT) ? elem->next : elem->prev;
     }
  }
  return 0;
}

/*---------------------------------------------------------------------------
                                MoveToMark
---------------------------------------------------------------------------*/
void MoveToMark( WNDATTR *wndattr, int direction )
{
 LPELEM *elem;
 int scrollpos;

  elem = wndattr->current;
  scrollpos = 0;

  if (direction == FNEXT) {
     while (elem->nextmk && elem != wndattr->list) {
        elem = elem->next;
        scrollpos++;
     }
     while (elem->nextmk == NULL) {
        elem = elem->next;
        scrollpos++;
     }
  } else {
     do {
        elem = elem->prev;
        scrollpos--;
     } while (elem->nextmk == NULL);

     while (elem->nextmk && elem != wndattr->list) {
        elem = elem->prev;
        scrollpos--;
     }
     if (elem->next->nextmk) {
        scrollpos++;
        elem = elem->next;
     }
  }

  if (elem != wndattr->list)
     CorrectScroll( wndattr, scrollpos );

  return;
}
/*---------------------------------------------------------------------------
                                  CmdMate
---------------------------------------------------------------------------*/
void CmdMate( WNDATTR *wndattr )
{
  if (MateScrollWnd != NULLHANDLE)
     MateScrollWnd = NULLHANDLE;
  else
     OldWndProc = WinSubclassWindow( wndattr->Client, MateWndProc );

  return;
}

/*---------------------------------------------------------------------------
                                MateWndProc
---------------------------------------------------------------------------*/
MRESULT EXPENTRY MateWndProc( HWND wnd, ULONG msg, MPARAM mp1, MPARAM mp2 )
{
 WNDATTR *wndattr;

   switch (msg) {
     case WM_FOCUSCHANGE:
        if (SHORT1FROMMP( mp2 ) == FALSE) {
           wndattr = (WNDATTR*)WinQueryWindowPtr( wnd, QWL_USER );
           MateScrollWnd = HWNDFROMMP( mp1 );
           WinSubclassWindow( wndattr->Client, OldWndProc );
        }
        break;
     default:
        break;
   }

  return OldWndProc( wnd, msg, mp1, mp2 );
}

/*---------------------------------------------------------------------------
                                  CmdSpawn
---------------------------------------------------------------------------*/
void CmdSpawn( WNDATTR *wndattr )
{
  char args[CCHMAXPATHCOMP + 4];
  int len;
  RESULTCODES result;

   len = strlen( _argv[0] ) + 1;
   memcpy( args, _argv[0], len );
   memcpy( args + len, "-i\0", 4 );

   DosExecPgm( NULL, 0, EXEC_ASYNC,
               args, NULL, &result, _argv[0] );

   return;
}

/*---------------------------------------------------------------------------
                                  CmdQuit
---------------------------------------------------------------------------*/
void CmdQuit( WNDATTR *wndattr )
{
   WinPostMsg( wndattr->Client, WM_QUIT, 0, 0 );
   return;
}
