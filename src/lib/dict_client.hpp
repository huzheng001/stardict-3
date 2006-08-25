#ifndef _DICT_CLINET_HPP_
#define _DICT_CLINET_HPP_

#include <glib.h>

class DictClient {
public:
	DictClient();
	~DictClient();
	bool connect(const char *host, int port);
private:
	GIOChannel *channel;
	guint source_id;

	void disconnect();
	static gboolean on_io_event(GIOChannel *, GIOCondition, gpointer);
	static gint get_status_code(gchar *line);
};

#endif//!_DICT_CLINET_HPP_
