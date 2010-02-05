#ifndef _GetWord_H_
#define _GetWord_H_

#include <windows.h>

typedef enum TKnownWndClass {
	kwcUnknown,
	kwcRichEdit,
	kwcMultiLineEdit,
	kwcSingleLineEdit,
	kwcInternetExplorer_Server,
	kwcConsole,
} TKnownWndClass;

TKnownWndClass GetWindowType(HWND WND, const TCHAR* WNDClass);
/* return value in utf-8 */
char* TryGetWordFromAnyWindow(TKnownWndClass WndType, HWND WND, POINT Pt, int *BeginPos);

#endif
