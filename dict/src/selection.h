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

#ifndef __SD_SELECTION_H__
#define __SD_SELECTION_H__

#include <gtk/gtk.h>
#include <string>

const int SELECTION_INTERVAL=300; 		    // check selection interval.

class Selection {
private:
	gint IsBusy;
	gint timeout;

	void create_selection_widget();

	static gint TimeOutCallback(gpointer data);
	static void SelectionReceivedCallback(GtkWidget* widget,GtkSelectionData *selection_data, guint time, Selection *oSelection);
	void SelectionReceived(gchar* sValue);
	gboolean Enable();
public:
	GdkAtom UTF8_STRING_Atom, COMPOUND_TEXT_Atom; //need to set it to static? this make oTextWin can't use it conveniently.
	std::string LastClipWord;
	GtkWidget* selection_widget;

	Selection();
	void Init();
	void End();
	void start();
	void stop();
};

#endif
