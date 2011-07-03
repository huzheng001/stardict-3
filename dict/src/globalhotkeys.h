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

#ifndef __SD_GLOBALHOTKEYS_H__
#define __SD_GLOBALHOTKEYS_H__

#include <string>

#if defined(_WIN32)
#include "win32/hotkey.h"
#endif

class GlobalHotkeys
{
private:
	std::string scan_key, mw_key;
public:
	void Init();
	void End();
	void start_scan(const char *hotkey);
	void stop_scan();
	void start_mainwindow(const char *hotkey);
	void stop_mainwindow();
};

#endif
