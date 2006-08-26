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

/*
 * Based on RFC 2229 and gnome dictionary source code
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <glib.h>

#include "sockets.hpp"

#include "dict_client.hpp"

sigc::signal<void, const std::string&> DictClient::on_error_;
sigc::signal<void, const DictClient::IndexList&> DictClient::on_lookup_end_;

/* Status codes as defined by RFC 2229 */
enum DICTStatusCode {
  STATUS_INVALID                   = 0,
    
  STATUS_N_DATABASES_PRESENT       = 110,
  STATUS_N_STRATEGIES_PRESENT      = 111,
  STATUS_DATABASE_INFO             = 112,
  STATUS_HELP_TEXT                 = 113,
  STATUS_SERVER_INFO               = 114,
  STATUS_CHALLENGE                 = 130,
  STATUS_N_DEFINITIONS_RETRIEVED   = 150,
  STATUS_WORD_DB_NAME              = 151,
  STATUS_N_MATCHES_FOUND           = 152,
  STATUS_CONNECT                   = 220,
  STATUS_QUIT                      = 221,
  STATUS_AUTH_OK                   = 230,
  STATUS_OK                        = 250,
  STATUS_SEND_RESPONSE             = 330,
  /* Connect response codes */
  STATUS_SERVER_DOWN               = 420,
  STATUS_SHUTDOWN                  = 421,
  /* Error codes */
  STATUS_BAD_COMMAND               = 500,
  STATUS_BAD_PARAMETERS            = 501,
  STATUS_COMMAND_NOT_IMPLEMENTED   = 502,
  STATUS_PARAMETER_NOT_IMPLEMENTED = 503,
  STATUS_NO_ACCESS                 = 530,
  STATUS_USE_SHOW_INFO             = 531,
  STATUS_UNKNOWN_MECHANISM         = 532,
  STATUS_BAD_DATABASE              = 550,
  STATUS_BAD_STRATEGY              = 551,
  STATUS_NO_MATCH                  = 552,
  STATUS_NO_DATABASES_PRESENT      = 554,
  STATUS_NO_STRATEGIES_PRESENT     = 555
};

class DefineCmd : public DICT::Cmd {
public:
	DefineCmd(const char *word, const char *database) {
		query_ = std::string("DEFINE ") + database + " \"" + word +
			"\"\r\n";
	}
	bool parse(gchar *str, int code);
};

bool DefineCmd::parse(gchar *str, int code)
{
	if (state_ != DATA)
		return false;
	switch (code) {
	case STATUS_N_DEFINITIONS_RETRIEVED: {
		gchar *p = g_utf8_strchr(str, -1, ' ');
		if (p)
			p = g_utf8_next_char (p);
          
		g_debug("server replied: %d definitions found\n", atoi(p));                  
		break;
	}
	case STATUS_WORD_DB_NAME: {		
		gchar *word, *db_name, *db_full, *p;

		word = str;

		/* skip the status code */
		word = g_utf8_strchr(word, -1, ' ');
		word = g_utf8_next_char(word);
          
		if (word[0] == '\"')
			word = g_utf8_next_char(word);
          
		p = g_utf8_strchr(word, -1, '\"');
		if (p)
			*p = '\0';
          
		p = g_utf8_next_char(p);
	  
		/* the database name is not protected by "" */
		db_name = g_utf8_next_char(p);
		if (!db_name)
			break;
          
		p = g_utf8_strchr(db_name, -1, ' ');
		if (p)
			*p = '\0';

		p = g_utf8_next_char(p);
	  
		db_full = g_utf8_next_char(p);
		if (!db_full)
			break;
          
		if (db_full[0] == '\"')
			db_full = g_utf8_next_char(db_full);
          
		p = g_utf8_strchr(db_full, -1, '\"');
		if (p)
			*p = '\0';
          
		g_debug("{ word .= '%s', db_name .= '%s', db_full .= '%s' }\n",
			word, db_name, db_full);
		reslist_.push_back(DICT::Definition(word));          
		break;
	}
	default:
		if (!reslist_.empty() && strcmp(".", str))
			reslist_.back().data_ += std::string(str) + "\n";
		
		break;		
	}
	return true;
}


DictClient::DictClient(const char *host, int port)
{
	host_ = host;
	port_ = port;
	channel_ = NULL;
	source_id_ = 0;
	is_connected_ = false;
	last_index_ = 0;
}

DictClient::~DictClient()
{
	disconnect();
}


bool DictClient::connect()
{
	int sd = Socket::socket();

	if (sd == -1) {
		on_error_.emit("Can not create socket: " + Socket::getErrorMsg());
		return false;
	}

#ifdef _WIN32
	channel_ = g_io_channel_win32_new_socket(sd);
#else
	channel_ = g_io_channel_unix_new(sd);
#endif

/* RFC2229 mandates the usage of UTF-8, so we force this encoding */
	g_io_channel_set_encoding(channel_, "UTF-8", NULL);

	g_io_channel_set_line_term(channel_, "\r\n", 2);

/* make sure that the channel is non-blocking */
	int flags = g_io_channel_get_flags(channel_);
	flags |= G_IO_FLAG_NONBLOCK;
	GError *err = NULL;
	g_io_channel_set_flags(channel_, GIOFlags(flags), &err);
	if (err) {
		g_io_channel_unref(channel_);
		channel_ = NULL;
		on_error_.emit("Unable to set the channel as non-blocking: " +
			       std::string(err->message));
		g_error_free(err);
		return false;
	}

	if (!Socket::connect(sd, host_, port_)) {
		gchar *mes = g_strdup_printf("Can not connect to %s: %s\n",
					     host_.c_str(), Socket::getErrorMsg().c_str());
		on_error_.emit(mes);
		g_free(mes);
		return false;
	}

	source_id_ = g_io_add_watch(channel_, GIOCondition(G_IO_IN | G_IO_ERR),
				   on_io_event, this);

	return true;
}

