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
#include <gtk/gtk.h>
#include "log.h"

class CmdLineOptions {
public:
	static void pre_parse_arguments(int argc, char **argv);
	static const GOptionEntry* get_options(void);
	static MessageLevel get_console_message_level(void);
	static MessageLevel get_log_message_level(void);
	static gboolean get_hide(void);
#if defined(_WIN32) || defined(CONFIG_GNOME)
	static gboolean get_newinstance(void);
#endif
#ifdef CONFIG_GNOME
	static gboolean get_quit(void);
#endif
	static gchar const * const* get_query_words(void);
	static gchar const * get_dirs_config(void);
	static gchar const * get_dirs_config_pre(void);
#if defined(_WIN32)
	static gboolean get_portable_mode(void);
	static gboolean get_portable_mode_pre(void);
#endif
};
