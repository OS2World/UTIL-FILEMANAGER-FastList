#ifndef INCL_LIST
 #define INCL_LIST

 #define EXTFUNC

 typedef struct _LPELEM {
   PVOID  data;
   ULONG  userdata;
   struct _LPELEM *next;
   struct _LPELEM *prev;
   struct _LPELEM *nextmk;
 } LPELEM;

 typedef LPELEM LPLIST;

 typedef struct {
   USHORT  lines;
   USHORT  mklines;
   USHORT  maxlines;
   USHORT  maxlen;
   LPELEM *lastmk;
   HMTX    lock;
 } LPINFO;

 LPELEM * EXTFUNC LPListCreate( void );
 BOOL     EXTFUNC LPListDestroy( LPLIST * );
 BOOL     EXTFUNC LPListClear( LPLIST * );
 BOOL     EXTFUNC LPListClearMarked( LPLIST * );
 LPELEM * EXTFUNC LPElemCreate( void );
 BOOL     EXTFUNC LPElemDestroy( LPELEM * );
 BOOL     EXTFUNC LPElemAppend( LPLIST *, LPELEM * );
 BOOL     EXTFUNC LPElemInsert( LPLIST *, LPELEM *, LPELEM * );
 BOOL     EXTFUNC LPElemDelete( LPLIST *, LPELEM * );
 BOOL     EXTFUNC LPElemRemove( LPLIST *, LPELEM * );
 BOOL     EXTFUNC LPElemSetData( LPLIST *, LPELEM *, char *, USHORT );
 BOOL     EXTFUNC LPElemExchange( LPLIST *, LPELEM *, LPELEM * );
 LPELEM * EXTFUNC LPElemTrack( LPLIST *, LPELEM *, int );
 LPELEM * EXTFUNC LPElemRotate( LPLIST *, LPELEM *, int );
 BOOL     EXTFUNC LPElemMark( LPLIST *, LPELEM * );
 BOOL     EXTFUNC LPElemUnmark( LPLIST *, LPELEM * );

 #define LPListLines( LIST )        ( ( ( LPINFO * )( LIST->data ) )->lines )
 #define LPListMarkedLines( LIST )  ( ( ( LPINFO * )( LIST->data ) )->mklines )
 #define LPListMaxLines( LIST )     ( ( ( LPINFO * )( LIST->data ) )->maxlines )
 #define LPListMaxLen( LIST )       ( ( ( LPINFO * )( LIST->data ) )->maxlen )
 #define LPListLock( LIST )         ( ( ( LPINFO * )( LIST->data ) )->lock )

#endif
