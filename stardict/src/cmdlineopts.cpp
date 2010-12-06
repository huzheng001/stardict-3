#include <glib.h>
#include <glib/gi18n.h>

#ifdef _WIN32
#  include <windows.h>
#endif

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
	{ G_OPTION_REMAINING, '\0', 0, G_OPTION_ARG_STRING_ARRAY, &query_words, NULL, NULL },
	{NULL}
};

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
