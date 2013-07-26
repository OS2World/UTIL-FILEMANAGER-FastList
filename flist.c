/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 Fast List (For OS/2)

  :General file listing and text line processing

 Original release:
          Russ Weathersby
          August 1996
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
*/

/*------------------------------Includes-----------------------------------*/
#define INCL_PM
#define INCL_BASE
#define INCL_WORKPLACE
#include <os2.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <process.h>
#include "list.h"
#include "flwin.h"
#include "menu.h"
#include "drag.h"
#include "util.h"

/*-------------------------------Defines-----------------------------------*/
#define LOGHEIGHT  250
#define LOGWIDTH   400
#define MAX_LINES  4000
#define MAX_LENGTH 256

#define HF_STDIN   0
#define HF_STDOUT  1

#define DEFAULT_FONT "8.Courier"

/*-------------------------Structs and Typedefs----------------------------*/
typedef struct {
   WNDATTR *wndattr;
   int argc;
   char **argv;
} IN_FILES_ARGS;

/*--------------------------Function Prototypes----------------------------*/

static MRESULT EXPENTRY SubWndProc( HWND, ULONG, MPARAM, MPARAM );

static void SetWin( HAB, int, char*[], CREATEOPS* );
static void InFiles( HWND, int, char*[] );
static void InPipe( HWND );
static void OutPipe( HWND );
static void ReadPipe( void * );
static void ReadFiles( void * );
static int  ReadINI( CREATEOPS*, char* );
static void SaveINI( HWND, CREATEOPS * );
static void ReadArgs( CREATEOPS*, int, char*[] );

/*-------------------------------Globals-----------------------------------*/
static PFNWP     OrigWndProc;
static CREATEOPS CreateOps;
static char      INIFileName[PATH_MAX];
static BOOL      NoSave;

extern FILEDLG FileDlg;
extern HWND MateScrollWnd;

/*----------------------------Main Code Start------------------------------*/
int main( int argc, char *argv[] )
{
 HAB    hab;
 HMQ    hmq;
 HWND   frame, client;
 QMSG   qmsg;

   hab = WinInitialize( 0 );

   hmq = WinCreateMsgQueue( hab, 0 );

   SetWin( hab, argc, argv, &CreateOps );
   frame = FLCreateWindow( &CreateOps );

   if (frame != NULLHANDLE) {
      client = WinWindowFromID( frame, FID_CLIENT );
      OrigWndProc = WinSubclassWindow( client, SubWndProc );

      CreatePopup( hab, client, frame );

      InFiles( client, argc, argv );

      InPipe( client );

      WinSetFocus( HWND_DESKTOP, client );

      while (WinGetMsg( hab, &qmsg, NULLHANDLE, 0, 0 ))
         WinDispatchMsg( hab, &qmsg );

      OutPipe( client );

      SaveINI( client, &CreateOps );

      DestroyPopup();

      WinDestroyWindow( frame );
   }

   WinDestroyMsgQueue( hmq );
   WinTerminate( hab );

   return EXIT_SUCCESS;
}

/*---------------------------------------------------------------------------
                                 SetWin
---------------------------------------------------------------------------*/
void SetWin( HAB hab, int argc, char *argv[], CREATEOPS *create )
{

   if (ReadINI( create, argv[0] ) == 0) {
      create->flags = FCF_VERTSCROLL | FCF_SIZEBORDER |
                      FCF_TASKLIST | FCF_ICON | FCF_SHELLPOSITION;
      create->maximize = FALSE;
      create->maxlines = MAX_LINES;
      create->maxlen = MAX_LENGTH;
      create->winheight = LOGHEIGHT;
      create->winwidth = LOGWIDTH;
      create->xpos = create->ypos = 0;
      strcpy( create->font, DEFAULT_FONT );
   }

   ReadArgs( create, argc, argv );

   create->hab = hab;

   return;
}

