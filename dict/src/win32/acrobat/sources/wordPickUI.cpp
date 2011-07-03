/*********************************************************************
 * 
 * This file part of WordPick - A Adobe Acrobat/Reader plugin for text
 * extraction by mouse click.
 * 2006 Dewolf Xue <deWolf_maTri_X@msn.com>
 * 2006 Hu Zheng <huzheng_001@163.com>
 *
 * This plugin is special for StarDict
 *
 * Compile under Acrobat SDK 7 + M$ VS 2003.
 *
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
 *
*********************************************************************/
#include <Shlwapi.h>
#include "PIHeaders.h"
#include "PIRequir.h"
#include "PIMain.h" // for gHINSTANCE
#include "wordPickUI.h"
#include "../../InterProcessCommunication.h"
#include "../win32/resource.h"


/*-------------------------------------------------------
	Constants/Declarations
-------------------------------------------------------*/
const size_t buffer_size = MAX_SCAN_TEXT_SIZE;
char buffer[buffer_size];
wchar_t wbuffer[buffer_size];
static bool boolPickWords = false;

static AVDocDidSetSelectionNPROTO setSelectionProc;
static AVToolButton wordPickToolButton;

// UI Callbacks
static AVExecuteProc		cbActivateTool;
static AVComputeEnabledProc	cbIsEnabled;
static AVComputeMarkedProc	cbIsMarked;

/*-------------------------------------------------------
	Utility Methods
-------------------------------------------------------*/

void *GetToolButtonIcon(void)
{
	return (AVCursor)LoadBitmap(gHINSTANCE, MAKEINTRESOURCE(IDB_BITMAP1));
}

/* Convert the string in wbuffer in UCS-2 encoding to string in buffer in UTF-8 encoding
	return value: true - success */
static bool ConvertBufferToUTF8(void)
{
	for(int wlen = wcslen(wbuffer); wlen > 0; --wlen) {
		int res = WideCharToMultiByte(
			CP_UTF8, // __in   UINT CodePage,
			0, // __in   DWORD dwFlags,
			wbuffer, // __in   LPCWSTR lpWideCharStr,
			wlen, // __in   int cchWideChar,
			buffer, //__out  LPSTR lpMultiByteStr,
			buffer_size - 1, // __in   int cbMultiByte,
			NULL, // __in   LPCSTR lpDefaultChar,
			NULL //__out  LPBOOL lpUsedDefaultChar
		);
		if(res) {
			buffer[res] = 0;
			return true;
		} else {
			DWORD dwErr = GetLastError();
			if(dwErr != ERROR_INSUFFICIENT_BUFFER && dwErr != ERROR_NO_UNICODE_TRANSLATION)
				return false;
		}
	}
	return false;
}

/*-------------------------------------------------------
	AVTool Callbacks
-------------------------------------------------------*/
static ACCB1 ASBool ACCB2 PDTextSelectEnumTextProcCB(void* procObj, PDFont font, ASFixed size, 
											   PDColorValue color, char* text, ASInt32 textLen)
{
	/* The text parameter contains text in UCS-2 encoding.
	Experimentally detected that it is in big-endian format.
	Append the text to wbuffer converting to little-endian format. */
	int text_ind = 0;
	size_t wbuffer_ind = wcslen(wbuffer);
	for(; text_ind + 1 < textLen && wbuffer_ind < buffer_size-1; text_ind += 2, wbuffer_ind++) {
		wbuffer[wbuffer_ind] = (((unsigned char)text[text_ind] << 8) | (unsigned char)text[text_ind + 1]);
	}
	wbuffer[wbuffer_ind] = 0;
	return true;
}

static ACCB1 ASBool ACCB2 IsEnabled (void *permRequired)
{
	AVDoc avDoc = AVAppGetActiveDoc();
	if (!avDoc)
		return false;
	else
	{
		PDPerms docPerms = PDDocGetPermissions(AVDocGetPDDoc(avDoc));
		PDPerms reqPerms = (PDPerms)permRequired;
		return ((reqPerms & docPerms) == reqPerms);
	}
}

static ACCB1 ASBool ACCB2 IsMarked (void *clientData)
{
	return boolPickWords;
}

