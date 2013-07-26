extern MRESULT ActMsgBeginDrag( WNDATTR * );
extern MRESULT ActMsgEndConversation( void );
extern MRESULT ActMsgDragOver( MPARAM, MPARAM );
extern MRESULT ActMsgRender( WNDATTR*, MPARAM );
extern MRESULT ActMsgRenderComplete( WNDATTR*, MPARAM, MPARAM );
extern MRESULT ActMsgDrop( WNDATTR *, MPARAM, MPARAM );
extern MRESULT ActMsgDragLeave( void );
extern MRESULT ActMsgPrintObject( WNDATTR*, MPARAM, MPARAM );
extern MRESULT ActMsgDiscardObject( WNDATTR* );
