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

#ifndef __SD_SPLASH_H__
#define __SD_SPLASH_H__

#include <string>

#include <gtk/gtk.h>

class splash_screen {
public:
	splash_screen();
	void display_action(const std::string& actname);
	void show();
	void on_mainwin_finish();
private:
	static gboolean vButtonPressCallback (GtkWidget * widget, GdkEventButton * event , splash_screen *oWin);
	static gboolean vMotionNotifyCallback (GtkWidget * widget, GdkEventMotion * event , splash_screen *oWin);
	GtkWidget *window;
	GtkLabel *text;
	GtkProgressBar *progress;
	gint press_x_root, press_y_root, press_window_x, press_window_y;
};

extern splash_screen stardict_splash;

#endif