/*---------------------------------------------------------------------------
                                  ReadINI
---------------------------------------------------------------------------*/
int ReadINI( CREATEOPS *create, char *path )
{
  FILE *fp;
  char *ptr;
  size_t bytes_read = 0;

   strcpy( INIFileName, path );
   ptr = strrchr( INIFileName, '.' );
   if (ptr == NULL)
      ptr = INIFileName + strlen( INIFileName );
   strcpy( ptr, ".ini" );
   strlwr( INIFileName );

   fp = fopen( INIFileName, "rb" );
   if (fp) {
      bytes_read = fread( create, 1, sizeof(CREATEOPS), fp );
      fclose( fp );
   }

   return (bytes_read == sizeof(CREATEOPS));
}
/*---------------------------------------------------------------------------
                                  SaveINI
---------------------------------------------------------------------------*/
void SaveINI( HWND wnd, CREATEOPS *save )
{
  WNDATTR  *wndattr;
  FILE     *fp;
  SWP       swp;

   wndattr = WinQueryWindowPtr( wnd, QWL_USER );

   if (wndattr->escape == TRUE || NoSave == TRUE)
      return;

   save->flags &= ~FCF_SHELLPOSITION;

   WinQueryWindowPos( wndattr->Frame, &swp );
   save->maximize = (swp.fl & SWP_MAXIMIZE) ? TRUE : FALSE;
   if (save->maximize) {
      save->winheight = WinQueryWindowUShort( wndattr->Frame, QWS_CYRESTORE );
      save->winwidth = WinQueryWindowUShort( wndattr->Frame, QWS_CXRESTORE );
      save->xpos = WinQueryWindowUShort( wndattr->Frame, QWS_XRESTORE );
      save->ypos = WinQueryWindowUShort( wndattr->Frame, QWS_YRESTORE );
   } else {
      save->winwidth = swp.cx;
      save->winheight = swp.cy;
      save->xpos = swp.x;
      save->ypos = swp.y;
    }

   strcpy( save->font, wndattr->font );

   fp = fopen( INIFileName, "wb" );
   if (fp) {
      fwrite( save, sizeof(CREATEOPS), 1, fp );
      fclose( fp );
   }

 return;
}
/*---------------------------------------------------------------------------
                                  InFiles
---------------------------------------------------------------------------*/
void InFiles( HWND client, int argc, char *argv[] )
{
 static IN_FILES_ARGS infargs;

   infargs.wndattr = WinQueryWindowPtr( client, QWL_USER );
   infargs.argc = argc;
   infargs.argv = argv;

   _beginthread( ReadFiles, NULL, 0x2000, (void*)&infargs );

   return;
}

/*---------------------------------------------------------------------------
                                ReadFiles
---------------------------------------------------------------------------*/
void ReadFiles( void *args )
{
  IN_FILES_ARGS *infargs;
  int i;
  HDIR hdir;
  char path[CCHMAXPATHCOMP];
  char fullname[CCHMAXPATHCOMP];
  char ffbuf3[64];
  ULONG fcount;
  char *argstr;
  APIRET rc;

   infargs = args;

   for (i = 1;  infargs->argc > 1; ++i, --infargs->argc ) {
      argstr = infargs->argv[i];
      if ((*argstr != '-') && (*argstr != '/')) {
         rc = DosQueryPathInfo( argstr,
                                FIL_QUERYFULLNAME,
                                path,
                                sizeof path );
         if (rc == NO_ERROR) {
            *(strrchr( path, '\\' ) + 1) = '\0';
            fcount = 1;
            hdir = HDIR_CREATE;
            rc = DosFindFirst( argstr,
                               &hdir,
                               FILE_NORMAL,
                               &ffbuf3,
                               sizeof ffbuf3,
                               &fcount,
                               FIL_STANDARD );
            while (rc == NO_ERROR) {
               strcpy( fullname, path );
               strcat( fullname, &ffbuf3[29] );

               strcpy( FileDlg.szFullFile, fullname );
               AppendFile( infargs->wndattr, fullname, 0, NULLHANDLE );

               rc = DosFindNext( hdir,
                                 &ffbuf3,
                                 sizeof ffbuf3,
                                 &fcount );
            }
         }
         DosFindClose( hdir );
      }
   }

   return;
}

/*---------------------------------------------------------------------------
                                  InPipe
---------------------------------------------------------------------------*/
void InPipe( HWND client )
{
   _beginthread( ReadPipe, NULL, 0x1000, (void*)client );

  return;
}

/*---------------------------------------------------------------------------
                                ReadPipe
---------------------------------------------------------------------------*/
void ReadPipe( void *wnd )
{
 WNDATTR *wndattr;
 USHORT   maxlen;
 char    *buffer;

   wndattr = WinQueryWindowPtr( (HWND)wnd, QWL_USER );
   maxlen = LPListMaxLen( wndattr->list );
   buffer = (char*)malloc( maxlen + 2 );
   if (buffer == NULL)
      return;

   while (fgets( buffer, maxlen, stdin ))
       AppendLine( wndattr, buffer );

   free( buffer );

  return;
}

