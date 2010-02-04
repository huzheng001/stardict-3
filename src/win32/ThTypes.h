#ifndef _ThTypes_H_
#define _ThTypes_H_

#include <windows.h>

#ifdef __cplusplus
extern "C"
{
#endif				/* __cplusplus */

/* Maximum size of scanned text: strlen(text) < STARDICT_MAX_INDEX_KEY_SIZE. */
#define MAX_SCAN_TEXT_SIZE 256

typedef struct TCurrentMode {
	HWND WND;
	POINT Pt;
	size_t WordLen;
	/* in utf-8 */
	char MatchedWord[MAX_SCAN_TEXT_SIZE];
	int BeginPos;
} TCurrentMode;

typedef struct TGlobalDLLData {
	HWND ServerWND;
	HHOOK g_hHookMouse;
	UINT_PTR TimerID;
	HWND LastWND;
	POINT LastPt;
	TCurrentMode CurMod;
	TCHAR LibName[MAX_PATH];
} TGlobalDLLData;

extern TGlobalDLLData *GlobalData;


void ThTypes_Init();
void Thtypes_End();

#ifdef __cplusplus
}
#endif				/* __cplusplus */

#endif
