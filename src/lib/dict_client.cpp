/* 
 * This file part of StarDict - A international dictionary for GNOME.
 * http://stardict.sourceforge.net
 * Copyright (C) 2006 Evgeniy <dushistov@mail.ru>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <glib.h>

#include "sockets.hpp"

#include "dict_client.hpp"

DictClient::DictClient()
{
	channel = NULL;
	source_id = 0;
}

DictClient::~DictClient()
{
	disconnect();
}


bool DictClient::connect(const char *host, int port)
{
	int sd = Socket::socket();
	if (sd == -1) {
		g_warning("Can not connect to %s: %s\n",
			  host, Socket::getErrorMsg().c_str());
		return false;
	}

#ifdef _WIN32
	channel = g_io_channel_win32_new_socket(sd);
#else
	channel = g_io_channel_unix_new(sd);
#endif

/* RFC2229 mandates the usage of UTF-8, so we force this encoding */
	g_io_channel_set_encoding(channel, "UTF-8", NULL);

	g_io_channel_set_line_term(channel, "\r\n", 2);

/* make sure that the channel is non-blocking */
	int flags = g_io_channel_get_flags(channel);
	flags |= G_IO_FLAG_NONBLOCK;
	GError *err = NULL;
	g_io_channel_set_flags(channel, GIOFlags(flags), &err);
	if (err) {
		g_warning("Unable to set the channel as non-blocking: %s",
			   err->message);
                   
		g_error_free(err);
		g_io_channel_unref(channel);
		channel = NULL;      
		return false;
	}

	if (!Socket::connect(sd, host, port)) {
		g_warning("Can not connect to %s: %s\n",
			  host, Socket::getErrorMsg().c_str());
		return false;
	}       

	source_id = g_io_add_watch(channel, GIOCondition(G_IO_IN | G_IO_ERR),
				   on_io_event, this);

	return true;
}

void DictClient::disconnect()
{
	if (source_id) {
		g_source_remove(source_id);
		source_id = 0;
	}

	if (channel) {
		g_io_channel_shutdown(channel, TRUE, NULL);
		g_io_channel_unref(channel);		
		channel = NULL;
	}
}

gboolean DictClient::on_io_event(GIOChannel *ch, GIOCondition cond,
				 gpointer user_data)
{
	DictClient *dict_client = static_cast<DictClient *>(user_data);

	g_assert(dict_client);
	if (!dict_client->channel) {
		g_warning("No channel available\n");
		return FALSE;
	}

	if (cond & G_IO_ERR) {
		g_warning("Channel IO error\n");
		return FALSE;
	}

	gsize term, len;
	gchar *line;
	GIOStatus res =
		g_io_channel_read_line(dict_client->channel, &line,
				       &len, &term, NULL);
	if (res == G_IO_STATUS_ERROR) {
		g_warning("Channel IO error\n");
		return FALSE;
	}

	if (!len)
		return TRUE;

	//truncate the line terminator before parsing
	line[term] = '\0';
	gint status_code = get_status_code(line);
	g_debug("statuc code: %d\n", status_code);
	g_free(line);

	return TRUE;
}

/* retrieve the status code from the server response line */
gint DictClient::get_status_code(gchar *line)
{  
  gint retval;
  
  if (strlen (line) < 3)
    return 0;
  
  if (!g_unichar_isdigit (line[0]) ||
      !g_unichar_isdigit (line[1]) ||
      !g_unichar_isdigit (line[2]))
    return 0;
  
  gchar tmp = line[3];
  line[3] = '\0';

  retval = atoi(line);

  line[3] = tmp;
  
  return retval;
}  
