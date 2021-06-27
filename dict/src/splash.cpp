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

#include "conf.h"
#include "lib/utils.h"
#include "libcommon.h"

#include "splash.h"

splash_screen stardict_splash;

// this is the callback which will destroy the splash screen window.
void splash_screen::on_mainwin_finish()
{
	gtk_widget_destroy(window);
}

splash_screen::splash_screen()
:
	window(NULL),
	text(NULL),
	progress(NULL)
{
}

void splash_screen::display_action(const std::string& actname)
{
	if ( NULL == text )
		return;
	gtk_label_set_text(text, actname.c_str());
	gtk_progress_bar_pulse(progress);
	while(gtk_events_pending())
		gtk_main_iteration();
}

void splash_screen::show()
{
	gtk_window_set_auto_startup_notification(FALSE);

//This function never failed
	GtkWidget *image =
		gtk_image_new_from_file(build_path(conf_dirs->get_data_dir(),
					"pixmaps" G_DIR_SEPARATOR_S "splash.png").c_str());

	window = gtk_window_new (GTK_WINDOW_TOPLEVEL);

#if GTK_MAJOR_VERSION >= 3
	GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
#else
	GtkWidget *vbox = gtk_vbox_new(FALSE, 0);
#endif
	gtk_box_pack_start(GTK_BOX(vbox),image,false,false,0);
	text = GTK_LABEL(gtk_label_new(_("Loading")));
	int w = gdk_pixbuf_get_width(gtk_image_get_pixbuf(GTK_IMAGE(image)));
	gtk_widget_set_size_request(GTK_WIDGET(text), w, -1);
	gtk_label_set_line_wrap(text, TRUE);
	gtk_label_set_justify(text, GTK_JUSTIFY_CENTER);
	gtk_box_pack_start(GTK_BOX(vbox),GTK_WIDGET(text),false,false,0);
	progress = GTK_PROGRESS_BAR(gtk_progress_bar_new());
	gtk_widget_set_size_request(GTK_WIDGET(progress), w, 12);
	gtk_box_pack_start(GTK_BOX(vbox),GTK_WIDGET(progress),false,false,0);

	gtk_widget_show_all(vbox);
	gtk_container_add(GTK_CONTAINER(window), vbox);

	//gtk_widget_set_app_paintable(window, TRUE);
	gtk_window_set_title (GTK_WINDOW (window), _("StarDict starting..."));
	gtk_window_set_position(GTK_WINDOW (window), GTK_WIN_POS_CENTER);
	//gtk_widget_set_size_request(window, w, h);
	gtk_window_set_decorated(GTK_WINDOW(window), FALSE);
	gtk_window_set_type_hint (GTK_WINDOW(window), GDK_WINDOW_TYPE_HINT_SPLASHSCREEN);
	gtk_window_set_modal(GTK_WINDOW(window), TRUE);
	gtk_widget_add_events(window, GDK_BUTTON_PRESS_MASK | GDK_BUTTON1_MOTION_MASK);
	g_signal_connect (G_OBJECT (window), "button_press_event", G_CALLBACK (vButtonPressCallback), this);
	g_signal_connect (G_OBJECT (window), "motion_notify_event", G_CALLBACK (vMotionNotifyCallback), this);
	gtk_widget_show(window);
	// go into main loop, processing events.
	while(gtk_events_pending())
		gtk_main_iteration();

	gtk_window_set_auto_startup_notification(TRUE);
}

gboolean splash_screen::vButtonPressCallback (GtkWidget * widget, GdkEventButton * event , splash_screen *oWin)
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

gboolean splash_screen::vMotionNotifyCallback (GtkWidget * widget, GdkEventMotion * event , splash_screen *oWin)
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
		gtk_window_move(GTK_WINDOW(oWin->window), x, y);
	}

	return TRUE;
}
