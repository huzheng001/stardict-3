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


typedef void (*get_http_response_func_t)(const char *buffer, size_t buffer_len, gpointer userdata);
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
