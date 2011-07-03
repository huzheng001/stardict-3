/*
 * Copyright (C) 2006 Evgeniy <dushistov@mail.ru>
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

/*
 * Based on RFC 2229 and gnome dictionary source code
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <glib.h>

#include "sockets.h"

#include "dict_client.h"

sigc::signal<void, const std::string&> DictClient::on_error_;
sigc::signal<void, const DictClient::IndexList&>
DictClient::on_simple_lookup_end_;
sigc::signal<void, const DictClient::StringList&>
 DictClient::on_complex_lookup_end_;

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


void DICT::Cmd::send(GIOChannel *channel, GError *&err)
{
	g_assert(channel);

	GIOStatus res =
		g_io_channel_write_chars(channel,
					 query().c_str(),
					 -1, NULL, &err);
	if (res != G_IO_STATUS_NORMAL)
		return;

	/* force flushing of the write buffer */
	res = g_io_channel_flush(channel, &err);

	if (res != G_IO_STATUS_NORMAL)
		return;

	state_ = DICT::Cmd::DATA;
}

class DefineCmd : public DICT::Cmd {
public:
	DefineCmd(const gchar *database, const gchar *word) {
		char *quote_word = g_shell_quote(word);
		query_ = std::string("DEFINE ") + database + ' ' + quote_word +
			"\r\n";
		g_free(quote_word);
	}
	bool parse(gchar *str, int code);
};

class MatchCmd : public DICT::Cmd {
public:
	MatchCmd(const gchar *database, const gchar *strategy,
		 const gchar *word) :
		database_(database),
		strategy_(strategy),
		word_(word)
		{		
		}
	const std::string& query() {
		if (query_.empty()) {
			handle_word();
			char *quote_word = g_shell_quote(word_.c_str());
			query_ = "MATCH " + database_ + " " + strategy_ +
				' ' + quote_word + "\r\n";
			g_free(quote_word);
		}
		return DICT::Cmd::query();
	}
	bool parse(gchar *str, int code);
protected:
	std::string database_;
	std::string strategy_;
	std::string word_;

	virtual void handle_word() {}
};

class LevCmd : public MatchCmd {
public:
	LevCmd(const gchar *database, const gchar *word) :
		MatchCmd(database, "lev", word)
		{
		}
};

class RegexpCmd : public MatchCmd {
public:
	RegexpCmd(const gchar *database, const gchar *word) :
		MatchCmd(database, "re", word) 
		{
		}
protected:
	void handle_word();
};

void RegexpCmd::handle_word()
{
	std::string newword;
	std::string::const_iterator it;

	for (it = word_.begin(); it != word_.end(); ++it)
		if (*it == '*')
			newword += ".*";
		else
			newword += *it;

	word_ = "^" + newword;
}

bool MatchCmd::parse(gchar *str, int code)
{
	if (code == STATUS_N_MATCHES_FOUND) { 
          gchar *p = g_utf8_strchr(str, -1, ' ');

          if (p)
            p = g_utf8_next_char (p);
	  g_debug("server replied: %d matches found\n", atoi (p));
        } else if (0 == strcmp (str, "."))
		state_ = FINISH;
	else {
          gchar *word, *db_name, *p;

          db_name = str;
          if (!db_name)
		  return false;

          p = g_utf8_strchr(db_name, -1, ' ');
          if (p)
            *p = '\0';

          word = g_utf8_next_char (p);

          if (word[0] == '\"')
		  word = g_utf8_next_char (word);

          p = g_utf8_strchr (word, -1, '\"');
          if (p)
            *p = '\0';
          
          reslist_.push_back(DICT::Definition(word));
        }

	return true;
}

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
	sd_ = -1;
	channel_ = NULL;
	source_id_ = 0;
	is_connected_ = false;
	last_index_ = 0;
}

DictClient::~DictClient()
{
	disconnect();
}

void DictClient::connect()
{
    Socket::resolve(host_, this, on_resolved);
}

