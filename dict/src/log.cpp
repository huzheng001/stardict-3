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

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <glib.h>
#include <glib/gstdio.h>
#include <glib/gi18n.h>
#include <cstdio>
#include <iostream>
#include <sstream>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef _WIN32
#  include <windows.h>
#  include <io.h>
#  include <fcntl.h>
#  include <process.h>
#endif

#include "conf.h"
#include "log.h"
#include "lib/utils.h"
#include "libcommon.h"

std::unique_ptr<Logger> logger;

Logger::Logger(MessageLevel console_level, MessageLevel log_level)
:
console_message_level(console_level),
log_message_level(log_level),
old_print_handler(NULL),
old_log_handler(NULL),
h_log_file(NULL),
flush_log_timer(0)
{
	old_log_handler = g_log_set_default_handler(log_handler, NULL);
	old_print_handler = g_set_print_handler(print_handler);
	if(log_message_level > MessageLevel_NONE)
		open_log();
}

Logger::~Logger(void)
{
	g_log_set_default_handler(old_log_handler, NULL);
	g_set_print_handler(old_print_handler);
	if(h_log_file)
		fclose(h_log_file);
	destroy_flush_log_timer();
}

void Logger::set_log_message_level(MessageLevel level)
{
	log_message_level = level;
	if(log_message_level > MessageLevel_NONE && !h_log_file)
		open_log();
}

MessageLevel Logger::convert_message_level(gint level)
{
	if(level < MessageLevel_NONE)
		level = MessageLevel_NONE;
	if(level > MessageLevel_DEBUG)
		level = MessageLevel_DEBUG;
	return static_cast<MessageLevel>(level);
}

void Logger::log_handler(const gchar * log_domain,
			       GLogLevelFlags log_level,
			       const gchar *message,
			       gpointer user_data)
{
	std::stringstream buf;
	if(log_domain && log_domain[0])
		buf << "(" << log_domain << ") ";
	if(log_level & G_LOG_LEVEL_ERROR)
		buf << "[error] ";
	else if(log_level & G_LOG_LEVEL_CRITICAL)
		buf << "[critical] ";
	else if(log_level & G_LOG_LEVEL_WARNING)
		buf << "[warning] ";
	else if(log_level & G_LOG_LEVEL_MESSAGE)
		buf << "[message] ";
	else if(log_level & G_LOG_LEVEL_INFO)
		buf << "[info] ";
	else if(log_level & G_LOG_LEVEL_DEBUG)
		buf << "[debug] ";
	if(message)
		buf << message;
	buf << "\n";
	if(is_do_log_message(log_level, logger->console_message_level)) {
		std::cout << buf.str();
#ifdef ENABLE_LOG_WINDOW
		gLogWindow.append(buf.str().c_str());
#endif
	}
	if(is_do_log_message(log_level, logger->log_message_level) && logger->h_log_file) {
		fputs(buf.str().c_str(), logger->h_log_file);
		logger->start_flush_log_timer();
	}
	// the application will be aborted short after this message
	if(log_level & G_LOG_FLAG_FATAL) {
		if(logger->h_log_file)
			fflush(logger->h_log_file);
		// make the fault loud and clear for the user
		show_error_dialog(buf.str().c_str());
	}
}

void Logger::print_handler(const gchar* message)
{
	if(MessageLevel_MESSAGE <= logger->console_message_level) {
		std::cout << message;
#ifdef ENABLE_LOG_WINDOW
		gLogWindow.append(message);
#endif
	}
	if(MessageLevel_MESSAGE <= logger->log_message_level && logger->h_log_file) {
		fputs(message, logger->h_log_file);
		logger->start_flush_log_timer();
	}
}

// returns true if the message must be processed
bool Logger::is_do_log_message(GLogLevelFlags message_log_level, MessageLevel max_level)
{
	if(message_log_level & G_LOG_LEVEL_ERROR) {
		return MessageLevel_ERROR <= max_level;
	}
	if(message_log_level & G_LOG_LEVEL_CRITICAL) {
		return MessageLevel_CRITICAL <= max_level;
	}
	if(message_log_level & G_LOG_LEVEL_WARNING) {
		return MessageLevel_WARNING <= max_level;
	}
	if(message_log_level & G_LOG_LEVEL_MESSAGE) {
		return MessageLevel_MESSAGE <= max_level;
	}
	if(message_log_level & G_LOG_LEVEL_INFO) {
		return MessageLevel_INFO <= max_level;
	}
	if(message_log_level & G_LOG_LEVEL_DEBUG) {
		return MessageLevel_DEBUG <= max_level;
	}
	return false;
}

std::string Logger::get_log_file_name(void)
{
	/* A number of StarDict instances may be launched.
	Each instance gets an independent log file.
	We use pid to construct unique log file name. */
	std::stringstream buf;
	buf << build_path(conf_dirs->get_log_dir(), "stardict-")
#ifdef _WIN32
		<< (gulong)_getpid()
#else
		<< (gulong)getpid()
#endif
		<< ".log";
	return buf.str();
}

void Logger::open_log(void)
{
	g_assert(!h_log_file);
	const std::string log_filename = get_log_file_name();
	h_log_file = g_fopen(log_filename.c_str(), "w");
	if(!h_log_file)
		g_warning("Unable to create log file %s", log_filename.c_str());
}

void Logger::start_flush_log_timer(void)
{
	if (!flush_log_timer)
		flush_log_timer = g_timeout_add(FLUSH_LOG_TIMEOUT, vFlushLogTimeOutCallback, this);
}

void Logger::destroy_flush_log_timer(void)
{
	if (flush_log_timer) {
		g_source_remove(flush_log_timer);
		flush_log_timer = 0;
	}
}

