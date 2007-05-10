#ifndef _STARDICT_HTTP_CLIENT_H_
#define _STARDICT_HTTP_CLIENT_H_

#include <glib.h>
#include "sigc++/sigc++.h"
#include <string>
#include <vector>

class HttpClient {
public:
	static sigc::signal<void, const char *> on_error_;
	static sigc::signal<void, const char *, size_t> on_response_;

	HttpClient();
	~HttpClient();
	void SendHttpGetRequest(const char* shost, const char* sfile);
	char *buffer;
private:
	std::string host_;
	std::string file_;
	GIOChannel *channel_;
	guint in_source_id_;
	guint out_source_id_;
	static void on_resolved(gpointer data, struct hostent *ret);
	static gboolean on_io_in_event(GIOChannel *, GIOCondition, gpointer);
	static gboolean on_io_out_event(GIOChannel *, GIOCondition, gpointer);
	void disconnect();
	void write_str(const char *str, GError **err);
	bool SendGetRequest();
	size_t buffer_len;
};

#endif
