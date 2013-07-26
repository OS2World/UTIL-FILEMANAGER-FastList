/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
List

Double-linked list functions
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
*/

#define INCL_DOSSEMAPHORES
#include <os2.h>
#include <stdlib.h>
#include <string.h>
#include "list.h"

/*-------------------------------------------------------------------------
                              LPListCreate
--------------------------------------------------------------------------*/
LPLIST * EXTFUNC LPListCreate( )
{
 LPLIST *list;
 LPINFO *lpinfo;

   list = LPElemCreate( );

   if (list) {
      lpinfo = ( LPINFO * )malloc( sizeof( LPINFO ) );
      if (lpinfo == NULL) {
         LPElemDestroy( list );
         list = ( LPLIST * )NULL;
      } else {
         lpinfo->maxlines = ~0;
         lpinfo->maxlen = ~0;
         lpinfo->lines = lpinfo->mklines = 0;
         lpinfo->lastmk = list;
         list->data = lpinfo;
         list->next = list->prev = list->nextmk = list;
      }
   }

   return list;
}
/*-------------------------------------------------------------------------
                              LPListDestroy
--------------------------------------------------------------------------*/
BOOL EXTFUNC LPListDestroy( LPLIST *list )
{
   if (list == NULL)
      return FALSE;

   LPListClear( list );
   LPElemDestroy( list );

   return TRUE;
}
/*-------------------------------------------------------------------------
                              LPListClear
--------------------------------------------------------------------------*/
BOOL EXTFUNC LPListClear( LPLIST *list )
{
 LPINFO *lpinfo;
 LPELEM *elem, *nextelem;

   if (list == NULL)
      return FALSE;

   elem = list->next;

   while (elem != list) {
      nextelem = elem->next;
      LPElemDestroy( elem );
      elem = nextelem;
   }

   lpinfo = ( LPINFO * )list->data;
   lpinfo->lines = lpinfo->mklines = 0;
   lpinfo->lastmk = list;
   list->next = list->prev = list->nextmk = list;

   return TRUE;
}
/*-------------------------------------------------------------------------
                           LPListClearMarked
--------------------------------------------------------------------------*/
BOOL EXTFUNC LPListClearMarked( LPLIST *list )
{
 LPELEM *elem, *nextelem;

   if (list == NULL)
      return FALSE;

   elem = list->nextmk;

   while (elem != list) {
      nextelem = elem->nextmk;
      LPElemDelete( list, elem );
      elem = nextelem;
   }

   return TRUE;
}
/*-------------------------------------------------------------------------
                              LPElemCreate
--------------------------------------------------------------------------*/
LPELEM * EXTFUNC LPElemCreate( void )
{
 LPELEM *elem;

   elem = ( LPELEM * )malloc( sizeof( LPELEM ) );
   if (elem) {
      elem->data = ( char * )NULL;
      elem->next = elem->prev = elem->nextmk = ( LPELEM * )NULL;
      elem->userdata = 0L;
   }

   return elem;
}
/*-------------------------------------------------------------------------
                              LPElemDestroy
--------------------------------------------------------------------------*/
BOOL EXTFUNC LPElemDestroy( LPELEM *elem )
{
   if (elem == NULL)
      return FALSE;

   if (elem->data)
      free( elem->data );

   free( elem );

   return TRUE;
}
/*-------------------------------------------------------------------------
                              LPElemAppend
--------------------------------------------------------------------------*/
BOOL EXTFUNC LPElemAppend( LPLIST *list, LPELEM *elem )
{
   LPElemInsert( list, list->prev, elem );

   return TRUE;
}
/*-------------------------------------------------------------------------
                              LPElemInsert
--------------------------------------------------------------------------*/
BOOL EXTFUNC LPElemInsert( LPLIST *list, LPELEM *afterelem, LPELEM *elem )
{
 LPINFO *lpinfo;

   if (afterelem == NULL || elem == NULL || list == NULL)
      return FALSE;

   lpinfo = ( LPINFO * )list->data;
   if (lpinfo == NULL)
      return FALSE;

   if (lpinfo->lines == lpinfo->maxlines)
      LPElemDelete( list, list->next );

   elem->prev = afterelem;
   elem->next = afterelem->next;
   elem->next->prev = elem;
   afterelem->next = elem;
   ++lpinfo->lines;

   return TRUE;
}
/*-------------------------------------------------------------------------
                              LPElemDelete
--------------------------------------------------------------------------*/
BOOL EXTFUNC LPElemDelete( LPLIST *list, LPELEM *elem )
{
   if (LPElemRemove( list, elem ) == FALSE)
      return FALSE;

   if (elem->nextmk)
      if (LPElemUnmark( list, elem ) == FALSE)
         return FALSE;

   LPElemDestroy( elem );

   return TRUE;
}
/*-------------------------------------------------------------------------
                              LPElemRemove
--------------------------------------------------------------------------*/
BOOL EXTFUNC LPElemRemove( LPLIST *list, LPELEM *elem )
{
 LPINFO *lpinfo;

   if (list == NULL || elem == NULL || elem == list)
      return FALSE;

   lpinfo = ( LPINFO * )list->data;
   if (lpinfo == NULL)
      return FALSE;

   if (lpinfo->lines == 0)
      return FALSE;

   if (elem->prev)
      elem->prev->next = elem->next;

   if (elem->next)
      elem->next->prev = elem->prev;

   --lpinfo->lines;

   return TRUE;
}