void DictClient::disconnect()
{
	if (source_id_) {
		g_source_remove(source_id_);
		source_id_ = 0;
	}

	if (channel_) {
		g_io_channel_shutdown(channel_, TRUE, NULL);
		g_io_channel_unref(channel_);
		channel_ = NULL;
	}
	is_connected_ = false;
}

gboolean DictClient::on_io_event(GIOChannel *ch, GIOCondition cond,
				 gpointer user_data)
{
	DictClient *dict_client = static_cast<DictClient *>(user_data);

	g_assert(dict_client);

	if (!dict_client->channel_) {
		g_warning("No channel available\n");
		return FALSE;
	}

	if (cond & G_IO_ERR) {
		gchar *mes =
			g_strdup_printf("Connection failed to the dictionary server at %s:%d",
					dict_client->host_.c_str(), dict_client->port_);
		on_error_.emit(mes);
		g_free(mes);
		return FALSE;
	}

	GError *err = NULL;
	gsize term, len;
	gchar *line;
	GIOStatus res;

	for (;;) {
		if (!dict_client->channel_)
			break;
		res = g_io_channel_read_line(dict_client->channel_, &line,
					     &len, &term, &err);
		if (res == G_IO_STATUS_ERROR) {
			if (err) {
				on_error_.emit("Error while reading reply from server: " +
					       std::string(err->message));
				g_error_free(err);
			}
			dict_client->disconnect();

			return FALSE;
		}
		
		if (!len)
			break;

		//truncate the line terminator before parsing
		line[term] = '\0';
		int status_code = get_status_code(line);
		bool res = dict_client->parse(line, status_code);
		g_free(line);
		if (!res) {
			dict_client->disconnect();
			return FALSE;
		}		
	}

	return TRUE;
}

bool DictClient::parse(gchar *line, int status_code)
{
	g_debug("get %s\n", line);

	if (!is_connected_) {
		if (status_code == STATUS_CONNECT)
			is_connected_ = true;
		else if (status_code == STATUS_SERVER_DOWN ||
			 status_code == STATUS_SHUTDOWN) {
			gchar *mes =
				g_strdup_printf("Unable to connect to the "
						"dictionary server at '%s:%d'. "
						"The server replied with code"
						" %d (server down)",
						host_.c_str(), port_,
						status_code);
			on_error_.emit(mes);
			g_free(mes);
			return true;
		} else {
			gchar *mes =
				g_strdup_printf("Unable to parse the dictionary"
						" server reply: '%s'", line);
			on_error_.emit(mes);
			g_free(mes);
			return false;
		}
	}

	if (!cmd_.get())
		return true;

	if (cmd_->state_ == DICT::Cmd::START) {
			GIOStatus res =
				g_io_channel_write_chars(channel_,
							 cmd_->query().c_str(),
							 -1, NULL, NULL);
			if (res != G_IO_STATUS_NORMAL) {
				g_warning("Write IO error\n");
				return false;
			}
			/* force flushing of the write buffer */
			g_io_channel_flush(channel_, NULL);
			cmd_->state_ = DICT::Cmd::DATA;
			return true;
	}

	if (status_code == STATUS_OK || cmd_->state_ == DICT::Cmd::FINISH) {
		defmap_.clear();
		const DICT::DefList& res = cmd_->result();
		IndexList ilist(res.size());
		for (size_t i = 0; i < res.size(); ++i) {
			ilist[i] = last_index_;
			defmap_.insert(std::make_pair(last_index_++, res[i]));
		}
		on_lookup_end_.emit(ilist);
		last_index_ = 0;
		cmd_.reset(0);
		return true;
	}

	if (!cmd_->parse(line, status_code))
		return false;


	return true;
}

/* retrieve the status code from the server response line */
int DictClient::get_status_code(gchar *line)
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

void DictClient::lookup_simple(const gchar *word)
{
	if (!word || !*word) {
		on_lookup_end_.emit(IndexList());
		return;
	}

	if ((!channel_ || !source_id_) && !connect())
			return;
	
	cmd_.reset(new DefineCmd(word, "*"));
}

const gchar *DictClient::get_word(size_t index) const
{
	DefMap::const_iterator it = defmap_.find(index);

	if (it == defmap_.end())
		return NULL;
	return it->second.word_.c_str();
}

const gchar *DictClient::get_word_data(size_t index) const
{
	DefMap::const_iterator it = defmap_.find(index);

	if (it == defmap_.end())
		return NULL;
	return it->second.data_.c_str();
}
