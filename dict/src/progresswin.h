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

#ifndef _PROGRESSWIN_HPP_
#define _PROGRESSWIN_HPP_

#include <string>

class progress_win {
public:
	explicit progress_win(GtkWindow *parent_win);
	~progress_win();
	void display_action(const std::string& actname);
private:
	static gboolean vButtonPressCallback (GtkWidget * widget, GdkEventButton * event , progress_win *oWin);
	static gboolean vMotionNotifyCallback (GtkWidget * widget, GdkEventMotion * event , progress_win *oWin);
	GtkLabel *text;
	GtkProgressBar *progress;
	GtkWidget *win;
	gint press_x_root, press_y_root, press_window_x, press_window_y;
};

#endif//!_PROGRESSWIN_HPP_
