/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 drag.c

 'Fast List' drag and drop
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
*/
#define INCL_DOS
#define INCL_PM
#define INCL_SPLDOSPRINT
#include <os2.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <process.h>
#include "list.h"
#include "flwin.h"
#include "menu.h"
#include "drag.h"
#include "util.h"
#include "res.h"

/*--------------------------------Macros-----------------------------------*/

/*-------------------------Function Declarations---------------------------*/
static void CleanUpDrag( void );

/*-------------------------------Globals-----------------------------------*/
static DRAGINFO *DragInfo;

char  FileName[CCHMAXPATHCOMP];
char *DefaultFileName = "list.txt";
char *RMF = "(DRM_OS2FILE,DRM_PRINT,DRM_DISCARD)X(DRF_TEXT)";


/*-------------------------------Code Start--------------------------------*/


/*---------------------------------------------------------------------------
                              ActMsgBeginDrag
---------------------------------------------------------------------------*/
MRESULT ActMsgBeginDrag( WNDATTR *wndattr )
{
 DRAGITEM  dragitem;
 DRAGIMAGE dragimage;
 HWND      hwndTarget;
 int       marked;

  DragInfo = DrgAllocDraginfo( 1 );
  DragInfo->hwndSource = wndattr->Client;

  dragitem.hwndItem = wndattr->Client;
  dragitem.ulItemID = 1;
  dragitem.hstrType = DrgAddStrHandle( DRT_TEXT );
  dragitem.hstrRMF = DrgAddStrHandle( RMF );
  dragitem.hstrTargetName = DrgAddStrHandle( DefaultFileName );
  dragitem.hstrContainerName = NULLHANDLE;
  dragitem.hstrSourceName = NULLHANDLE;
  dragitem.fsControl = 0;
  dragitem.fsSupportedOps = DO_COPYABLE | DO_MOVEABLE;

  DrgSetDragitem( DragInfo, &dragitem, sizeof dragitem, 0 );

  dragimage.cb = sizeof dragimage;
  dragimage.cxOffset = 0;
  dragimage.cyOffset = 0;
  dragimage.fl = DRG_ICON | DRG_STRETCH;

  marked = (LPListMarkedLines( wndattr->list ) != 0);
  dragimage.sizlStretch.cx = marked ? 20 : 32;
  dragimage.sizlStretch.cy = marked ? 20 : 32;

  dragimage.hImage = WinLoadPointer( HWND_DESKTOP,
                                     NULLHANDLE,
                                     ID_FLWIN_RES );

  hwndTarget = DrgDrag( wndattr->Client,
                        DragInfo,
                        &dragimage,
                        1L,
                        VK_ENDDRAG,
                        NULL );

  if (hwndTarget == NULLHANDLE)
     CleanUpDrag();

 return MRFROMSHORT( FALSE );
}
/*---------------------------------------------------------------------------
                              ActMsgDragOver
---------------------------------------------------------------------------*/
MRESULT ActMsgDragOver( MPARAM mp1, MPARAM mp2 )
{
 DRAGITEM *dragitem;
 USHORT    response, action;

  DragInfo = (DRAGINFO*)PVOIDFROMMP( mp1 );
  DrgAccessDraginfo( DragInfo );

  action = DOR_NODROP;
  if (DragInfo->cditem == 1) {
     if (DragInfo->usOperation == DO_COPY ||
         DragInfo->usOperation == DO_MOVE ||
         DragInfo->usOperation == DO_DEFAULT ) {
         dragitem = DrgQueryDragitemPtr( DragInfo, 0 );
         if (dragitem->fsSupportedOps & DO_COPYABLE) {
            response = DOR_DROP;
            action = DragInfo->usOperation;
            action = (action == DO_DEFAULT) ? DO_COPY : action;
         }
     }
  }
  DrgFreeDraginfo( DragInfo );
  DragInfo = NULL;

  return MRFROM2SHORT( response, action );
}
/*---------------------------------------------------------------------------
                            ActMsgDragLeave
---------------------------------------------------------------------------*/
MRESULT ActMsgDragLeave( )
{
 return MRFROMSHORT( FALSE );
}

