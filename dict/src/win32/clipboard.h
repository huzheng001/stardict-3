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

#ifndef __SD_CLIPBOARD_H__
#define __SD_CLIPBOARD_H__

#include <gtk/gtk.h>
#include <string>

const int CLIPBOARD_INTERVAL=300; 		    // check clipboard interval.

class Clipboard
{
private:
	gint IsBusy;
	gint timeout;
	GtkClipboard* clipboard;

	static gint TimeOutCallback(gpointer data);
	static void ClipboardReceivedCallback(GtkClipboard *clipboard, const gchar *text, gpointer data);
	void ClipboardReceived(gchar* sValue);
public:
	std::string LastClipWord;

	Clipboard();
	void Init();
	void End();
	void start();
	void stop();
	
};

#endif
