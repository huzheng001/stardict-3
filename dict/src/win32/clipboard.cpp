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

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <glib/gi18n.h>

#include "../stardict.h"
#include "clipboard.h"


// Notice: once you changed this file, try to change src/selection.cpp too.

Clipboard::Clipboard()
{
	IsBusy = 0;
	timeout = 0;
	clipboard = NULL;
}

void Clipboard::Init()
{
	clipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
}

void Clipboard::End()
{
	stop();
}

void Clipboard::start()
{
	if (!timeout)
		timeout = g_timeout_add(CLIPBOARD_INTERVAL,TimeOutCallback,this);
}

void Clipboard::stop()
{
	if (timeout) {
		g_source_remove(timeout);
		timeout = 0;
	}
	LastClipWord.clear();
}

gint Clipboard::TimeOutCallback(gpointer data)
{
	Clipboard *oClipboard = (Clipboard *)data;
	if (oClipboard->IsBusy) {
		oClipboard->IsBusy++;
		if (oClipboard->IsBusy*CLIPBOARD_INTERVAL > 8000) {
			oClipboard->IsBusy = 0;
		}
	} else {
		oClipboard->IsBusy = 1;
		gtk_clipboard_request_text(oClipboard->clipboard, ClipboardReceivedCallback, oClipboard);
	}
	return true;
}

void Clipboard::ClipboardReceivedCallback(GtkClipboard *clipboard, const gchar *text, gpointer data)
{
	Clipboard *oClipboard = (Clipboard *)data;
	oClipboard->IsBusy = 0;
	if (text) {
		gchar *a = g_strdup(text);
		oClipboard->ClipboardReceived(a);
		g_free(a);
	}
	else {
		oClipboard->IsBusy = 0;
		oClipboard->LastClipWord.clear();
	}
}

/* sToken in utf-8 encoding. */
void Clipboard::ClipboardReceived(gchar* sToken)
{
	gchar *a, *a2;
	while (g_ascii_isspace(*sToken))
		sToken++;
	a=strchr(sToken,'\n');
	if (a)
		*a='\0';
	if(sToken[0] == '\0') {
		LastClipWord.clear();
		return;
	}

	if (g_str_has_prefix(sToken,"http://") || g_str_has_prefix(sToken,"https://") || g_str_has_prefix(sToken,"ftp://")) {
		LastClipWord.clear();
		return;
	}

	a = sToken;
	while(*a) {
		a2 = g_utf8_next_char(a);
		if(a2 < sToken + MAX_INDEX_KEY_SIZE)
			a = a2;
		else
			break;
	}
	*a = '\0';

	if (LastClipWord != sToken)
	{
		LastClipWord = sToken;
		gpAppFrame->SimpleLookupToFloat(sToken);
	}
}
