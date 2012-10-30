/*
 * Copyright (C) 2006 Evgeniy <dushistov@mail.ru>
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

#include <gtk/gtk.h>
#include <glib/gi18n.h>

#include "lib/utils.h"

#include "progresswin.h"

progress_win::progress_win(GtkWindow *parent_win)
{
	win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_widget_add_events(win, GDK_BUTTON_PRESS_MASK | GDK_BUTTON1_MOTION_MASK);
	g_signal_connect (G_OBJECT (win), "button_press_event", G_CALLBACK (vButtonPressCallback), this);
	g_signal_connect (G_OBJECT (win), "motion_notify_event", G_CALLBACK (vMotionNotifyCallback), this);
	gtk_window_set_decorated(GTK_WINDOW(win), FALSE);
	gtk_window_set_modal(GTK_WINDOW(win), TRUE);
	//gtk_window_set_keep_above(GTK_WINDOW(win), TRUE);
	if(parent_win)
		gtk_window_set_transient_for(GTK_WINDOW(win), parent_win);
	gtk_window_set_title(GTK_WINDOW(win), _("Loading"));
	gtk_window_set_position(GTK_WINDOW(win), parent_win ? GTK_WIN_POS_CENTER_ON_PARENT : GTK_WIN_POS_CENTER);
#if GTK_MAJOR_VERSION >= 3
	GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
#else
	GtkWidget *vbox = gtk_vbox_new(FALSE, 0);
#endif
	gtk_container_add(GTK_CONTAINER(win), vbox);
	text = GTK_LABEL(gtk_label_new(_("Loading")));
	gtk_box_pack_start(GTK_BOX(vbox),GTK_WIDGET(text),false,false,0);
	progress = GTK_PROGRESS_BAR(gtk_progress_bar_new());
	gtk_box_pack_start(GTK_BOX(vbox),GTK_WIDGET(progress),false,false,0);
	gtk_widget_show_all(win);
	ProcessGtkEvent();
}

progress_win::~progress_win(void)
{
	gtk_widget_destroy(win);
}

void progress_win::display_action(const std::string& actname)
{
	gtk_label_set_text(text, actname.c_str());
	gtk_progress_bar_pulse(progress);
	ProcessGtkEvent();
}

gboolean progress_win::vButtonPressCallback (GtkWidget * widget, GdkEventButton * event , progress_win *oWin)
{
	if (event->type == GDK_BUTTON_PRESS && event->button == 1
		/* check that this event is not redirected due to a grab enforced by a modal window */
		&& widget == gtk_get_event_widget((GdkEvent*)event)) {
		gtk_window_get_position(GTK_WINDOW(widget),&(oWin->press_window_x),&(oWin->press_window_y));
		oWin->press_x_root = (gint)(event->x_root);
		oWin->press_y_root = (gint)(event->y_root);
	}
	return TRUE;
}

gboolean progress_win::vMotionNotifyCallback (GtkWidget * widget, GdkEventMotion * event , progress_win *oWin)
{
	if (event->state & GDK_BUTTON1_MASK 
		/* check that this event is not redirected due to a grab enforced by a modal window */
		&& widget == gtk_get_event_widget((GdkEvent*)event)) {
		gint x,y;
		x = oWin->press_window_x + (gint)(event->x_root) - oWin->press_x_root;
		y = oWin->press_window_y + (gint)(event->y_root) - oWin->press_y_root;
		if (x<0)
			x = 0;
		if (y<0)
			y = 0;
		gtk_window_move(GTK_WINDOW(oWin->win), x, y);
	}

	return TRUE;
}
