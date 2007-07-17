#ifndef _STARDICT_HTTP_CLIENT_H_
#define _STARDICT_HTTP_CLIENT_H_

#include <glib.h>
#include "sigc++/sigc++.h"
#include <string>
#include <vector>

typedef void (*get_http_response_func_t)(char *buffer, size_t buffer_len, int userdata);

class HttpClient {
public:
	static sigc::signal<void, HttpClient*, const char *> on_error_;
	static sigc::signal<void, HttpClient *> on_response_;

	HttpClient();
	~HttpClient();
	void SendHttpGetRequest(const char* shost, const char* sfile, gint data);
	void SendHttpGetRequestWithCallback(const char* shost, const char* sfile, get_http_response_func_t callback_func, gint data);
	char *buffer;
	size_t buffer_len;
	gint userdata;
	get_http_response_func_t callback_func_;
private:
	std::string host_;
	std::string file_;
	int sd_;
	GIOChannel *channel_;
	guint in_source_id_;
	guint out_source_id_;
	static void on_resolved(gpointer data, struct hostent *ret);
	static gboolean on_io_in_event(GIOChannel *, GIOCondition, gpointer);
	static gboolean on_io_out_event(GIOChannel *, GIOCondition, gpointer);
	void disconnect();
	void write_str(const char *str, GError **err);
	bool SendGetRequest();
};

#endif
