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

#ifndef _DESKTOP_HPP_
#define _DESKTOP_HPP_

#include <glib.h>
#include <string>
#include <gtk/gtk.h>

extern void play_sound_file(const std::string& filename);
extern void play_video_file(const std::string& filename);
extern void show_help(const gchar *section);
/* url in utf-8 encoding */
extern void show_url(const char *url);
extern void play_sound_on_event(const gchar *eventname);
extern bool confirm_enable_network_dicts(GtkWidget *parent_window = NULL);

#endif
