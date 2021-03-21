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

#include <cstring>
#include <glib/gi18n.h>

#include "stardict.h"
#include "conf.h"
#include "iskeyspressed.h"

#include "selection.h"


// Notice: once you changed this file, try to change src/win32/clipboard.cpp too.


/* 
 Is there any way to know that selection is changed?
 So we don't need to get selection text every interval milliseconds
 and compare the text to know whether selection changed.
 I tried the "selection_notify_event", but it means that some one is requesting the selection.
 if you use the "selection-clear-event", so when selection is changed,you will get this event,
 but it need you do a gtk_selection_owner_set,this make the old selection be cleared :(

 OK, there is no way the know the selection is changed...
 Is there any way to only get the selection's text with a limit length,
 but not the whole text? Stardict only need the first 256 chars....it seems no way too :(
*/

/*
GdkAtom Selection::TARGETS_Atom;
GdkAtom Selection::UTF8_STRING_Atom;
*/

Selection::Selection()
{
	UTF8_STRING_Atom = COMPOUND_TEXT_Atom = GDK_NONE;
	IsBusy = 0;
	selection_widget = NULL;
	timeout = 0;
}

/********************************************************************/
gboolean Selection::Enable()
{
	return !gpAppFrame->oTopWin.TextSelected();
}

void Selection::create_selection_widget()
{
	if (selection_widget)
		gtk_widget_destroy(selection_widget);
	selection_widget = gtk_invisible_new ();
	g_signal_connect (G_OBJECT (selection_widget), "selection_received", G_CALLBACK (SelectionReceivedCallback), this);
}


void Selection::Init()
{
	UTF8_STRING_Atom = gdk_atom_intern("UTF8_STRING",false);
	COMPOUND_TEXT_Atom = gdk_atom_intern("COMPOUND_TEXT",false);

	create_selection_widget();
}

void Selection::End()
{
	stop();
	if (selection_widget)
		gtk_widget_destroy(selection_widget);
}

void Selection::start()
{
	if (!timeout)
		timeout = g_timeout_add(SELECTION_INTERVAL,TimeOutCallback,this);
}

void Selection::stop()
{
	if (timeout) {
		g_source_remove(timeout);
		timeout = 0;
	}
	LastClipWord.clear();
}

gint Selection::TimeOutCallback(gpointer data)
{
	Selection *oSelection = (Selection *)data;
	if (oSelection->Enable()) {

		if (conf->get_bool_at("dictionary/only_scan_while_modifier_key")) {
			bool do_scan = gpAppFrame->unlock_keys->is_pressed();
			if (!do_scan)
				return true;
		}

		if (oSelection->IsBusy) {
			oSelection->IsBusy++;
			if (oSelection->IsBusy*SELECTION_INTERVAL > 8000 ) {
				/* "selection_received" signal is not received for 8 seconds.
					there encounter some error, and i find that this often take a long time 
					(several minutes) to get the signal at last,
					during this period, if you call gtk_selection_convert(),
					the "selection_received" signal will not be received also,
					and at last these signals are received at almost the same time...BAD.
				
					so here create a new selection_widget, then call gtk_selection_convert()...
					this should can throw that error selection.
					!!!:
					But it seems (i am not sure) it will make the widgets in StarDict 
					become unselectable! see BUGS.
				 */
				g_warning("Error, selection data didn't received, retrying!");
				oSelection->create_selection_widget();
				oSelection->IsBusy = 0;
			}
		} else {
			oSelection->IsBusy = 1;
			gtk_selection_convert (oSelection->selection_widget, GDK_SELECTION_PRIMARY, oSelection->UTF8_STRING_Atom, GDK_CURRENT_TIME);
		}
	}

	return true;
}

void Selection::SelectionReceivedCallback(GtkWidget* widget,GtkSelectionData *selection_data, guint time, Selection *oSelection)
{
	gchar *result;
	result = (gchar *)gtk_selection_data_get_text (selection_data);
	if (!result) {
		/* If we asked for UTF8 and didn't get it, try compound_text;
		 * if we asked for compound_text and didn't get it, try string;
		 * If we asked for anything else and didn't get it, give up.
		 */
		if (gtk_selection_data_get_target(selection_data) == oSelection->UTF8_STRING_Atom) {
			gtk_selection_convert (widget, GDK_SELECTION_PRIMARY, oSelection->COMPOUND_TEXT_Atom, GDK_CURRENT_TIME);
		} else if (gtk_selection_data_get_target(selection_data) == oSelection->COMPOUND_TEXT_Atom) {
			gtk_selection_convert (widget, GDK_SELECTION_PRIMARY, GDK_TARGET_STRING, GDK_CURRENT_TIME);
		} else {
			oSelection->IsBusy = 0;
			oSelection->LastClipWord.clear();
		}
		return;
	}
	oSelection->IsBusy = 0;

	oSelection->SelectionReceived(result);
	g_free (result);
}

/********************************************************************/
/* sToken in utf-8 encoding. */
void Selection::SelectionReceived(gchar* sToken)
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

	if (LastClipWord != sToken) {
		LastClipWord = sToken;
		gpAppFrame->SimpleLookupToFloat(sToken);
	}
}
