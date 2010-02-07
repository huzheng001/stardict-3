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
#  include <windows.h>
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
	std::string filename_utf8;
	std::win_string filename_win;
	if(file_name_to_utf8(filename, filename_utf8) 
		&& utf8_to_windows(filename_utf8, filename_win)) {
		/* for playing wav files only:
		PlaySound(filename_win.c_str(), 0, SND_ASYNC | SND_FILENAME);

		mciSendString does not play ogg files.
		The function returns 0, but no sound produced.
		DirectShow based decoder from http://www.vorbis.com/ installed.
		Test environment: Windows XP sp3. */
		g_debug("play sound %s", filename_utf8.c_str());
		bool done=false;
		if(!conf->get_bool_at("dictionary/always_use_sound_play_command")) {
			MCIERROR mcierr;
			std::win_string cmd;
			if((mcierr = mciSendString(TEXT("close all"), NULL, 0, NULL)))
				goto mci_error;
			cmd = TEXT("open \"");
			cmd += filename_win;
			cmd += TEXT("\" alias stardict_sound");
			if((mcierr = mciSendString(cmd.c_str(), NULL, 0, NULL)))
				goto mci_error;
			if((mcierr = mciSendString(TEXT("play stardict_sound"), NULL, 0, NULL)))
				goto mci_error;
			done = true;
			goto mci_end;
mci_error:
			g_warning("Play sound command failed.");
mci_end:
			;
		}
		if(!done) {
			const std::string &playcmd=
			conf->get_string_at("dictionary/sound_play_command");
			spawn_command(playcmd.c_str(), filename_utf8.c_str());
		}
	}
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
	std::string datadir_utf8;
	if(!file_name_to_utf8(gStarDictDataDir, datadir_utf8))
		return ;
	// You may translate it as "%s\\help\\stardict-zh_CN.chm" when this file available.
	glib::CharStr filename_utf8(
		g_strdup_printf(_("%s\\help\\stardict.chm"), datadir_utf8.c_str()));
	std::win_string filename_win;
	if(!utf8_to_windows(get_impl(filename_utf8), filename_win))
		return ;
	ShellExecute((HWND)(GDK_WINDOW_HWND(gpAppFrame->window->window)), 
		TEXT("OPEN"), filename_win.c_str(), NULL, NULL, SW_SHOWNORMAL);
#elif defined(CONFIG_GNOME)
	gnome_help_display ("stardict.xml", section, NULL);
#endif
}

void show_url(const char *url)
{
	if (!url)
		return;
#ifdef _WIN32
	std::win_string url_win;
	if(!utf8_to_windows(url, url_win))
		return ;
	ShellExecute((HWND)(GDK_WINDOW_HWND(gpAppFrame->window->window)),
		TEXT("OPEN"), url_win.c_str(), NULL, NULL, SW_SHOWNORMAL);
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
