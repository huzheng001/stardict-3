#ifndef _LOG_HPP_
#define _LOG_HPP_

#include <glib.h>

/**
 * Encapsulate logging mechanizm.
 */
class Logger {
public:
	Logger(gint level = 2);
	void set_verbosity(gint level) { verbose_ = level; }
private:
	gint verbose_;

	static void log(const gchar *log_domain,
			GLogLevelFlags log_level,
			const gchar *message,
			gpointer user_data);
};


#endif//!_LOG_HPP_
