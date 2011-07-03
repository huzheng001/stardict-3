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

#include "../stardict.h"
#include "hotkey.h"

struct Entry {
	bool registered;
	int id;
	HotkeyHandler handler;
	UINT modifiers, key;
} *hotkeylist;
int HotkeyCount;
HWND ServerWND;
HWND Create_hiddenwin();
static LRESULT CALLBACK hotkey_mainmsg_handler(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

HWND Create_hiddenwin()
{
	WNDCLASSEX wcex;
	TCHAR wname[32] = TEXT("StarDictHotkey");

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style	        = 0;
	wcex.lpfnWndProc	= (WNDPROC)hotkey_mainmsg_handler;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= stardictexe_hInstance;
	wcex.hIcon		= NULL;
	wcex.hCursor		= NULL,
	wcex.hbrBackground	= NULL;
	wcex.lpszMenuName	= NULL;
	wcex.lpszClassName	= wname;
	wcex.hIconSm		= NULL;

	RegisterClassEx(&wcex);

	// Create the window
	return CreateWindow(wname, TEXT(""), 0, 0, 0, 0, 0, GetDesktopWindow(), NULL,
		stardictexe_hInstance, 0);
}

void hotkey_gdk2winapi(guint gdk_key, guint gdk_modifiers, UINT &winapi_key, UINT &winapi_modifiers)
{
	winapi_modifiers = 0;
	if (gdk_modifiers & GDK_SHIFT_MASK)
		winapi_modifiers |= MOD_SHIFT;
	if (gdk_modifiers & GDK_CONTROL_MASK)
		winapi_modifiers |= MOD_CONTROL;
	if (gdk_modifiers & GDK_MOD1_MASK)
		winapi_modifiers |= MOD_ALT;
	if (gdk_modifiers & GDK_META_MASK)
		winapi_modifiers |= MOD_WIN;
	switch (gdk_key) {
	case GDK_BackSpace:
		winapi_key = VK_BACK;
		break;
	case GDK_Tab:
		winapi_key = VK_TAB;
		break;
	case GDK_Cancel:
		winapi_key = VK_CANCEL;
		break;
	case GDK_Clear:
		winapi_key = VK_CLEAR;
		break;
	case GDK_Return:
		winapi_key = VK_RETURN;
		break;
	case GDK_Pause:
		winapi_key = VK_PAUSE;
		break;
	case GDK_Escape:
		winapi_key = VK_ESCAPE;
		break;
	case GDK_space:
		winapi_key = VK_SPACE;
		break;
	case GDK_KP_Prior:
		winapi_key = VK_PRIOR;
		break;
	case GDK_KP_Next:
		winapi_key = VK_NEXT;
		break;
	case GDK_KP_End:
		winapi_key = VK_END;
		break;
	case GDK_KP_Home:
		winapi_key = VK_HOME;
		break;
	case GDK_KP_Left:
		winapi_key = VK_LEFT;
		break;
	case GDK_KP_Up:
		winapi_key = VK_UP;
		break;
	case GDK_KP_Right:
		winapi_key = VK_RIGHT;
		break;
	case GDK_KP_Down:
		winapi_key = VK_DOWN;
		break;
	case GDK_Select:
		winapi_key = VK_SELECT;
		break;
	case GDK_Print:
		winapi_key = VK_PRINT;
		break;
	case GDK_Execute:
		winapi_key = VK_EXECUTE;
		break;
	case GDK_Insert:
		winapi_key = VK_INSERT;
		break;
	case GDK_Delete:
		winapi_key = VK_DELETE;
		break;
	case GDK_Help:
		winapi_key = VK_HELP;
		break;
	case GDK_KP_0:
		winapi_key = VK_NUMPAD0;
		break;
	case GDK_KP_1:
		winapi_key = VK_NUMPAD1;
		break;
	case GDK_KP_2:
		winapi_key = VK_NUMPAD2;
		break;
	case GDK_KP_3:
		winapi_key = VK_NUMPAD3;
		break;
	case GDK_KP_4:
		winapi_key = VK_NUMPAD4;
		break;
	case GDK_KP_5:
		winapi_key = VK_NUMPAD5;
		break;
	case GDK_KP_6:
		winapi_key = VK_NUMPAD6;
		break;
	case GDK_KP_7:
		winapi_key = VK_NUMPAD7;
		break;
	case GDK_KP_8:
		winapi_key = VK_NUMPAD8;
		break;
	case GDK_KP_9:
		winapi_key = VK_NUMPAD9;
		break;
	case GDK_KP_Multiply:
		winapi_key = VK_MULTIPLY;
		break;
	case GDK_KP_Add:
		winapi_key = VK_ADD;
		break;
	case GDK_KP_Separator:
		winapi_key = VK_SEPARATOR;
		break;
	case GDK_KP_Subtract:
		winapi_key = VK_SUBTRACT;
		break;
	case GDK_KP_Decimal:
		winapi_key = VK_DECIMAL;
		break;
	case GDK_KP_Divide:
		winapi_key = VK_DIVIDE;
		break;
	case GDK_F1:
		winapi_key = VK_F1;
		break;
	case GDK_F2:
		winapi_key = VK_F2;
		break;
	case GDK_F3:
		winapi_key = VK_F3;
		break;
	case GDK_F4:
		winapi_key = VK_F4;
		break;
	case GDK_F5:
		winapi_key = VK_F5;
		break;
	case GDK_F6:
		winapi_key = VK_F6;
		break;
	case GDK_F7:
		winapi_key = VK_F7;
		break;
	case GDK_F8:
		winapi_key = VK_F8;
		break;
	case GDK_F9:
		winapi_key = VK_F9;
		break;
	case GDK_F10:
		winapi_key = VK_F10;
		break;
	case GDK_F11:
		winapi_key = VK_F11;
		break;
	case GDK_F12:
		winapi_key = VK_F12;
		break;
	case GDK_F13:
		winapi_key = VK_F13;
		break;
	case GDK_F14:
		winapi_key = VK_F14;
		break;
	case GDK_F15:
		winapi_key = VK_F15;
		break;
	case GDK_F16:
		winapi_key = VK_F16;
		break;
	case GDK_F17:
		winapi_key = VK_F17;
		break;
	case GDK_F18:
		winapi_key = VK_F18;
		break;
	case GDK_F19:
		winapi_key = VK_F19;
		break;
	case GDK_F20:
		winapi_key = VK_F20;
		break;
	case GDK_F21:
		winapi_key = VK_F21;
		break;
	case GDK_F22:
		winapi_key = VK_F22;
		break;
	case GDK_F23:
		winapi_key = VK_F23;
		break;
	case GDK_F24:
		winapi_key = VK_F24;
		break;
	case GDK_A:
	case GDK_a:
		winapi_key = 'A';
		break;
	case GDK_B:
	case GDK_b:
		winapi_key = 'B';
		break;
	case GDK_C:
	case GDK_c:
		winapi_key = 'C';
		break;
	case GDK_D:
	case GDK_d:
		winapi_key = 'D';
		break;
	case GDK_E:
	case GDK_e:
		winapi_key = 'E';
		break;
	case GDK_F:
	case GDK_f:
		winapi_key = 'F';
		break;
	case GDK_G:
	case GDK_g:
		winapi_key = 'G';
		break;
	case GDK_H:
	case GDK_h:
		winapi_key = 'H';
		break;
	case GDK_I:
	case GDK_i:
		winapi_key = 'I';
		break;
	case GDK_J:
	case GDK_j:
		winapi_key = 'J';
		break;
	case GDK_K:
	case GDK_k:
		winapi_key = 'K';
		break;
	case GDK_L:
	case GDK_l:
		winapi_key = 'L';
		break;
	case GDK_M:
	case GDK_m:
		winapi_key = 'M';
		break;
	case GDK_N:
	case GDK_n:
		winapi_key = 'N';
		break;
	case GDK_O:
	case GDK_o:
		winapi_key = 'O';
		break;
	case GDK_P:
	case GDK_p:
		winapi_key = 'P';
		break;
	case GDK_Q:
	case GDK_q:
		winapi_key = 'Q';
		break;
	case GDK_R:
	case GDK_r:
		winapi_key = 'R';
		break;
	case GDK_S:
	case GDK_s:
		winapi_key = 'S';
		break;
	case GDK_T:
	case GDK_t:
		winapi_key = 'T';
		break;
	case GDK_U:
	case GDK_u:
		winapi_key = 'U';
		break;
	case GDK_V:
	case GDK_v:
		winapi_key = 'V';
		break;
	case GDK_W:
	case GDK_w:
		winapi_key = 'W';
		break;
	case GDK_X:
	case GDK_x:
		winapi_key = 'X';
		break;
	case GDK_Y:
	case GDK_y:
		winapi_key = 'Y';
		break;
	case GDK_z:
	case GDK_Z:
		winapi_key = 'Z';
		break;
	case GDK_0:
		winapi_key = '0';
		break;
	case GDK_1:
		winapi_key = '1';
		break;
	case GDK_2:
		winapi_key = '2';
		break;
	case GDK_3:
		winapi_key = '3';
		break;
	case GDK_4:
		winapi_key = '4';
		break;
	case GDK_5:
		winapi_key = '5';
		break;
	case GDK_6:
		winapi_key = '6';
		break;
	case GDK_7:
		winapi_key = '7';
		break;
	case GDK_8:
		winapi_key = '8';
		break;
	case GDK_9:
		winapi_key = '9';
		break;
	case GDK_grave:
		winapi_key = '~';
		break;
	case GDK_bracketleft:
		winapi_key = '[';
		break;
	case GDK_bracketright:
		winapi_key = ']';
		break;
	case GDK_backslash:
		winapi_key = '\\';
		break;
	case GDK_apostrophe:
		winapi_key = '\'';
		break;
	case GDK_comma:
		winapi_key = ',';
		break;
	case GDK_period:
		winapi_key = '.';
		break;
	case GDK_semicolon:
		winapi_key = ':';
		break;
	default:
		winapi_key = 0;
    }
}

LRESULT CALLBACK hotkey_mainmsg_handler(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	switch (msg) {
		case WM_HOTKEY:
		{
			UINT fuModifiers = (UINT) LOWORD(lparam);
			UINT uVirtKey = (UINT) HIWORD(lparam);
/*			if ((fuModifiers == (MOD_CONTROL | MOD_ALT))&&(uVirtKey == VK_F1)) {
				ToggleScan();
			}
			else if ((fuModifiers == (MOD_CONTROL | MOD_ALT))&&(uVirtKey == 'Z')) {
				ShowMainwindow();
			}*/
			for (int i = 0; i < HotkeyCount; i++)
				if (hotkeylist[i].registered &&(hotkeylist[i].handler != NULL)&&
				   (fuModifiers == hotkeylist[i].modifiers)&&(uVirtKey == hotkeylist[i].key)) {
					(hotkeylist[i].handler)();
					break;
				}
			break;
		}
		default:
			/*nothing*/;
	}

	return DefWindowProc(hwnd, msg, wparam, lparam);
}

void win32hotkey_Init(int nhotkey)
{
	HotkeyCount = nhotkey;
	hotkeylist = new Entry[HotkeyCount];
	const size_t buf_size = 100;
	TCHAR AtomName[buf_size];
	for (int i = 0; i < HotkeyCount; i++) {
		hotkeylist[i].registered = false;
#ifdef UNICODE
#  ifdef _MSC_VER
		swprintf(AtomName, buf_size, TEXT("StarDictHotKey%d"), i+1);
#  else
		swprintf(AtomName, TEXT("StarDictHotKey%d"), i+1);
#  endif
#else
		sprintf(AtomName, TEXT("StarDictHotKey%d"), i+1);
#endif
		hotkeylist[i].id = GlobalAddAtom(AtomName);
		hotkeylist[i].handler = NULL;
	}
	ServerWND = Create_hiddenwin();
}

void win32hotkey_End()
{
	for (int i = 0; i < HotkeyCount; i++) {
		win32hotkey_disable(i);
		GlobalDeleteAtom(hotkeylist[i].id);
	}
	delete[] hotkeylist;
	DestroyWindow(ServerWND);
}

void win32hotkey_enable(int index, HotkeyHandler handler, guint key, GdkModifierType modifiers)
{
	UINT wkey, wmodifiers;
	hotkey_gdk2winapi(key, modifiers, wkey, wmodifiers);
	if (wkey == 0)
		return;
	if (!hotkeylist[index].registered) {
		hotkeylist[index].registered =
			RegisterHotKey(ServerWND, hotkeylist[index].id - 0xC000, wmodifiers, wkey);
		if (hotkeylist[index].registered) {
			hotkeylist[index].handler = handler;
			hotkeylist[index].key = wkey;
			hotkeylist[index].modifiers = wmodifiers;
		}
	}
}

void win32hotkey_disable(int index)
{
	if (hotkeylist[index].registered) {
		UnregisterHotKey(ServerWND, hotkeylist[index].id - 0xC000);
		hotkeylist[index].registered = false;
		hotkeylist[index].handler = NULL;
	}
}