gint Logger::vFlushLogTimeOutCallback(gpointer data)
{
	Logger* pLogger = static_cast<Logger*>(data);
	if(pLogger->h_log_file)
		fflush(pLogger->h_log_file);
	pLogger->flush_log_timer = 0;
	return FALSE; // destroy the timer
}

void Logger::show_error_dialog(const char* msg)
{
	GtkWidget *message_dlg = gtk_message_dialog_new(
		NULL,
		(GtkDialogFlags)(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
		GTK_MESSAGE_ERROR,
		GTK_BUTTONS_OK,
		"%s", msg
	);

	gtk_dialog_set_default_response(GTK_DIALOG(message_dlg), GTK_RESPONSE_OK);
	gtk_window_set_resizable(GTK_WINDOW(message_dlg), FALSE);
	gtk_window_set_title(GTK_WINDOW(message_dlg), _("Error"));
	gtk_dialog_run(GTK_DIALOG(message_dlg));
	gtk_widget_destroy(message_dlg);
}

#ifdef ENABLE_LOG_WINDOW

LogWindow::LogWindow(void):
	window(NULL),
	textview(NULL),
	scrolled_window(NULL)
{
}

void LogWindow::Init(void)
{
	if(window)
		return;

	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(window), _("Log window"));
	gtk_window_set_default_size(GTK_WINDOW(window), 400, 400);
	g_signal_connect(window, "destroy", G_CALLBACK(on_destroy), this);

	scrolled_window = GTK_SCROLLED_WINDOW(gtk_scrolled_window_new(NULL, NULL));
	gtk_container_set_border_width(GTK_CONTAINER (scrolled_window), 5);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
		GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolled_window), GTK_SHADOW_IN);
	gtk_container_add(GTK_CONTAINER(window), GTK_WIDGET(scrolled_window));

	textview = GTK_TEXT_VIEW(gtk_text_view_new());
	gtk_text_view_set_editable(textview, FALSE);
	gtk_text_view_set_cursor_visible(textview, FALSE);
	gtk_text_view_set_wrap_mode(textview, GTK_WRAP_CHAR);
	gtk_container_add(GTK_CONTAINER(scrolled_window), GTK_WIDGET(textview));

	append_in_window(text_buf.c_str());
	text_buf.clear();

	gtk_widget_show_all(window);
}

void LogWindow::End(void)
{
	if(window)
		gtk_widget_destroy(window);
	window = NULL;
	textview = NULL;
	scrolled_window = NULL;
}

void LogWindow::append(const gchar* str)
{
	/* append_in_window accepts only valid utf8 strings, 
	we must take care over the proper encoding. */
	std::string fixed_str;
	if(!g_utf8_validate(str, -1, NULL)) {
		fixed_str = fix_utf8_str(str);
		str = fixed_str.c_str();
	}
	if(window)
		append_in_window(str);
	else
		text_buf.append(str);
}

void LogWindow::append_in_window(const gchar* str)
{
	g_assert(window);
	GtkTextBuffer *buffer = gtk_text_view_get_buffer(textview);
	GtkTextIter iter;
	gtk_text_buffer_get_end_iter(buffer, &iter);
	gtk_text_buffer_insert(buffer, &iter, str, -1);
	/* scroll window to show the last line */
	GtkAdjustment* vadj = gtk_scrolled_window_get_vadjustment(scrolled_window);
	gtk_adjustment_set_value(vadj, vadj->upper - vadj->page_size);
}

void LogWindow::on_destroy(GtkObject *object, gpointer userdata)
{
	LogWindow *log_win = static_cast<LogWindow *>(userdata);
	log_win->window = NULL;
	log_win->textview = NULL;
	log_win->scrolled_window = NULL;
}

LogWindow gLogWindow;
#endif // ENABLE_LOG_WINDOW

#if defined(_WIN32) && defined(_DEBUG)
void test_windows_console(void)
{
	/* test stdout and stderr
	By default GUI application on Windows do not have a console associated 
	with them, so output from printf, std::cout << "string" goes nowhere.
	(printf may not work for other reasons too, see msvc_2008/readme.txt.)
	Create a console and redirect stdout and stderr to it.
	See Microsoft article for details: http://support.microsoft.com/kb/105305.
	
	For g_warning and the like that is not enough.
	"gmessages.c in 2.6 doesn't use stdio on Windows any longer,
	but plain write() to file descriptor 1 or 2, like on Unix."
	We may dup2 descriptor 1 to stdout, and dup2 descriptor 2 to stderr.
	That works when we use Dev-Cpp to compile the project, but does not work with MSVC.
	For that reason a custom log handler was set up (with g_log_set_handler).
	
	That all is not reliable, unfortunately... It has been tested on Windows XP.
	*/
	// check what works
	puts("Hello from puts");
#ifdef _MSC_VER
	printf_s("Hello from printf_s\n");
#endif
	printf("Hello from printf\n");
#ifdef _MSC_VER
	fprintf_s(stderr, "Hello from fprintf_s on stderr\n");
#endif
#ifndef _WIN32
	// crashs on Windows
	fprintf(stderr, "Hello from fprintf on stderr\n");
#endif
	std::cout << "Hello from cout!" << std::endl;
	std::cerr << "Hello from cerr!" << std::endl;
	g_printf("Hello from g_printf\n");
	g_print("Hello from g_print\n");
	g_debug("Hello from g_debug");
	g_message("Hello from g_message. 1 = %d", 1);
	g_warning("Hello from g_warning");
	g_critical("Hello from g_critical");
}

#endif // #ifdef defined(_WIN32) && defined(_DEBUG)
