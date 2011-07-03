/* 
 * Copyright (C) 2006 Hu Zheng <huzheng_001@163.com>
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

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif
#include <shlwapi.h>

#include "../stardict.h"

#include "mouseover.h"
#include "InterProcessCommunication.h"

//#define DEBUG

// StarDict's Mouseover feature get the example delphi source code from Mueller Electronic Dictionary.
// Homepage: http://vertal1.narod.ru/mueldic.html E-mail: svv_soft@mail.ru

void Mouseover::NeedSpyDll()
{
	if (fSpyDLL == 0) {
		// Notice, the path must be absolute!
		std::string path = build_path(conf_dirs->get_dll_dir(), "TextOutSpy.dll");
		std::string path_utf8;
		std_win_string path_win;
		if(file_name_to_utf8(path, path_utf8) && utf8_to_windows(path_utf8, path_win)) {
			fSpyDLL = LoadLibrary(path_win.c_str());
			if (fSpyDLL==0) {
				g_warning("Unable to load TextOutSpy.dll");
				fSpyDLL = (HINSTANCE)-1;
			} else {
				ActivateSpy_func = (ActivateSpy_func_t)GetProcAddress(fSpyDLL, "ActivateTextOutSpying");
				if(!ActivateSpy_func)
					g_warning("Unable to find an entry point in TextOutSpy.dll");
			}
		} else
			fSpyDLL = (HINSTANCE)-1;
	}
}

HWND Mouseover::Create_hiddenwin()
{
	WNDCLASSEX wcex;
	const TCHAR wname[] = TEXT("StarDictMouseover");

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style	        = 0;
	wcex.lpfnWndProc	= (WNDPROC)mouseover_mainmsg_handler;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= stardictexe_hInstance;
	wcex.hIcon			= NULL;
	wcex.hCursor		= NULL,
	wcex.hbrBackground	= NULL;
	wcex.lpszMenuName	= NULL;
	wcex.lpszClassName	= wname;
	wcex.hIconSm		= NULL;

	RegisterClassEx(&wcex);

	return CreateWindow(wname, TEXT(""), 0, 0, 0, 0, 0, GetDesktopWindow(), NULL,
		stardictexe_hInstance, 0);
}

void Mouseover::ShowTranslation()
{
	char MatchedWord[MAX_SCAN_TEXT_SIZE];
	/* GlobalData->CurMod is shared by multiple processes, copy data ASAP. */
	strcpy(MatchedWord, GlobalData->CurMod.MatchedWord);
	int BeginPos = GlobalData->CurMod.BeginPos;
	bool IgnoreScanModifierKey = static_cast<bool>(GlobalData->CurMod.IgnoreScanModifierKey);
#ifdef DEBUG
	{
		const char * utf8_marker = (
			g_utf8_validate(MatchedWord, -1, NULL) ? "" : "!!!"
		);
		std::string buf;
		if(BeginPos >= 0) {
			buf.assign(MatchedWord, BeginPos);
			buf += "[";
			const char *p = MatchedWord + BeginPos;
			const char *q = g_utf8_next_char(p);
			buf.append(p, q-p);
			buf += "]";
			buf.append(q);
		} else
			buf = MatchedWord;
		g_debug("ShowTranslation: %s (%d) %s", utf8_marker,
			BeginPos, buf.c_str());
	}
#endif
	if (!conf->get_bool_at("dictionary/scan_selection")) // Needed by acrobat plugin.
		return;
	if (conf->get_bool_at("dictionary/only_scan_while_modifier_key")
		&& !IgnoreScanModifierKey) {
		bool do_scan = gpAppFrame->unlock_keys->is_pressed();
		if (!do_scan)
			return;
	}
	if (g_utf8_validate(MatchedWord, -1, NULL)) {
		if(BeginPos >= 0)
			gpAppFrame->SmartLookupToFloat(MatchedWord, BeginPos, IgnoreScanModifierKey);
		else
			gpAppFrame->SimpleLookupToFloat(MatchedWord, IgnoreScanModifierKey);
	} else {
		g_warning("ShowTranslation: incorrect encoding of the word!");
		if(BeginPos >= 0) {
			glib::CharStr str1(g_locale_to_utf8(MatchedWord, BeginPos, NULL, NULL, NULL));
			if (!str1)
				return;
			glib::CharStr str2(g_locale_to_utf8(MatchedWord + BeginPos, -1, NULL, NULL, NULL));
			if (!str2)
				return;
			BeginPos = strlen(get_impl(str1));
			glib::CharStr str(g_strdup_printf("%s%s", get_impl(str1), get_impl(str2)));
			gpAppFrame->SmartLookupToFloat(get_impl(str), BeginPos, IgnoreScanModifierKey);
		} else {
			glib::CharStr str(g_locale_to_utf8(MatchedWord, -1, NULL, NULL, NULL));
			if (!str)
				return;
			gpAppFrame->SimpleLookupToFloat(get_impl(str), IgnoreScanModifierKey);
		}
	}
}

LRESULT CALLBACK Mouseover::mouseover_mainmsg_handler(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	switch (msg) {
		case WM_STARDICT_SHOW_TRANSLATION:
			ShowTranslation();
			break;
		default:
			/*nothing*/;
	}

	return DefWindowProc(hwnd, msg, wparam, lparam);
}

Mouseover::Mouseover()
{
	fSpyDLL = 0;
	ActivateSpy_func = NULL;
}

void Mouseover::Init()
{
	ThTypes_Init();
	ZeroMemory(GlobalData, sizeof(TGlobalDLLData));
	std::string path = build_path(conf_dirs->get_dll_dir(), "TextOutHook.dll");
	std::string path_utf8;
	std_win_string path_win;
	file_name_to_utf8(path, path_utf8);
	utf8_to_windows(path_utf8, path_win);
	StrCpy(GlobalData->LibName, path_win.c_str());
	GlobalData->ServerWND = Create_hiddenwin();
}

void Mouseover::End()
{
	if ((fSpyDLL!=0)&&(fSpyDLL!=(HINSTANCE)-1)) {
		stop();
		FreeLibrary(fSpyDLL);
	}
	fSpyDLL = NULL;
	ActivateSpy_func = NULL;
	DestroyWindow(GlobalData->ServerWND);
	GlobalData->ServerWND = NULL;
	Thtypes_End();
}

void Mouseover::start()
{
	NeedSpyDll();
	if (ActivateSpy_func)
		ActivateSpy_func(true);
}

void Mouseover::stop()
{
	if (ActivateSpy_func)
		ActivateSpy_func(false);
}