/*-------------------------------------------------------------------------
                              LPElemSetData
--------------------------------------------------------------------------*/
BOOL EXTFUNC LPElemSetData( LPLIST *list, LPELEM *elem,
                            char *data, USHORT len )
{
 LPINFO  *lpinfo;
 char    *dest;

  if (  list == NULL || list->data == NULL || elem == NULL
     || data == NULL || elem == list)
      return FALSE;

  lpinfo = list->data;

  if (elem->data != NULL)
     free( elem->data );

  len = min( len, lpinfo->maxlen );

  elem->data = malloc( ( size_t )len + 1 );
  dest = elem->data;
  if (dest != NULL) {
     memcpy( dest, data, len );
     *( dest + len ) = '\0';
  }

  if (dest == NULL)
     return FALSE;

  return TRUE;
}
/*-------------------------------------------------------------------------
                              LPElemExchange
Retains position in mark list, not just value swapping.
--------------------------------------------------------------------------*/
BOOL EXTFUNC LPElemExchange( LPLIST *list, LPELEM *elem1, LPELEM *elem2 )
{
 LPELEM *tmp;

  if (list == NULL || elem1 == NULL || elem2 == NULL)
     return FALSE;

  tmp = elem2->prev;

  LPElemRemove( list, elem2 );
  LPElemInsert( list, elem1->prev, elem2 );

  if (tmp != elem1) {
     LPElemRemove( list, elem1 );
     LPElemInsert( list, tmp, elem1 );
  }

  return TRUE;
}
/*-------------------------------------------------------------------------
                              LPElemTrack
--------------------------------------------------------------------------*/
LPELEM * EXTFUNC LPElemTrack( LPLIST *list, LPELEM *elem, int delta )
{
 LPINFO *lpinfo;
 int direction;

   if (elem == NULL || list == NULL)
      return ( LPELEM * )NULL;

   direction = ( delta < 0 ) ? 1 : -1;

   lpinfo = ( LPINFO * )list->data;
   if (abs( delta ) > lpinfo->lines)
      return list;

   while (delta != 0) {
      elem = ( direction == 1 ) ? elem->prev : elem->next;
      delta += direction;
      if (elem == list)
         break;
   }

   return elem;
}
/*-------------------------------------------------------------------------
                              LPElemRotate
--------------------------------------------------------------------------*/
LPELEM * EXTFUNC LPElemRotate( LPLIST *list, LPELEM *elem, int delta )
{
 LPINFO *lpinfo;
 int direction;

   if (elem == NULL && list == NULL)
      return ( LPELEM * )NULL;

   direction = ( delta < 0 ) ? 1 : -1;

   lpinfo = ( LPINFO * )list->data;

   if (delta > lpinfo->lines && lpinfo->lines != 0)
      delta %= lpinfo->lines;

   if (abs( delta ) > ( lpinfo->lines / 2 + 1 )) {
      delta += lpinfo->lines * direction;
      direction = -direction;
   }

   while (delta != 0) {
      elem = ( direction == 1 ) ? elem->prev : elem->next;
      if (elem != list)
         delta += direction;
   }

   return elem;
}
/*-------------------------------------------------------------------------
                              LPElemMark
--------------------------------------------------------------------------*/
BOOL EXTFUNC LPElemMark( LPLIST *list, LPELEM *elem )
{
 LPINFO *lpinfo;

   if (list == NULL || elem == NULL || elem == list || elem->nextmk)
      return FALSE;

   lpinfo = list->data;
   lpinfo->lastmk->nextmk = elem;
   lpinfo->lastmk = elem;
   elem->nextmk = list;
   lpinfo->mklines++;

   return TRUE;
}
/*-------------------------------------------------------------------------
                             LPElemUnmark
--------------------------------------------------------------------------*/
BOOL EXTFUNC LPElemUnmark( LPLIST *list, LPELEM *elem )
{
 LPINFO *lpinfo;
 LPELEM *xelem;

   if (list == NULL || elem == NULL || elem->nextmk == NULL || elem == list)
      return FALSE;

   lpinfo = list->data;
   xelem = list;
   while (xelem->nextmk != list) {
      if (xelem->nextmk == elem) {
         xelem->nextmk = elem->nextmk;
         elem->nextmk = ( LPELEM * )NULL;
         if (elem == lpinfo->lastmk)
            lpinfo->lastmk = xelem;
         lpinfo->mklines--;
         return TRUE;
      }
      xelem = xelem->nextmk;
   }

   return FALSE;
}
