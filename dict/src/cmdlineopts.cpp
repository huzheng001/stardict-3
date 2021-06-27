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

#include <glib.h>
#include <glib/gi18n.h>

#ifdef _WIN32
#  include <windows.h>
#endif

#include <memory>

#include "cmdlineopts.h"
#include "log.h"

static gint console_message_level = Logger::default_message_level;
static gint log_message_level = MessageLevel_NONE;
static gboolean hide_option = FALSE;
#if defined(_WIN32) || defined(CONFIG_GNOME)
static gboolean newinstance_option = FALSE;
#endif
#ifdef CONFIG_GNOME
static gboolean quit_option = FALSE;
#endif
static gchar **query_words = NULL;
static gchar *dirs_config_option = NULL;
static gchar *dirs_config_option_pre = NULL;
#if defined(_WIN32)
static gboolean portable_mode_option = FALSE;
static gboolean portable_mode_option_pre = FALSE;
#endif

static const GOptionEntry options [] =
{
	{ "message-level", 'm', 0, G_OPTION_ARG_INT, &console_message_level,
	  N_("How many messages print to console (0-6)"), NULL },
	{ "log-level", 'l', 0, G_OPTION_ARG_INT, &log_message_level,
	  N_("How many messages print to log (0-6)"), NULL },
	{ "hide", 'h', 0, G_OPTION_ARG_NONE, &hide_option,
	  N_("Hide the main window, do not show splash screen"), NULL },
#if defined(_WIN32) || defined(CONFIG_GNOME)
	{ "new", 'n', 0, G_OPTION_ARG_NONE, &newinstance_option,
	  N_("Start a new instance of stardict"), NULL },
#endif
#ifdef CONFIG_GNOME
	{ "quit", 'q', 0, G_OPTION_ARG_NONE, &quit_option,
	  N_("Quit an existing instance of stardict"), NULL },
#endif
	{ "dirs-config", 0, 0, G_OPTION_ARG_FILENAME, &dirs_config_option,
	  N_("StarDict directories configuration file"), "config-file" },
#if defined(_WIN32)
	{ "portable-mode", 0, 0, G_OPTION_ARG_NONE, &portable_mode_option,
	  N_("Activate portable mode"), NULL },
#endif
	{ G_OPTION_REMAINING, '\0', 0, G_OPTION_ARG_STRING_ARRAY, &query_words, NULL, NULL },
	{NULL}
};

class CleanOptions {
public:
	/* Free data refered by options.
	This method is called right before exiting the application.
	Strictly speaking, all variables will be freed anyway when the application exits.
	Calling this function is right programming practice. */
	~CleanOptions(void)
	{
		g_free(dirs_config_option);
		dirs_config_option = NULL;
		g_strfreev(query_words);
		query_words = NULL;
		g_free(dirs_config_option_pre);
		dirs_config_option_pre = NULL;
	}
} cleanOptions;

/* Preliminary parsing command line arguments.
We only extract a few options that we need early in project initialization before
the main command line parser can be used. */
void CmdLineOptions::pre_parse_arguments(int argc, char **argv)
{
	for(int i=1; i<argc; ++i) {
		if(!dirs_config_option_pre) {
			if(strstr(argv[i], "--dirs-config=") == argv[i])
				dirs_config_option_pre = g_strdup(argv[i] + sizeof("--dirs-config=") - 1);
			else if(strcmp(argv[i], "--dirs-config") == 0 && i + 1 < argc)
				dirs_config_option_pre = g_strdup(argv[i+1]);
		}
#if defined(_WIN32)
		if(strcmp(argv[i], "--portable-mode") == 0)
			portable_mode_option_pre = TRUE;
#endif
	}
}

const GOptionEntry* CmdLineOptions::get_options(void)
{
	return options;
}

MessageLevel CmdLineOptions::get_console_message_level(void)
{
	return Logger::convert_message_level(console_message_level);
}

MessageLevel CmdLineOptions::get_log_message_level(void)
{
	return Logger::convert_message_level(log_message_level);
}

gboolean CmdLineOptions::get_hide(void)
{
	return hide_option;
}

#if defined(_WIN32) || defined(CONFIG_GNOME)
gboolean CmdLineOptions::get_newinstance(void)
{
	return newinstance_option;
}
#endif

#ifdef CONFIG_GNOME
gboolean CmdLineOptions::get_quit(void)
{
	return quit_option;
}
#endif

gchar const * const* CmdLineOptions::get_query_words(void)
{
	return query_words;
}

gchar const * CmdLineOptions::get_dirs_config(void)
{
	return dirs_config_option;
}

gchar const * CmdLineOptions::get_dirs_config_pre(void)
{
	return dirs_config_option_pre;
}

#if defined(_WIN32)
gboolean CmdLineOptions::get_portable_mode(void)
{
	return portable_mode_option;
}

gboolean CmdLineOptions::get_portable_mode_pre(void)
{
	return portable_mode_option_pre;
}
#endif
