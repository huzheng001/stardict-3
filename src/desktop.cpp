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

#if ! (defined(_WIN32) || defined(CONFIG_GNOME))
const char* const gHelpDirBase = STARDICT_DATA_DIR G_DIR_SEPARATOR_S "help";

/* Guess value of current locale from value of the environment variables
   or system-dependent defaults.
   This is a simplified version of the guess_category_value function from the
   gettext package. */
static const char * guess_locale(void)
{
	const char *locale = setlocale(LC_MESSAGES, NULL);
	if (strcmp (locale, "C") == 0)
		return locale;
	const char *language = getenv ("LANGUAGE");
	if (language != NULL && language[0] != '\0')
		return language;
	return locale;
}

static std::string guess_html_help_dir(void)
{
	const char* locale_list = guess_locale();
	std::string locale;
	std::string dir;
	while(locale_list && locale_list[0]) {
		const char* t = strchr(locale_list, ':');
		if(t) {
			locale.assign(locale_list, t-locale_list);
			locale_list = t + 1;
		} else {
			locale.assign(locale_list);
			locale_list = NULL;
		}
		size_t pos = locale.find('.');
		if(pos != std::string::npos)
			locale.resize(pos);
		dir = gHelpDirBase;
		dir += G_DIR_SEPARATOR;
		dir += locale;
		if(g_file_test(dir.c_str(), G_FILE_TEST_IS_DIR))
			return dir;
		pos = locale.find('_');
		if(pos != std::string::npos)
			locale.resize(pos);
		dir = gHelpDirBase;
		dir += G_DIR_SEPARATOR;
		dir += locale;
		if(g_file_test(dir.c_str(), G_FILE_TEST_IS_DIR))
			return dir;
	}
	dir = gHelpDirBase;
	dir += G_DIR_SEPARATOR_S "C";
	if(g_file_test(dir.c_str(), G_FILE_TEST_IS_DIR))
		return dir;
	return "";
}

/* show error in modal dialog */
void show_error(const char* message)
{
	GtkWidget *message_dlg = 
	gtk_message_dialog_new(
		gpAppFrame ? GTK_WINDOW(gpAppFrame->window) : NULL,
		GtkDialogFlags(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
		GTK_MESSAGE_ERROR,
		GTK_BUTTONS_OK,
		"%s", message);

	gtk_dialog_set_default_response(GTK_DIALOG(message_dlg), GTK_RESPONSE_OK);
	gtk_window_set_resizable(GTK_WINDOW(message_dlg), FALSE);
	gtk_dialog_run(GTK_DIALOG(message_dlg));
	gtk_widget_destroy(message_dlg);
}
#endif

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
	if(!conf->get_bool_at("dictionary/always_use_sound_play_command")) {
		std::string filename_utf8;
		std_win_string filename_win;
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
			MCIERROR mcierr;
			std_win_string cmd;
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
			if(done)
				return;
			// else mci service failed, give the custom command a try
		} else
			return; // error
	}
#elif defined(CONFIG_GNOME)
	if(!conf->get_bool_at("dictionary/always_use_sound_play_command")) {
		// Fails quietly if playing is not possible.
		gnome_sound_play(filename.c_str());
		return;
	}
#endif
	const std::string &playcmd=
		conf->get_string_at("dictionary/sound_play_command");
	spawn_command(playcmd.c_str(), filename.c_str());
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
	std_win_string filename_win;
	if(!utf8_to_windows(get_impl(filename_utf8), filename_win))
		return ;
	ShellExecute((HWND)(GDK_WINDOW_HWND(gpAppFrame->window->window)), 
		TEXT("OPEN"), filename_win.c_str(), NULL, NULL, SW_SHOWNORMAL);
#elif defined(CONFIG_GNOME)
	gnome_help_display ("stardict.xml", section, NULL);
#else
	std::string dir = guess_html_help_dir();
	if(dir.empty()) {
		std::string index_file = std::string(gHelpDirBase) 
			+ G_DIR_SEPARATOR_S "C" G_DIR_SEPARATOR_S "index.html";
		glib::CharStr message(g_strdup_printf(_("Unable to find help file %s."),
			index_file.c_str()));
		show_error(get_impl(message));
	} else {
		std::string index_file = dir + G_DIR_SEPARATOR_S "index.html";
		if(g_file_test(index_file.c_str(), G_FILE_TEST_EXISTS))
			show_url(index_file.c_str());
		else {
			glib::CharStr message(g_strdup_printf(_("Unable to find help file %s."),
				index_file.c_str()));
			show_error(get_impl(message));
		}
	}
#endif
}

void show_url(const char *url)
{
	if (!url)
		return;
#ifdef _WIN32
	if(!conf->get_bool_at("dictionary/always_use_open_url_command")) {
		std_win_string url_win;
		if(!utf8_to_windows(url, url_win))
			return ;
		ShellExecute((HWND)(GDK_WINDOW_HWND(gpAppFrame->window->window)),
			TEXT("OPEN"), url_win.c_str(), NULL, NULL, SW_SHOWNORMAL);
		return;
	}
#elif defined(CONFIG_GNOME)
	if(!conf->get_bool_at("dictionary/always_use_open_url_command")) {
		gnome_url_show(url, NULL);
		return;
	}
#endif
	
	const std::string &cmd = conf->get_string_at("dictionary/url_open_command");
	spawn_command(cmd.c_str(), url);
}

void play_sound_on_event(const gchar *eventname)
{
	if (conf->get_bool_at("dictionary/enable_sound_event"))
	  play_sound_file(gStarDictDataDir+ G_DIR_SEPARATOR_S "sounds" G_DIR_SEPARATOR_S +eventname+".wav");
}
