extern void PasteFromClipboard( HAB, HWND );
extern BOOL SaveList( char*, LPLIST*, BOOL, BOOL );

extern void CorrectScroll( WNDATTR*, int );
extern void PositionDialog( HWND );

extern BOOL AppendFile( WNDATTR *, char *, BOOL, HEV );
extern BOOL AppendLines( WNDATTR *, char *, BOOL, HEV );
extern BOOL AppendLine( WNDATTR *, char * );

extern void GetTempDirName( char* );
