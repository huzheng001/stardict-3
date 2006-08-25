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

bool DictClient::connect(const char *host, int port)
{
	int sd = Socket::socket();
	if (sd == -1 || !Socket::connect(sd, host, port)) {
		g_warning("Can not connect to %s: %s\n",
			  host, Socket::getErrorMsg().c_str());
		return false;
	}
	GIOChannel *ch;
#ifdef _WIN32
	ch = g_io_channel_win32_new_socket(sd);
#else
	ch = g_io_channel_unix_new(sd);
#endif

/* RFC2229 mandates the usage of UTF-8, so we force this encoding */
	g_io_channel_set_encoding(ch, "UTF-8", NULL);

	g_io_channel_set_line_term(ch, "\r\n", 2);

	gchar *str = NULL;
	if (g_io_channel_read_line(ch, &str, NULL, NULL, NULL) ==
	    G_IO_STATUS_NORMAL)
		g_debug("read: %s\n", str);
	return true;
}