/*---------------------------------------------------------------------------
                                 OutPipe
---------------------------------------------------------------------------*/
void OutPipe( HWND client )
{
 WNDATTR *wndattr;
 LPELEM *elem;

   wndattr = WinQueryWindowPtr( client, QWL_USER );
   elem = wndattr->list->next;
   errno = 0;
   while (elem != wndattr->list && !errno) {
       fputs( elem->data, stdout );
       fputc( '\n', stdout );
       elem = elem->next;
   }

  return;
}

/*---------------------------------------------------------------------------
                                 ReadArgs
---------------------------------------------------------------------------*/
void ReadArgs( CREATEOPS *create, int argc, char *argv[] )
{
 char *option;
 int   number;
 ULONG flags;

  while (--argc) {
     option = argv[argc];
     if (*option == '-' || *option == '/') {
        option++;
        number = atoi( option + 1 );

        switch (*option) {
           case 'w':case 'W':
              create->maxlen = number;
              break;
           case 'l':case 'L':
              create->maxlines = number;
              break;
           case 't':case 'T':
              flags = FCF_TITLEBAR | FCF_SYSMENU | FCF_MINMAX;
              if (number)
                 create->flags |= flags;
              else
                 create->flags &= ~flags;
              break;
           case 'I':case 'i':
              create->flags |= FCF_SHELLPOSITION;
              create->winheight = LOGHEIGHT;
              create->winwidth = LOGWIDTH;
              NoSave = TRUE;
              break;
           default:
              break;
        }
     }
  }
  return;
}
/*---------------------------------------------------------------------------
                                 SubWndProc

Purpose: Intercept messages destined for client window
---------------------------------------------------------------------------*/
MRESULT EXPENTRY SubWndProc( HWND wnd, ULONG msg, MPARAM mp1, MPARAM mp2 )
{
  WNDATTR *wndattr;
  HAB      hab;
  ULONG    time;
  static ULONG LastTime;
  ULONG    keystate;

   wndattr = WinQueryWindowPtr( wnd, QWL_USER );

   switch (msg) {
     case WM_DRAWCLIPBOARD:
        hab = WinQueryAnchorBlock( wnd );
        time = WinGetCurrentTime( hab );
        if (time > (LastTime + 300)) { /* some prg paste twice in succession */
           PasteFromClipboard( hab, wnd );
           LastTime = time;
        }
        break;
     case WM_CONTEXTMENU:
        ActMsgContextMenu( wndattr,
                           SHORT1FROMMP( mp1 ),
                           SHORT2FROMMP( mp1 ) );
        break;
     case WM_COMMAND:
        ActMsgCommand( wndattr, mp1, mp2 );
        break;
     case WM_VSCROLL:
        if (MateScrollWnd != NULLHANDLE) {
           keystate = WinGetKeyState( HWND_DESKTOP, VK_CTRL );
           if (!(keystate & 0x8000))
              WinPostMsg( MateScrollWnd, msg, mp1, mp2 );
           keystate = WinGetKeyState( HWND_DESKTOP, VK_ALT );
           if (keystate & 0x8000)
              return MRFROMSHORT( FALSE );
        }
        break;
     case WM_BEGINDRAG:
        if (WinGetKeyState( HWND_DESKTOP, VK_CTRL ) & 0x8000 ||
            WinGetKeyState( HWND_DESKTOP, VK_SHIFT ) & 0x8000 )
             return ActMsgBeginDrag( wndattr );
        break;
     case DM_DRAGOVER:
        return ActMsgDragOver( mp1, mp2 );
        break;
     case DM_DRAGLEAVE:
        return ActMsgDragLeave( );
        break;
     case DM_RENDER:
        return ActMsgRender( wndattr, mp1 );
        break;
     case DM_RENDERCOMPLETE:
        return ActMsgRenderComplete( wndattr, mp1, mp2 );
        break;
     case DM_DROP:
        return ActMsgDrop( wndattr, mp1, mp2 );
        break;
     case DM_ENDCONVERSATION:
        return ActMsgEndConversation();
        break;
     case DM_PRINTOBJECT:
        return ActMsgPrintObject( wndattr, mp1, mp2 );
        break;
     case DM_DISCARDOBJECT:
        return ActMsgDiscardObject( wndattr );
        break;
     default:
        break;
   }

   return OrigWndProc( wnd, msg, mp1, mp2 );
}
