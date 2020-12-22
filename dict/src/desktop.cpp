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

#include <glib/gi18n.h>
#include <cstdlib>

#ifdef CONFIG_GNOME
#  include <libgnome/libgnome.h>
#elif defined(_WIN32)
#  include <gdk/gdkwin32.h>
#  include <windows.h>
#endif

#include "conf.h"
#include "stardict.h"

#include "desktop.h"

#if GTK_MAJOR_VERSION >= 3

#ifndef _WIN32
#  include "canberra.h"
#  include "canberra-gtk.h"
#endif

#endif



#ifndef CONFIG_GNOME
/* Guess value of the current locale from values of environment variables
	and system-dependent defaults.
	This function works as a simplified version of the guess_category_value
	function from the gettext package.
*/
static std::string guess_locale(void)
{
#ifdef _WIN32
	/* LC_MESSAGES is not defined on Windows. It compiles but produces a runtime error.
	We may try gtk_set_locale() to get the effective locale. */
	glib::CharStr locale_wrap(g_win32_getlocale());
	const char* locale = get_impl(locale_wrap);
#else
	const char *locale = setlocale(LC_MESSAGES, NULL);
#endif
	if (strcmp (locale, "C") == 0)
		return locale;
	const char *language = getenv ("LANGUAGE");
	if (language != NULL && language[0] != '\0')
		return language;
	return locale;
}

static std::string guess_html_help_dir(void)
{
	const std::string std_locale_list = guess_locale();
	const char* locale_list = std_locale_list.c_str();
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
		dir = build_path(conf_dirs->get_help_dir(), locale);
		if(g_file_test(dir.c_str(), G_FILE_TEST_IS_DIR))
			return dir;
		pos = locale.find('_');
		if(pos != std::string::npos)
			locale.resize(pos);
		dir = build_path(conf_dirs->get_help_dir(), locale);
		if(g_file_test(dir.c_str(), G_FILE_TEST_IS_DIR))
			return dir;
	}
	dir = build_path(conf_dirs->get_help_dir(), "C");
	if(g_file_test(dir.c_str(), G_FILE_TEST_IS_DIR))
		return dir;
	return "";
}
#endif

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
		if(done) {
			return;
		} else { // else mci service failed.
			PlaySound(filename_win.c_str(), 0, SND_ASYNC | SND_FILENAME);
			return;
		}

	} else {
		return; // error
	}
#else

#if GTK_MAJOR_VERSION >= 3
	if (conf->get_bool_at("dictionary/always_use_sound_play_command")) {
		const std::string &playcmd = conf->get_string_at("dictionary/sound_play_command");
		spawn_command(playcmd.c_str(), filename.c_str());
	} else {
		ca_context_play(ca_gtk_context_get(), 0, CA_PROP_MEDIA_FILENAME, filename.c_str(), NULL);
	}
#else
	const std::string &playcmd = conf->get_string_at("dictionary/sound_play_command");
	spawn_command(playcmd.c_str(), filename.c_str());
#endif

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
#if defined(CONFIG_GNOME)
	glib::Error err;
	if(!gnome_help_display ("stardict.xml", section, get_addr(err))) {
		glib::CharStr message(g_strdup_printf(_("Show help action failed with the following error message: \"%s\""),
			err->message));
		show_error(get_impl(message));
	}
#else
	std::string dir = guess_html_help_dir();
	if(dir.empty()) {
		std::string index_file = build_path(conf_dirs->get_help_dir(),
			"C" G_DIR_SEPARATOR_S "index.html");
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
		play_sound_file(build_path(conf_dirs->get_data_dir(),
			std::string("sounds" G_DIR_SEPARATOR_S) +eventname+".wav"));
}

/* Enabling network dictionaries poses a security risk.
 * When Enable Net Dict is configured, StarDict sends the contents
 * of the clipboard to a dictionary server, which allows remote attackers
 * to obtain sensitive information by sniffing the network.
 * Warn the user about the risk and let him/her cancel the request. */
bool confirm_enable_network_dicts(GtkWidget *parent_window)
{
	if(!parent_window)
		parent_window = gpAppFrame->window;
	const char* msg = _("You are about to enable network dictionaries. "
		"Be aware that enabling network dictionaries poses a security risk. "
		"Your text will be sent to remote servers. "
		"Additionally, since network requests do not use any encryption, "
		"anyone able to sniff communication can see that text. "
		"Text will be send when you type into the Search field of the Main window and "
		"when you select some text if Scan selection feature is enabled. "
		"There may be other cases when network queries are done.\n"
		"\n"
		"Do you want to enable network dictionaries anyway?"
		);
	GtkWidget *dialog = gtk_message_dialog_new(
			GTK_WINDOW(parent_window),
			GTK_DIALOG_MODAL,
			GTK_MESSAGE_WARNING,
			GTK_BUTTONS_YES_NO,
			"%s",
			msg);
	bool confirmed = false;
	gint response = gtk_dialog_run (GTK_DIALOG (dialog));
	if (response == GTK_RESPONSE_YES) {
		confirmed = true;
	}
	gtk_widget_destroy(dialog);
	return confirmed;
}
