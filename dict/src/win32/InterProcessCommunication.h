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

#ifndef _InterProcessCommunication_H_
#define _InterProcessCommunication_H_

#include <windows.h>

#ifdef __cplusplus
extern "C"
{
#endif				/* __cplusplus */

#define WM_STARDICT_SHOW_TRANSLATION (WM_USER + 300)

/* Maximum size of scanned text: strlen(text) < STARDICT_MAX_INDEX_KEY_SIZE. */
#define MAX_SCAN_TEXT_SIZE 256

typedef struct TCurrentMode {
	HWND WND;
	POINT Pt;
	/* in utf-8 */
	char MatchedWord[MAX_SCAN_TEXT_SIZE];
	/* MatchedWord may contain arbitrary text that was extracted.
	BeginPos specifies position in that text.
	It points to the first byte of the character the mouse was over. 
	StarDict should extract the word under the pointer. 
	If mouse position is irrelevant and complete MatchedWord should be looked up as whole,
	set BeginPos to -1. */
	int BeginPos;
	/* StarDict may be configured to scan only when a specified modifier key is being pressed,
	Ctrl, for example. This is useful for mouse-hover scanning.
	Modifier key is unnecessary for Acrobat plugin. 
	This flag allows to specify whether the modifier key should be taken into account or not. */
	int IgnoreScanModifierKey;
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
/* Notify StarDict main application that a new word was scanned. 
Parameters:
	timeout - how long to way for the message to be processed (in milliseconds). */
void NotifyStarDictNewScanWord(UINT timeout);

#ifdef __cplusplus
}
#endif				/* __cplusplus */

#endif