/*---------------------------------------------------------------------------
                               ActMsgDrop
---------------------------------------------------------------------------*/
MRESULT ActMsgDrop( WNDATTR *wndattr, MPARAM mp1, MPARAM mp2 )
{
 DRAGITEM     *dragitem;
 DRAGTRANSFER *dragxfer;
 BOOL          done;
 char  path[CCHMAXPATHCOMP];
 char  filename[CCHMAXPATHCOMP];
 int   cnt;

   DragInfo = (DRAGINFO*)PVOIDFROMMP( mp1 );
   DrgAccessDraginfo( DragInfo );

   dragitem = DrgQueryDragitemPtr( DragInfo, 0 );

   cnt = DrgQueryStrName( dragitem->hstrContainerName,
                          sizeof path,
                          path );
   done = FALSE;

   if (cnt) {
      DosQueryPathInfo( path, FIL_QUERYFULLNAME, path, sizeof path );

      cnt = DrgQueryStrName( dragitem->hstrSourceName,
                             sizeof filename,
                             filename );
      if (cnt) {
         strcat( path, filename );
         done = AppendFile( wndattr, path, 0, NULLHANDLE );
      }
   }
   if (done == FALSE) {
       dragxfer = DrgAllocDragtransfer( 1 );
       dragxfer->hwndClient = wndattr->Client;
       dragxfer->pditem = dragitem;
       GetTempDirName( path );
       strcat( path, DefaultFileName );
       dragxfer->hstrRenderToName = DrgAddStrHandle( path );
       dragxfer->hstrSelectedRMF = DrgAddStrHandle( "<DRM_OS2FILE,DRF_TEXT>" );
       dragxfer->usOperation = DragInfo->usOperation;
       DrgSendTransferMsg( DragInfo->hwndSource,
                           DM_RENDER,
                           MPFROMP( dragxfer ),
                           0 );
   } else {
      WinPostMsg( DragInfo->hwndSource,
                  DM_ENDCONVERSATION,
                  MPFROMLONG( dragitem->ulItemID ),
                  MPFROMLONG( DMFL_TARGETSUCCESSFUL ) );
      CleanUpDrag();
  }


 return MRFROMSHORT( 0 );
}
/*---------------------------------------------------------------------------
                             ActMsgRender
---------------------------------------------------------------------------*/
MRESULT ActMsgRender( WNDATTR *wndattr, MPARAM mp1 )
{
 void do_render( void * );
 DRAGTRANSFER *dragxfer;

  dragxfer = PVOIDFROMMP( mp1 );
  if ((dragxfer->usOperation & (DO_COPY | DO_MOVE))) {
      wndattr->extra = dragxfer;
      _beginthread( do_render, NULL, 0x2000, wndattr );
  }

  return MRFROMSHORT( TRUE );
}
/*---------------------------------------------------------------------------
                               do_render
---------------------------------------------------------------------------*/
void do_render ( void *arg )
{
 WNDATTR      *wndattr;
 DRAGTRANSFER *dragxfer;
 char          filename[CCHMAXPATHCOMP];
 ULONG         mp2;
 ULONG         msg;
 int           marked;

  wndattr = (WNDATTR*)arg;
  dragxfer = (DRAGTRANSFER*)wndattr->extra;

  marked = (LPListMarkedLines( wndattr->list ) != 0);
  DrgQueryStrName( dragxfer->hstrRenderToName,
                   sizeof filename,
                   filename );

  mp2 = DMFL_RENDERFAIL;
  if (SaveList( filename, wndattr->list, marked, FALSE ) == TRUE) {
     if (dragxfer->usOperation & DO_MOVE) {
        msg = marked ? FLM_CLEARMARKED : FLM_CLEAR;
        WinPostMsg( wndattr->Client, msg, 0, 0 );
     }
     mp2 = DMFL_RENDEROK;
  }
  DrgPostTransferMsg( dragxfer->hwndClient,
                      DM_RENDERCOMPLETE,
                      dragxfer,
                      mp2,
                      0L,
                      FALSE);


  DrgFreeDragtransfer( dragxfer );

  return;
}
/*---------------------------------------------------------------------------
                          ActMsgRenderComplete
---------------------------------------------------------------------------*/
MRESULT ActMsgRenderComplete( WNDATTR *wndattr, MPARAM mp1, MPARAM mp2 )
{
 DRAGTRANSFER *dragxfer;
 DRAGITEM     *dragitem;
 ULONG         response;
 char  filename[CCHMAXPATHCOMP];

   dragxfer = PVOIDFROMMP( mp1 );
   dragitem = dragxfer->pditem;
   response = DMFL_TARGETFAIL;
   if (SHORT1FROMMP( mp2 ) == DMFL_RENDEROK) {
      DrgQueryStrName( dragxfer->hstrRenderToName,
                       sizeof filename,
                       filename );
      if (AppendFile( wndattr, filename, 0, NULLHANDLE ) == TRUE)
         response = DMFL_TARGETSUCCESSFUL;
      DosDelete( filename );
   }
   DrgDeleteStrHandle( dragxfer->hstrRenderToName );
   DrgDeleteStrHandle( dragxfer->hstrSelectedRMF );
   DrgFreeDragtransfer( dragxfer );

   DrgSendTransferMsg( DragInfo->hwndSource,
                       DM_ENDCONVERSATION,
                       MPFROMLONG( dragitem->ulItemID ),
                       MPFROMLONG( response ) );
   CleanUpDrag();

   return MRFROMSHORT( 0 );
}
/*---------------------------------------------------------------------------
                          ActMsgPrintObject
---------------------------------------------------------------------------*/
MRESULT ActMsgPrintObject( WNDATTR *wndattr, MPARAM mp1, MPARAM mp2 )
{
 PRINTDEST     *pdest;
 DEVOPENSTRUC  *dopstruct;
 HSPL           hspl;
 LPELEM        *elem;
 BOOL           marked;

   pdest = PVOIDFROMMP( mp2 );
   dopstruct = (DEVOPENSTRUC*)pdest->pdopData;

   dopstruct->pszDataType = "PM_Q_RAW";
   hspl = SplQmOpen( NULL, 5L, (PQMOPENDATA)dopstruct );
   if (hspl != SPL_ERROR) {
      if (SplQmStartDoc( hspl, "List" ) == TRUE) {
         marked = (LPListMarkedLines( wndattr->list ) != 0);
         elem = wndattr->list->next;
         while (elem != wndattr->list) {
            if ((elem->nextmk && marked) || !marked) {
               SplQmWrite( hspl, strlen( elem->data ), elem->data );
               SplQmWrite( hspl, 2, "\r\n" );
            }
            elem = elem->next;
         }
         SplQmEndDoc( hspl );
      }
      SplQmClose( hspl );
   }

 return MRFROMSHORT( DRR_SOURCE );
}
/*---------------------------------------------------------------------------
                          ActMsgDiscardObject
---------------------------------------------------------------------------*/
MRESULT ActMsgDiscardObject( WNDATTR *wndattr )
{
 ULONG msg;

   if (LPListMarkedLines( wndattr->list ) == 0)
      msg = FLM_CLEAR;
   else
      msg = FLM_CLEARMARKED;

    WinSendMsg( wndattr->Client, msg, 0, 0 );

 return MRFROMSHORT( DRR_SOURCE );
}

/*---------------------------------------------------------------------------
                           ActMsgEndConversation
---------------------------------------------------------------------------*/
MRESULT ActMsgEndConversation( )
{
  CleanUpDrag();

  return MRFROMSHORT( FALSE );
}
/*---------------------------------------------------------------------------
                              CleanUpDrag
---------------------------------------------------------------------------*/
void CleanUpDrag()
{
   if (DragInfo != NULL) {
      DrgDeleteDraginfoStrHandles( DragInfo );
      DrgFreeDraginfo( DragInfo );
      DragInfo = NULL;
   }
}
