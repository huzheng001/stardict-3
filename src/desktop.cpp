/* 
 * This file part of StarDict - A international dictionary for GNOME.
 * http://stardict.sourceforge.net
 * Copyright (C) 2006 Evgeniy <dushistov@mail.ru>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <glib/gi18n.h>
#include <cstdlib>

#ifdef CONFIG_GNOME
#  include <libgnome/libgnome.h>
#  include <libgnomeui/libgnomeui.h>
#elif defined(_WIN32)
#  include <gdk/gdkwin32.h>
#endif

#include "conf.h"
#include "stardict.h"

#include "desktop.hpp"

static void spawn_command(const gchar *exe, const gchar *arg)
{
  gchar *qarg = g_shell_quote(arg);
  gchar *cmd=g_strdup_printf("%s %s", exe, qarg);
  g_free(qarg);
  g_spawn_command_line_async(cmd, NULL);
  g_free(cmd);
}

void play_sound_file(const std::string& filename)
{
#ifdef _WIN32
//TODO: more good solution?
#if !defined(_MSC_VER)
	PlaySound(filename.c_str(), 0, SND_ASYNC | SND_FILENAME);
#endif
#elif defined(CONFIG_GNOME)
	gnome_sound_play(filename.c_str());
#else
	const std::string &playcmd=
		conf->get_string_at("dictionary/sound_play_command");
	spawn_command(playcmd.c_str(), filename.c_str());
#endif
}

void play_video_file(const std::string& filename)
{
	const std::string &playcmd=
		conf->get_string_at("dictionary/video_play_command");
	spawn_command(playcmd.c_str(), filename.c_str());
}

void show_help(const gchar *section)
{
#ifdef _WIN32
  // You may translate it as "%s\\help\\stardict-zh_CN.chm" when this file available.
  gchar *filename = g_strdup_printf(_("%s\\help\\stardict.chm"), gStarDictDataDir.c_str());
  ShellExecute((HWND)(GDK_WINDOW_HWND(gpAppFrame->window->window)), "OPEN", filename, NULL, NULL, SW_SHOWNORMAL);
  g_free(filename);
#elif defined(CONFIG_GNOME)
  gnome_help_display ("stardict.xml", section, NULL);
#endif
}

void show_url(const char *url)
{
	if (!url)
		return;
#ifdef _WIN32
	ShellExecute((HWND)(GDK_WINDOW_HWND(gpAppFrame->window->window)), "OPEN", url, NULL, NULL, SW_SHOWNORMAL);
#elif defined(CONFIG_GNOME)
	gnome_url_show(url, NULL);
#elif defined(CONFIG_GPE)
	gchar *command = g_strdup_printf("gpe-mini-browser %s", url);
	system(command);
	g_free(command);
#else
	spawn_command("firefox", url);
#endif
}

void play_sound_on_event(const gchar *eventname)
{
	if (conf->get_bool_at("dictionary/enable_sound_event"))
	  play_sound_file(gStarDictDataDir+ G_DIR_SEPARATOR_S "sounds" G_DIR_SEPARATOR_S +eventname+".wav");
}
