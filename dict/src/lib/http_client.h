#ifndef _STARDICT_HTTP_CLIENT_H_
#define _STARDICT_HTTP_CLIENT_H_

#include <glib.h>
#include <string>
#include <vector>
#include <cstring>

#ifndef _WIN32
#  include <netdb.h>
#else
typedef unsigned long in_addr_t;
#endif

#include "stardict-sigc++.h"


typedef void (*get_http_response_func_t)(char *buffer, size_t buffer_len, gpointer userdata);
enum HttpMethod {HTTP_METHOD_GET, HTTP_METHOD_POST};

class HttpClient {
public:
	sigc::signal<void, HttpClient*, const char *> on_error_;
	sigc::signal<void, HttpClient *> on_response_;

	HttpClient();
	~HttpClient();
	void SendHttpGetRequest(const char* shost, const char* sfile, gpointer data);
	void SendHttpGetRequestWithCallback(const char* shost, const char* sfile, get_http_response_func_t callback_func, gpointer data);
	void SendHttpRequest(const char* shost, const char* sfile, gpointer data);
	void SetMethod(HttpMethod httpMethod)
	{
		httpMethod_ = httpMethod;
	}
	void SetHeaders(const char* headers)
	{
		headers_ = headers;
	}
	void SetBody(const char* body)
	{
		body_ = body;
	}
	void SetAllowAbsoluteURI(bool b)
	{
		allow_absolute_URI_ = b;
	}

	char *buffer;
	size_t buffer_len;
	gpointer userdata;
	get_http_response_func_t callback_func_;
private:
	std::string host_;
	std::string file_;
	HttpMethod httpMethod_;
	std::string headers_;
	std::string body_;
	bool allow_absolute_URI_;
	int sd_;
	GIOChannel *channel_;
	guint in_source_id_;
	guint out_source_id_;
	static void on_resolved(gpointer data, bool resolved, in_addr_t sa);
	static void on_connected(gpointer data, bool succeeded);
	static gboolean on_io_in_event(GIOChannel *, GIOCondition, gpointer);
	static gboolean on_io_out_event(GIOChannel *, GIOCondition, gpointer);
	void disconnect();
	void write_str(const char *str, GError **err);
	bool SendRequest();
};

#endif