static ACCB1 void ACCB2 ActivateWordPickTool(void *clientData)
{
	boolPickWords = !boolPickWords;
}

static ACCB1 void ACCB2 wordPickAVDocDidSetSelection(
	AVDoc doc, ASAtom selType, void * selData, void * clientData)
{
	if(!boolPickWords)
		return;
	if(selType != ASAtomFromString("Text"))
		return;
	wbuffer[0] = 0;
	PDTextSelect Text = static_cast<PDTextSelect>(selData);
	/* NOTE
	Acrobat enumerates text in the order it appears in the PDF file, 
	which is often not the same as the order in which a person would read the text. */
	PDTextSelectEnumTextProc enumProc 
		= ASCallbackCreateProto(PDTextSelectEnumTextProc, &PDTextSelectEnumTextProcCB);
	DURING
		PDTextSelectEnumTextUCS(Text, enumProc, NULL);
	HANDLER
		ASCallbackDestroy(enumProc);
		return;
	END_HANDLER
	ASCallbackDestroy(enumProc);
	if(!ConvertBufferToUTF8())
		return;
	GlobalData->CurMod.BeginPos = -1;
	strcpy_s(GlobalData->CurMod.MatchedWord, MAX_SCAN_TEXT_SIZE, buffer);
	GlobalData->CurMod.IgnoreScanModifierKey = true;
	NotifyStarDictNewScanWord(1000);
}

static void SetUpToolButton(void)
{
	void *WordPickIcon = GetToolButtonIcon();
	AVToolBar toolBar = AVAppGetToolBar();
	AVToolButton separator = AVToolBarGetButtonByName (toolBar, ASAtomFromString("Hand"));

	wordPickToolButton = AVToolButtonNew (ASAtomFromString("ADBE:WordPick"), WordPickIcon, true, false);
	cbActivateTool = ASCallbackCreateProto(AVExecuteProc, &ActivateWordPickTool);
	AVToolButtonSetExecuteProc (wordPickToolButton, cbActivateTool, NULL);
	/* pdPermCopy - permissions required to extract text from a document.
	In Document Properties this property is named "Content Copy or Extration".
	To see Document Restrictions go to Main menu -> File -> Document properties -> Security tab.
	If we ignore permissions here (use 0), PDTextSelectEnumTextUCS throws an exception.
	We will not get text anyway. */
	AVToolButtonSetComputeEnabledProc (wordPickToolButton, cbIsEnabled, (void *)pdPermCopy);
	AVToolButtonSetComputeMarkedProc (wordPickToolButton, cbIsMarked, NULL);
	AVToolButtonSetHelpText (wordPickToolButton, "Toggle StarDict");

	AVToolBarAddButton(toolBar, wordPickToolButton, true, separator);
}

static void DestroyToolButton(void)
{
	AVToolButtonDestroy (wordPickToolButton);
	wordPickToolButton = NULL;
	ASCallbackDestroy(cbActivateTool);
	cbActivateTool = NULL;
}

void SetUpUI(void)
{
	setSelectionProc = ASCallbackCreateNotification(AVDocDidSetSelection, &wordPickAVDocDidSetSelection);
	AVAppRegisterNotification(AVDocDidSetSelectionNSEL, gExtensionID, setSelectionProc, NULL);

	cbIsEnabled		= ASCallbackCreateProto (AVComputeEnabledProc, &IsEnabled);
	cbIsMarked		= ASCallbackCreateProto (AVComputeMarkedProc, &IsMarked);

	if (ASGetConfiguration(ASAtomFromString("CanEdit")))
		SetUpToolButton();
}

void CleanUpUI(void)
{
	AVAppUnregisterNotification(AVDocDidSetSelectionNSEL, gExtensionID, setSelectionProc, NULL);
	ASCallbackDestroy(setSelectionProc);
	setSelectionProc = NULL;

	if(wordPickToolButton)
		DestroyToolButton();
	ASCallbackDestroy(cbIsEnabled);
	cbIsEnabled = NULL;
	ASCallbackDestroy(cbIsMarked);
	cbIsMarked = NULL;
}
