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
#include "GetWord.h"
#include "TextOutHook.h"

TKnownWndClass GetWindowType(HWND WND, const TCHAR* WNDClass)
{
	const TCHAR* StrKnownClasses[] = {
		TEXT("RICHEDIT20A"),
		TEXT("RICHEDIT20W"),
		TEXT("RICHEDIT"),
		TEXT("EDIT"),
		TEXT("INTERNET EXPLORER_SERVER"),
		TEXT("CONSOLEWINDOWCLASS"), // NT
		TEXT("TTYGRAB"), // 9x
		};
	TKnownWndClass KnownClasses[] = {
		kwcRichEdit,
		kwcRichEdit,
		kwcRichEdit,
		kwcMultiLineEdit,
		kwcInternetExplorer_Server,
		kwcConsole,
		kwcConsole,
	};
	int i;
	for (i=0; i<7; i++) {
		if (StrCmpI(WNDClass, StrKnownClasses[i])==0)
			break;
	}
	if (i<7) {
		if (KnownClasses[i] == kwcMultiLineEdit) {
			if ((GetWindowLong(WND, GWL_STYLE) & ES_MULTILINE) == 0)
				return kwcSingleLineEdit;
		}
		return KnownClasses[i];
	} else
		return kwcUnknown;
}

typedef struct TConsoleParams {
	HWND WND;
	POINT Pt;
	RECT ClientRect;
	TCHAR Buffer[256];
} TConsoleParams;

static int GetWordFromConsolePack(TConsoleParams *params)
{
	HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
	if (hStdOut != INVALID_HANDLE_VALUE) {
		CONSOLE_SCREEN_BUFFER_INFO csbi;
		if (GetConsoleScreenBufferInfo(hStdOut, &csbi)) {
			COORD CurPos;
			CurPos.X = csbi.srWindow.Left + (SHORT)(params->Pt.x * (csbi.srWindow.Right - csbi.srWindow.Left + 1) / params->ClientRect.right);
			CurPos.Y = csbi.srWindow.Top + (SHORT)(params->Pt.y * (csbi.srWindow.Bottom - csbi.srWindow.Top + 1) / params->ClientRect.bottom);
			if ((CurPos.X >= 0) && (CurPos.X <= csbi.dwSize.X - 1) && (CurPos.Y >= 0) && (CurPos.Y <= csbi.dwSize.Y - 1)) {
				int BegPos;
				TCHAR *Buf;

				BegPos = CurPos.X;
				CurPos.X = 0; // read all the line
				Buf = GlobalAlloc(GMEM_FIXED, (csbi.dwSize.X + 1)*sizeof(TCHAR));
				if (Buf) {
					DWORD ActualRead;
					if ((ReadConsoleOutputCharacter(hStdOut, Buf, csbi.dwSize.X, CurPos, &ActualRead))
						&& (ActualRead == csbi.dwSize.X)) {
						int WordLen;
						
						// TODO: check the first parameter
						OemToCharBuff((LPCSTR)Buf, Buf, csbi.dwSize.X);
						if (csbi.dwSize.X > 255)
							WordLen = 255;
						else
							WordLen = csbi.dwSize.X;
						StrCpyN(params->Buffer, Buf, WordLen);
						GlobalFree(Buf);
						return WordLen;
					}
				}
			}
		}
	}
	return 0;
}
static void GetWordFromConsolePackEnd() {}

static BOOL RemoteExecute(HANDLE hProcess, void *RemoteThread, size_t RemoteSize, void *Data, int DataSize, DWORD *dwReturn)
{
	void *pRemoteThread = VirtualAllocEx(hProcess, NULL, RemoteSize, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
	void *pData;
	HANDLE hThread;

	if (!pRemoteThread)
		return FALSE;
	if (!WriteProcessMemory(hProcess, pRemoteThread, RemoteThread, RemoteSize, 0)) {
		VirtualFreeEx(hProcess, pRemoteThread, RemoteSize, MEM_RELEASE);
		return FALSE;
	}
	pData = VirtualAllocEx(hProcess, NULL, DataSize, MEM_COMMIT, PAGE_READWRITE);
	if (!pData) {
		VirtualFreeEx(hProcess, pRemoteThread, RemoteSize, MEM_RELEASE);
		return FALSE;
	}
	if (!WriteProcessMemory(hProcess, pData, Data, DataSize, 0)) {
		VirtualFreeEx(hProcess, pRemoteThread, RemoteSize, MEM_RELEASE);
		VirtualFreeEx(hProcess, pData, DataSize, MEM_RELEASE);
		return FALSE;
	}
	// Bug: I don't know why the next line will fail in Windows XP, so get word from cmd.exe can't work presently.
	hThread = CreateRemoteThread(hProcess, 0, 0, (LPTHREAD_START_ROUTINE)pRemoteThread, pData, 0, 0);
	WaitForSingleObject(hThread, INFINITE);
	GetExitCodeThread(hThread, dwReturn);
	ReadProcessMemory(hProcess, pData, Data, DataSize, 0);
	VirtualFreeEx(hProcess, pRemoteThread, RemoteSize, MEM_RELEASE);
	VirtualFreeEx(hProcess, pData, DataSize, MEM_RELEASE);
	if (hThread) {
		CloseHandle(hThread);
		return TRUE;
	} else {
		return FALSE;
	}
}

/* return value in utf-8 
BUG:
On Windows XP hook dlls are not loaded into a console process (cmd.exe).
Search for "SetWindowsHookEx WH_MOUSE cmd.exe" in Internet.
This function always returns NULL. */
static char* GetWordFromConsole(HWND WND, POINT Pt, int *BeginPos)
{
	TConsoleParams *TP;
	DWORD pid;
	DWORD MaxWordSize;
	char *Result;

	TP = malloc(sizeof(TConsoleParams));
	TP->WND = WND;
	TP->Pt = Pt;
	ScreenToClient(WND, &(TP->Pt));
	GetClientRect(WND, &(TP->ClientRect));
	
	GetWindowThreadProcessId(GetParent(WND), &pid);

	if (pid != GetCurrentProcessId()) {
		HANDLE ph;
		MaxWordSize = 0;
		// The next line will fail in Win2k, but OK in Windows XP.
		ph = OpenProcess(PROCESS_CREATE_THREAD | PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ, FALSE, pid);
		if (ph) {
			if (!RemoteExecute(ph, GetWordFromConsolePack,
				(size_t)GetWordFromConsolePackEnd - (size_t)GetWordFromConsolePack, TP,
				sizeof(TConsoleParams), &MaxWordSize))
				MaxWordSize = 0;
			CloseHandle(ph);
		}
	} else {
		MaxWordSize = GetWordFromConsolePack(TP);
	}

	if (MaxWordSize > 0) {
		// TODO: fix this
		Result = NULL; // _strdup(TP->Buffer);
	} else {
		Result = NULL;
	}
	free(TP);
	return Result;
}

char* TryGetWordFromAnyWindow(TKnownWndClass WndType, HWND WND, POINT Pt, int *BeginPos)
{
	typedef char* (*GetWordFunction_t)(HWND, POINT, int*);
	const GetWordFunction_t GetWordFunction[]= {
		ExtractFromEverything,
		ExtractFromEverything,
		ExtractFromEverything,
		ExtractFromEverything,
		ExtractFromEverything,
		GetWordFromConsole,
	};
	return GetWordFunction[WndType](WND, Pt, BeginPos);
}
