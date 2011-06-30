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
