#ifndef INCL_FLWIN
 #define INCL_FLWIN

 #ifndef INCL_LIST
  #include "list.h"
 #endif

 #define FLM_SETWINDOW     WM_USER+1
 #define FLM_CORRECTWINDOW WM_USER+2
 #define FLM_CLEAR         WM_USER+3
 #define FLM_CLEARMARKED   WM_USER+4
 #define FLM_USER          WM_USER+5

 #define NO_SELECT         -1

 #define ID_FLWIN_RES 1

 typedef struct {
   HAB      hab;
   HWND     Frame;
   HWND     Client;
   HWND     Scroll;
   LONG     fontmax;
   LONG     fontbase;
   LONG     fontwidth;
   char     font[50];
   USHORT   scrollpos;
   USHORT   prevscrollpos;
   USHORT   winlines;
   int      xpos;
   short    select;
   BOOL     escape;
   LPLIST  *list;
   LPELEM  *current;
   void    *extra;    /* use for whatever you want  */
 } WNDATTR;

 typedef struct {
   HAB     hab;
   ULONG   flags;
   USHORT  maxlines;
   USHORT  maxlen;
   BOOL    maximize;
   USHORT  winheight;
   USHORT  winwidth;
   short   xpos;
   short   ypos;
   char    font[50];
 } CREATEOPS;

 HWND EXTFUNC FLCreateWindow( CREATEOPS * );

#endif
