/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 util.c

 useful routines
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
*/
#define INCL_PM
#define INCL_DOS
#include <os2.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <process.h>
#include "list.h"
#include "flwin.h"
#include "util.h"

/*--------------------------------Macros----------------------------------*/

/*-------------------------Structs and Typedefs----------------------------*/

typedef struct {
   FILE   *fp;
   LPLIST *list;
   BOOL    marked;
} SAVEARGS;

typedef struct {
  HWND    wnd;
  HEV     hev;
  LPLIST *list;
  char   *text;
} APPLINES_ARGS;

typedef struct {
  FILE    *fp;
  WNDATTR *wndattr;
  HEV      hev;
} APPFILE_ARGS;

/*-------------------------Function Prototypes----------------------------*/
BOOL AddLine( LPLIST*, char*, USHORT );

/*------------------------------Globals-----------------------------------*/
extern char **_argv;

/*---------------------------------------------------------------------------
                                SaveList
---------------------------------------------------------------------------*/
BOOL SaveList( char *filename, LPLIST *list, BOOL marked, BOOL threaded )
{
 void savethread( void * );
 SAVEARGS *args;
 FILE     *fp;

  fp = fopen( filename, "w" );
  if (fp == NULL)
     return FALSE;

  args = (SAVEARGS*)malloc( sizeof(SAVEARGS) );
  args->fp = fp;
  args->list = list;
  args->marked = marked;
  if (threaded == TRUE)
     _beginthread( savethread, NULL, 0x2000, (void*)args );
  else
     savethread( (void*)args );

 return TRUE;
}
/*---------------------------------------------------------------------------
                                savethread
---------------------------------------------------------------------------*/
void savethread( void *args )
{
 LPELEM   *elem;
 SAVEARGS *sargs;
 int       writeit;

   sargs = args;
   elem = sargs->list->next;
   while (elem != sargs->list) {
      writeit = !sargs->marked;
      writeit |= sargs->marked && elem->nextmk;
      if (writeit)
         fputs( (char*)elem->data, sargs->fp );
      elem = elem->next;
      if (elem != sargs->list && writeit)
         fputc( '\n', sargs->fp );
   }
   fclose( sargs->fp );
   free( args );

   return;
}
/*---------------------------------------------------------------------------
                               CorrectScroll
---------------------------------------------------------------------------*/
void CorrectScroll( WNDATTR *wndattr, int delta )
{
 int maxmove;

   if (delta == 0)
      return;

   maxmove = -wndattr->scrollpos;
   if (delta < maxmove) {
      delta = maxmove;
   } else {
      maxmove = LPListLines( wndattr->list ) - wndattr->scrollpos;
      if (delta > maxmove)
         delta = maxmove;
   }

   WinScrollWindow( wndattr->Client,
                     0,
                     delta * wndattr->fontmax,
                     NULL, NULL, NULLHANDLE, NULL,
                     SW_INVALIDATERGN );

   wndattr->scrollpos += delta;
   return;
}
/*---------------------------------------------------------------------------
                            PasteFromClipboard
---------------------------------------------------------------------------*/
void PasteFromClipboard( HAB hab, HWND wnd )
{
 WNDATTR *wndattr;
 char *text;

    WinOpenClipbrd( hab );
    text = ( char * )WinQueryClipbrdData( hab, CF_TEXT );

    if (text) {
       wndattr = WinQueryWindowPtr( wnd, QWL_USER );
       AppendLines( wndattr, text, FALSE, NULLHANDLE );
    }

    WinCloseClipbrd( hab );

   return;
}
/*---------------------------------------------------------------------------
                               PositionDialog
---------------------------------------------------------------------------*/
void PositionDialog( HWND dlgwnd )
{
 HWND frame;
 SWP  swp;
 int  x, y;
 long maxx, maxy;

   frame = WinQueryWindow( dlgwnd, QW_OWNER );

   WinQueryWindowPos( frame, &swp );
   x = swp.x + swp.cx / 2;
   y = swp.y + swp.cy / 2;

   WinQueryWindowPos( dlgwnd, &swp );
   x -= swp.cx / 2;
   y -= swp.cy / 2;

   maxx = WinQuerySysValue( HWND_DESKTOP, SV_CXSCREEN ) - swp.cx;
   maxy = WinQuerySysValue( HWND_DESKTOP, SV_CYSCREEN ) - swp.cy;
   x = min( x, maxx );
   x = max( x, 0 );
   y = min( y, maxy );
   y = max( y, 0 );

   WinSetWindowPos( dlgwnd, NULLHANDLE, x, y, 0, 0,
                    SWP_MOVE | SWP_ACTIVATE );
   return;
}
/*-------------------------------------------------------------------------
                                AppendFile
--------------------------------------------------------------------------*/
BOOL AppendFile( WNDATTR *wndattr, char *filename, BOOL threaded, HEV hev )
{
 void   addfile( void * );
 APPFILE_ARGS *Args;

   Args = ( APPFILE_ARGS * )malloc( sizeof( APPFILE_ARGS ) );
   Args->fp = fopen( filename, "r" );

   Args->hev = hev;

   if (Args->fp == NULL) {
      free( Args );
      if (hev != NULLHANDLE)
         DosPostEventSem( hev );
      return FALSE;
   }

   Args->wndattr = wndattr;

   if (threaded)
      _beginthread( addfile, NULL, 0x2000, Args );
   else
      addfile( Args );

  return TRUE;
}
/*-------------------------------------------------------------------------
                                addfile
--------------------------------------------------------------------------*/
void addfile( void *arg )
{
 APPFILE_ARGS *Args;
 char   *buffer;
 USHORT  maxlen, len;
 USHORT  lines_added;

   Args = arg;

   lines_added = 0;

   maxlen = LPListMaxLen( Args->wndattr->list );
   buffer = (char*)malloc( ( size_t )maxlen + 1 );

   if (buffer != NULL) {
      while (fgets( buffer, maxlen, Args->fp )) {
         len = strcspn( buffer, "\n\r" );
         if (AddLine( Args->wndattr->list, buffer, len ) == TRUE)
            lines_added++;
      }
      free( buffer );
   }
   fclose( Args->fp );

   WinPostMsg( Args->wndattr->Client, FLM_CORRECTWINDOW,
               MPFROMSHORT( lines_added ), 0);

   if (Args->hev != NULLHANDLE)
      DosPostEventSem( Args->hev );

   free( Args );

   return;
}
/*--------------------------------------------------------------------------
                               AppendLines
--------------------------------------------------------------------------*/
BOOL AppendLines( WNDATTR *wndattr, char *text, BOOL threaded, HEV hev )
{
 void addlines( void * );
 APPLINES_ARGS *Args;

   Args = ( APPLINES_ARGS * )malloc( sizeof( APPLINES_ARGS ) );
   Args->text = text;
   Args->wnd = wndattr->Client;
   Args->list = wndattr->list;
   Args->hev = hev;

   if (threaded)
      _beginthread( addlines, NULL, 0x2000, Args );
   else
      addlines( Args );

  return TRUE;
}
/*-------------------------------------------------------------------------
                                addlines
--------------------------------------------------------------------------*/
void addlines( void *arg )
{
 USHORT  len, maxlen, linelen, lines_added;
 APPLINES_ARGS *Args;
 BOOL ret;

   Args = arg;

   maxlen = LPListMaxLen( Args->list );
   lines_added = 0;

   do {
      len = strcspn( Args->text, "\r\n" );

      linelen = min( len, maxlen );

      if (AddLine( Args->list, Args->text, linelen ) == TRUE)
         lines_added++;

      Args->text += len;

      Args->text += ( *Args->text == '\n' || *Args->text == '\r' );
      Args->text += ( *Args->text == '\n' || *Args->text == '\r' );

   } while (*Args->text);

  if (Args->hev)
     DosPostEventSem( Args->hev );

   ret = WinPostMsg( Args->wnd, FLM_CORRECTWINDOW,
                     MPFROMSHORT( lines_added ), 0);

   free( Args );

   return;
}
/*-------------------------------------------------------------------------
                               AppendLine
--------------------------------------------------------------------------*/
BOOL AppendLine( WNDATTR *wndattr, char *text )
{
 USHORT  len;

   len = strcspn( text, "\r\n" );
   AddLine( wndattr->list, text, len );

   WinPostMsg( wndattr->Client, FLM_CORRECTWINDOW, MPFROMSHORT( 1 ), 0);

  return TRUE;
}
/*-------------------------------------------------------------------------
                                AddLine
--------------------------------------------------------------------------*/
BOOL AddLine( LPLIST *list, char *text, USHORT len )
{
 LPELEM *elem;
 BOOL    ret;

   if (text == NULL)
      return FALSE;

   elem = LPElemCreate();
   if (elem == NULL) {
      ret = FALSE;
   } else {
      DosEnterCritSec( );
      if (LPElemSetData( list, elem, text, len ) == FALSE) {
         LPElemDestroy( elem );
         ret = FALSE;
      } else {
         LPElemAppend( list, elem );
         ret = TRUE;
      }
      DosExitCritSec( );
   }

  return ret;
}

/*-------------------------------------------------------------------------
                             GetTempDirName
--------------------------------------------------------------------------*/
void GetTempDirName( char *buffer )
{
 char *value;

  strcpy( buffer, _argv[0] );
  *strrchr( buffer, '\\' ) = '\0';

  if ((DosScanEnv( "TEMP", &value ) == 0) ||
      (DosScanEnv( "TMP", &value ) == 0 ))
         strcpy( buffer, value );

  if (buffer[strlen( buffer ) - 1] != '\\')
     strcat( buffer, "\\" );

  return;
}