void DictClient::on_resolved(gpointer data, struct hostent *ret)
{
	DictClient *oDictClient = (DictClient *)data;
	oDictClient->sd_ = Socket::socket();

	if (oDictClient->sd_ == -1) {
		on_error_.emit("Can not create socket: " + Socket::get_error_msg());
		return;
	}

#ifdef _WIN32
	oDictClient->channel_ = g_io_channel_win32_new_socket(oDictClient->sd_);
#else
	oDictClient->channel_ = g_io_channel_unix_new(oDictClient->sd_);
#endif

/* RFC2229 mandates the usage of UTF-8, so we force this encoding */
	g_io_channel_set_encoding(oDictClient->channel_, "UTF-8", NULL);

	g_io_channel_set_line_term(oDictClient->channel_, "\r\n", 2);

/* make sure that the channel is non-blocking */
	int flags = g_io_channel_get_flags(oDictClient->channel_);
	flags |= G_IO_FLAG_NONBLOCK;
	GError *err = NULL;
	g_io_channel_set_flags(oDictClient->channel_, GIOFlags(flags), &err);
	if (err) {
		g_io_channel_unref(oDictClient->channel_);
		oDictClient->channel_ = NULL;
		on_error_.emit("Unable to set the channel as non-blocking: " +
			       std::string(err->message));
		g_error_free(err);
		return;
	}

	if (!Socket::connect(oDictClient->sd_, ret, oDictClient->port_)) {
		gchar *mes = g_strdup_printf("Can not connect to %s: %s\n",
					     oDictClient->host_.c_str(), Socket::get_error_msg().c_str());
		on_error_.emit(mes);
		g_free(mes);
		return;
	}

	oDictClient->source_id_ = g_io_add_watch(oDictClient->channel_, GIOCondition(G_IO_IN | G_IO_ERR),
				   on_io_event, oDictClient);
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
	if (sd_ != -1) {
		Socket::close(sd_);
		sd_ = -1;
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

	if (!cmd_.get()) {
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

	bool success = false;

	switch (status_code) {
	case STATUS_BAD_PARAMETERS:
	{
		gchar *mes = g_strdup_printf("Bad parameters for command '%s'",
					     cmd_->query().c_str());
		on_error_.emit(mes);
		g_free(mes);
		cmd_->state_ = DICT::Cmd::FINISH;
		break;
	}
	case STATUS_BAD_COMMAND:
	{
		gchar *mes = g_strdup_printf("Bad command '%s'",
					     cmd_->query().c_str());
		on_error_.emit(mes);
		g_free(mes);
		cmd_->state_ = DICT::Cmd::FINISH;
		break;
	}
	default:
		success = true;
		break;
	}

	if (cmd_->state_ == DICT::Cmd::START) {
		GError *err = NULL;
		cmd_->send(channel_, err);
		if (err) {
			on_error_.emit(err->message);
			g_error_free(err);
			return false;
		}
		return true;
	}

	if (status_code == STATUS_OK || cmd_->state_ == DICT::Cmd::FINISH ||
	    status_code == STATUS_NO_MATCH ||
	    status_code == STATUS_BAD_DATABASE ||
	    status_code == STATUS_BAD_STRATEGY ||
	    status_code == STATUS_NO_DATABASES_PRESENT ||
	    status_code == STATUS_NO_STRATEGIES_PRESENT) {
		defmap_.clear();
		const DICT::DefList& res = cmd_->result();
		if (simple_lookup_) {
			IndexList ilist(res.size());
			for (size_t i = 0; i < res.size(); ++i) {
				ilist[i] = last_index_;
				defmap_.insert(std::make_pair(last_index_++, res[i]));
			}
			last_index_ = 0;
			cmd_.reset(0);
			disconnect();
			on_simple_lookup_end_.emit(ilist);
		} else {
			StringList slist;
			for (size_t i = 0; i < res.size(); ++i)
				slist.push_back(res[i].word_);
			last_index_ = 0;
			cmd_.reset(0);
			disconnect();
			on_complex_lookup_end_.emit(slist);
		}

		return success;
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
	simple_lookup_ = true;

	if (!word || !*word) {
		on_simple_lookup_end_.emit(IndexList());
		return;
	}

	if (!channel_ || !source_id_)
			return;
    connect();

	cmd_.reset(new DefineCmd("*", word));
}

void DictClient::lookup_with_rule(const gchar *word)
{
	simple_lookup_ = false;

	if (!word || !*word) {
		on_complex_lookup_end_.emit(StringList());
		return;
	}

	if (!channel_ || !source_id_)
			return;
    connect();

	cmd_.reset(new RegexpCmd("*", word));
}

void DictClient::lookup_with_fuzzy(const gchar *word)
{
	simple_lookup_ = false;

	if (!word || !*word) {
		on_complex_lookup_end_.emit(StringList());
		return;
	}

	if (!channel_ || !source_id_)
			return;
    connect();

	cmd_.reset(new LevCmd("*", word));
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

