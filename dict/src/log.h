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

#ifndef __LOG_H__
#define __LOG_H__

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <memory>
#include <string>
#include <glib.h>
#include <gtk/gtk.h>

enum MessageLevel
{
	MessageLevel_NONE = 0,
	MessageLevel_ERROR,
	MessageLevel_CRITICAL,
	MessageLevel_WARNING,
	MessageLevel_MESSAGE,
	MessageLevel_INFO,
	MessageLevel_DEBUG
};

class Logger
{
public:
	Logger(MessageLevel console_level, MessageLevel log_level);
	~Logger(void);
	MessageLevel get_console_message_level(void) const
	{
		return console_message_level;
	}
	void set_console_message_level(MessageLevel level)
	{
		console_message_level = level;
	}
	MessageLevel get_log_message_level(void) const
	{
		return log_message_level;
	}
	void set_log_message_level(MessageLevel level);
	static MessageLevel convert_message_level(gint level);
	static const MessageLevel default_message_level = MessageLevel_MESSAGE;
private:
	static void log_handler(const gchar * log_domain,
				       GLogLevelFlags log_level,
				       const gchar *message,
				       gpointer user_data);
	static void print_handler(const gchar* message);
	static bool is_do_log_message(GLogLevelFlags message_log_level, MessageLevel max_level);
	static std::string get_log_file_name(void);
	void open_log(void);
	void start_flush_log_timer(void);
	void destroy_flush_log_timer(void);
	static gint vFlushLogTimeOutCallback(gpointer data);
	static void show_error_dialog(const char* msg);

	MessageLevel console_message_level;
	MessageLevel log_message_level;
	GPrintFunc old_print_handler;
	GLogFunc old_log_handler;
	FILE * h_log_file;
	gint flush_log_timer;
	static const int FLUSH_LOG_TIMEOUT = 5000; // 5 seconds
};

extern std::unique_ptr<Logger> logger;

#ifdef ENABLE_LOG_WINDOW
/* Log window that show output from g_print, g_message, g_debug and the like 
We need such a window because windows console does not show all unicode characters. */
class LogWindow {
public:
	LogWindow(void);
	void Init(void);
	void End(void);
	void append(const gchar* str);
private:
	void append_in_window(const gchar* str);
	static void on_destroy(GtkObject *object, gpointer userdata);
private:
	GtkWidget *window;
	GtkTextView *textview;
	GtkScrolledWindow *scrolled_window;
	/* store text while window is not available */
	std::string text_buf;
};

extern LogWindow gLogWindow;
#endif // ENABLE_LOG_WINDOW

#if defined(_WIN32) && defined(_DEBUG)
void test_windows_console(void);
#endif

#endif
