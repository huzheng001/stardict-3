#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "http_client.h"
#include "sockets.hpp"

sigc::signal<void, const char *> HttpClient::on_error_;
sigc::signal<void, const char *, size_t> HttpClient::on_response_;

HttpClient::HttpClient()
{
	channel_ = NULL;
	in_source_id_ = 0;
	out_source_id_ = 0;
	buffer = NULL;
	buffer_len = 0;
}

HttpClient::~HttpClient()
{
	disconnect();
}

void HttpClient::disconnect()
{
	if (in_source_id_) {
		g_source_remove(in_source_id_);
		in_source_id_ = 0;
	}
	if (out_source_id_) {
		g_source_remove(out_source_id_);
		out_source_id_ = 0;
	}
	if (channel_) {
		g_io_channel_shutdown(channel_, TRUE, NULL);
		g_io_channel_unref(channel_);
		channel_ = NULL;
	}
}

void HttpClient::SendHttpGetRequest(const char* shost, const char* sfile)
{
	host_ = shost;
	file_ = sfile;
	Socket::resolve(host_, this, on_resolved);
}

void HttpClient::on_resolved(gpointer data, struct hostent *ret)
{
	HttpClient *oHttpClient = (HttpClient *)data;
	int sd = Socket::socket();
	if (sd == -1) {
		std::string str = "Can not create socket: " + Socket::get_error_msg();
		on_error_.emit(str.c_str());
		return;
	}

#ifdef _WIN32
	oHttpClient->channel_ = g_io_channel_win32_new_socket(sd);
#else
	oHttpClient->channel_ = g_io_channel_unix_new(sd);
#endif
	g_io_channel_set_encoding(oHttpClient->channel_, NULL, NULL);
	/* make sure that the channel is non-blocking */
	int flags = g_io_channel_get_flags(oHttpClient->channel_);
	flags |= G_IO_FLAG_NONBLOCK;
	GError *err = NULL;
	g_io_channel_set_flags(oHttpClient->channel_, GIOFlags(flags), &err);
	if (err) {
		g_io_channel_unref(oHttpClient->channel_);
		oHttpClient->channel_ = NULL;
		gchar *str = g_strdup_printf("Unable to set the channel as non-blocking: %s", err->message);
		on_error_.emit(str);
		g_free(str);
		g_error_free(err);
		return;
	}
	if (!Socket::connect(sd, ret, 80)) {
		gchar *mes = g_strdup_printf("Can not connect to %s: %s\n",
			oHttpClient->host_.c_str(), Socket::get_error_msg().c_str());
		on_error_.emit(mes);
		g_free(mes);
		return;
	}
	if (oHttpClient->SendGetRequest())
		return;
	oHttpClient->out_source_id_ = g_io_add_watch(oHttpClient->channel_, GIOCondition(G_IO_OUT), on_io_out_event, oHttpClient);
	oHttpClient->in_source_id_ = g_io_add_watch(oHttpClient->channel_, GIOCondition(G_IO_IN | G_IO_ERR), on_io_in_event, oHttpClient);
}

void HttpClient::write_str(const char *str, GError **err)
{
	int len = strlen(str);
	int left_byte = len;
	GIOStatus res;
	gsize bytes_written;
	while (left_byte) {
		res = g_io_channel_write_chars(channel_, str+(len - left_byte), left_byte, &bytes_written, err);
		if (res != G_IO_STATUS_NORMAL) {
			disconnect();
			return;
		}
		left_byte -= bytes_written;
	}
	res = g_io_channel_flush(channel_, err);
	if (res == G_IO_STATUS_ERROR) {
		disconnect();
	}
}

bool HttpClient::SendGetRequest()
{
	std::string request;

	request += "GET HTTP://";
	request += host_;
	request += file_;
	request += " HTTP/1.1\r\n";

	request += "User-Agent: Mozilla/4.0(compatible;MSIE 5.00;Windows 98)\r\n";
	request += "Accept: */*\r\n";

	request += "Host: ";
	request += host_;
	request += "\r\n";

	request += "Connection: close\r\n\r\n";
	
	GError *err = NULL;
	write_str(request.c_str(), &err);
	if (err) {
		on_error_.emit(err->message);
		g_error_free(err);
		return true;
	}
	return false;
}

gboolean HttpClient::on_io_out_event(GIOChannel *ch, GIOCondition cond, gpointer user_data)
{
	HttpClient *http_client = static_cast<HttpClient *>(user_data);
	if (!http_client->channel_) {
		return FALSE;
	}
	GIOStatus res = g_io_channel_flush(http_client->channel_, NULL);
	if (res == G_IO_STATUS_AGAIN) {
		return TRUE;
	} else if (res == G_IO_STATUS_ERROR) {
		http_client->disconnect();
	}
	return FALSE;
}

gboolean HttpClient::on_io_in_event(GIOChannel *ch, GIOCondition cond, gpointer user_data)
{
	HttpClient *http_client = static_cast<HttpClient *>(user_data);
	if (!http_client->channel_) {
		return FALSE;
	}
	if (cond & G_IO_ERR) {
		http_client->disconnect();
		return FALSE;
	}
	if (cond & G_IO_STATUS_EOF) {
		http_client->disconnect();
		g_print("Over\n");
		return FALSE;
	}
	GIOStatus res;
	gchar *str_return = NULL;
	gsize length;
	GError *err = NULL;

	res = g_io_channel_read_to_end(http_client->channel_, &str_return, &length, &err);
	if (res == G_IO_STATUS_ERROR) {
		if (err) {
			gchar *str = g_strdup_printf("Error while reading reply from server: %s", err->message);
			on_error_.emit(str);
			g_free(str);
			g_error_free(err);
		}
		http_client->disconnect();
		return FALSE;
	}
	if (str_return) {
		http_client->buffer = (char *)g_realloc(http_client->buffer, http_client->buffer_len + length);
		memcpy(http_client->buffer+http_client->buffer_len, str_return, length);
		http_client->buffer_len += length;
		g_free(str_return);
	}
	if (res == G_IO_STATUS_NORMAL) {
		http_client->disconnect();
		on_response_.emit(http_client->buffer, http_client->buffer_len);
		return FALSE;
	}
	return TRUE;
}
