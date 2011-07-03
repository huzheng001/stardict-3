/*
 * Copyright 2011 kubtek <kubtek@mail.com>
 *
 * This file is part of StarDict.
 *
 * StarDict is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * StarDict is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with StarDict.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <shlwapi.h>
#include "TextOutSpy.h"
#include "InterProcessCommunication.h"


const int MOUSEOVER_INTERVAL = 300;

HINSTANCE g_hInstance = NULL;
HANDLE hSynhroMutex = 0;
HINSTANCE hGetWordLib = 0;
typedef void (*GetWordProc_t)(TCurrentMode *);
GetWordProc_t GetWordProc = NULL;

static void SendWordToServer()
{
	if (hGetWordLib == 0) {
		hGetWordLib = LoadLibrary(GlobalData->LibName);
		if (hGetWordLib) {
			GetWordProc = (GetWordProc_t)GetProcAddress(hGetWordLib, "GetWord");
		}
		else {
			hGetWordLib = (HINSTANCE)-1;
		}
	}
	if (GetWordProc) {
		GlobalData->CurMod.WND = GlobalData->LastWND;
		GlobalData->CurMod.Pt = GlobalData->LastPt;
		GetWordProc(&(GlobalData->CurMod));
		if(GlobalData->CurMod.BeginPos < 0)
			GlobalData->CurMod.BeginPos = 0;
		GlobalData->CurMod.IgnoreScanModifierKey = FALSE;
		NotifyStarDictNewScanWord(MOUSEOVER_INTERVAL);
	}
}

void CALLBACK TimerFunc(HWND hWnd,UINT nMsg,UINT nTimerid,DWORD dwTime)
{
	if (WaitForSingleObject(hSynhroMutex, 0) == WAIT_OBJECT_0) {
		if (GlobalData->TimerID) {
			if (KillTimer(0, GlobalData->TimerID))
				GlobalData->TimerID=0;
		}
		ReleaseMutex(hSynhroMutex);
	}
	if ((GlobalData->LastWND!=0)&&(GlobalData->LastWND == WindowFromPoint(GlobalData->LastPt))) {
		if (WaitForSingleObject(hSynhroMutex, 0) == WAIT_OBJECT_0) {
			SendWordToServer();
			ReleaseMutex(hSynhroMutex);
		}
	}
}

LRESULT CALLBACK MouseHookProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	if ((nCode == HC_ACTION) && ((wParam == WM_MOUSEMOVE) || (wParam == WM_NCMOUSEMOVE))) {
		if (WaitForSingleObject(hSynhroMutex, 0) == WAIT_OBJECT_0) {
			HWND WND;
			TCHAR wClassName[64];

			if (GlobalData->TimerID) {
				if (KillTimer(0, GlobalData->TimerID))
					GlobalData->TimerID=0;
			}
			WND = WindowFromPoint(((PMOUSEHOOKSTRUCT)lParam)->pt);
			
			if (GetClassName(WND, wClassName, sizeof(wClassName) / sizeof(TCHAR))) {
					const TCHAR* DisableClasses[] = {
						TEXT("gdkWindowChild"),
						TEXT("gdkWindowTemp"),
					};
					int i;
					for (i=0; i<2; i++) {
						if (StrCmp(wClassName, DisableClasses[i])==0)
							break;
					}
					if (i<2) {
						ReleaseMutex(hSynhroMutex);
						return CallNextHookEx(GlobalData->g_hHookMouse, nCode, wParam, lParam);
					}
			}
			GlobalData->TimerID = SetTimer(0, 0, MOUSEOVER_INTERVAL, TimerFunc);
			GlobalData->LastWND = WND;
			GlobalData->LastPt = ((PMOUSEHOOKSTRUCT)lParam)->pt;
			ReleaseMutex(hSynhroMutex);
		}
	}
	return CallNextHookEx(GlobalData->g_hHookMouse, nCode, wParam, lParam);
}

DLLIMPORT void ActivateTextOutSpying (int Activate)
{
	// After call SetWindowsHookEx(), when you move mouse to a application's window, 
	// this dll will load into this application automatically. And it is unloaded 
	// after call UnhookWindowsHookEx().
	if (Activate) {
		if (GlobalData->g_hHookMouse != NULL) return;
		GlobalData->g_hHookMouse = SetWindowsHookEx(WH_MOUSE, MouseHookProc, g_hInstance, 0);
	}
	else {
		if (GlobalData->g_hHookMouse == NULL) return;
		if (WaitForSingleObject(hSynhroMutex, 0) == WAIT_OBJECT_0) {
			if (GlobalData->TimerID) {
				if (KillTimer(0, GlobalData->TimerID))
					GlobalData->TimerID=0;
			}
			ReleaseMutex(hSynhroMutex);
		}
		UnhookWindowsHookEx(GlobalData->g_hHookMouse);
		GlobalData->g_hHookMouse = NULL;
	}
}


BOOL APIENTRY DllMain (HINSTANCE hInst     /* Library instance handle. */ ,
                       DWORD reason        /* Reason this function is being called. */ ,
                       LPVOID reserved     /* Not used. */ )
{
	switch (reason)
	{
	case DLL_PROCESS_ATTACH:
		g_hInstance = hInst;
		hSynhroMutex = CreateMutex(NULL, FALSE, TEXT("StarDictTextOutSpyMutex"));
		ThTypes_Init();
		break;

	case DLL_PROCESS_DETACH:
		WaitForSingleObject(hSynhroMutex, INFINITE);
		if (GlobalData->TimerID) {
			if (KillTimer(0, GlobalData->TimerID))
				GlobalData->TimerID=0;
		}
		ReleaseMutex(hSynhroMutex);
		CloseHandle(hSynhroMutex);
		{
			MSG msg;
			while (PeekMessage (&msg, 0, WM_TIMER, WM_TIMER, PM_REMOVE))
				;
		}
		if ((hGetWordLib != 0)&&(hGetWordLib != (HINSTANCE)(-1))) {
			FreeLibrary(hGetWordLib);
		}
		Thtypes_End();
		break;

	case DLL_THREAD_ATTACH:
		break;

	case DLL_THREAD_DETACH:
		break;
	}

	/* Returns TRUE on success, FALSE on failure */
	return TRUE;
}
