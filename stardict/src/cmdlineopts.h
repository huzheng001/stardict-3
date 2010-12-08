#include <glib.h>
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
};
