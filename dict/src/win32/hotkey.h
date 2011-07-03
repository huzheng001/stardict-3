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

#ifndef __SD_HOTKEY_H__
#define __SD_HOTKEY_H__

#include <windows.h>
#include <gdk/gdktypes.h>
#include <gdk/gdkkeysyms.h>

typedef void (*HotkeyHandler)();

//class Hotkey
//{
//private:

//public:
	void win32hotkey_Init(int nhotkey);
	void win32hotkey_End();
	void win32hotkey_enable(int index, HotkeyHandler handler, guint key, GdkModifierType modifiers);
	void win32hotkey_disable(int index);
//};

#endif
