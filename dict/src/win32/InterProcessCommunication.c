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

#include "InterProcessCommunication.h"

HANDLE MMFHandle = 0;
TGlobalDLLData *GlobalData = NULL;

void ThTypes_Init()
{
	if (!MMFHandle)
		MMFHandle = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0,
			sizeof(TGlobalDLLData), TEXT("StarDictTextOutHookSharedMem"));
	if (!GlobalData)
		GlobalData = MapViewOfFile(MMFHandle, FILE_MAP_ALL_ACCESS, 0, 0, 0);
}

void Thtypes_End()
{
	if (GlobalData) {
		UnmapViewOfFile(GlobalData);
		GlobalData = NULL;
	}
	if (MMFHandle) {
		CloseHandle(MMFHandle);
		MMFHandle = 0;
	}
}

void NotifyStarDictNewScanWord(UINT timeout)
{
	DWORD SendMsgAnswer;
	if(!GlobalData || !GlobalData->ServerWND || GlobalData->CurMod.MatchedWord[0] == '\0')
		return;
	SendMessageTimeout(GlobalData->ServerWND, WM_STARDICT_SHOW_TRANSLATION, 0, 0,
		SMTO_ABORTIFHUNG, timeout, &SendMsgAnswer);
}
