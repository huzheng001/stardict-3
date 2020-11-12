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

#ifndef _STARDICT_CLIENT_HPP_
#define _STARDICT_CLIENT_HPP_

#include <glib.h>
#include <list>
#include <vector>

#ifndef _WIN32
#  include <netdb.h>
#else
typedef unsigned long in_addr_t;
#endif

#include "rsa.h"
#include "stardict-sigc++.h"


namespace STARDICT {
	enum {
		CMD_CLIENT,
		CMD_AUTH,
		CMD_LOOKUP,
		CMD_PREVIOUS,
		CMD_NEXT,
		//CMD_QUERY,
		CMD_SELECT_QUERY,
		CMD_SMART_QUERY,
		CMD_DEFINE,
		CMD_REGISTER,
		CMD_CHANGE_PASSWD,
		CMD_SET_DICT_MASK,
		CMD_GET_DICT_MASK,
		//CMD_SET_COLLATE_FUNC,
		//CMD_GET_COLLATE_FUNC,
		//CMD_SET_LANGUAGE,
		//CMD_GET_LANGUAGE,
		//CMD_SET_EMAIL,
		//CMD_GET_EMAIL,
		//CMD_FROMTO,
		//CMD_TMP_DICTMASK,
		//CMD_DICTS_LIST,
		CMD_MAX_DICT_COUNT,
		CMD_DIR_INFO,
		CMD_DICT_INFO,
		//CMD_USER_LEVEL,
		//CMD_GET_USER_LEVEL,
		CMD_GET_ADINFO,
		CMD_QUIT,
	};
	struct LookupResponse {
		struct DictResponse {
			DictResponse();
			~DictResponse();
			char *oword;
			struct DictResult {
				DictResult();
				~DictResult();
				char *bookname;
				struct WordResult {
					WordResult();
					~WordResult();
					char *word;
					std::list<char *> datalist;
				};
				std::list<struct WordResult *> word_result_list;
			};
			std::list<struct DictResult *> dict_result_list;
		};
		~LookupResponse();
		struct DictResponse dict_response;
		enum ListType {
			ListType_None,
			ListType_List,
			ListType_Rule_List,
			ListType_Regex_List,
			ListType_Fuzzy_List,
			ListType_Tree,
		};
		ListType listtype;
		struct WordTreeElement {
			char *bookname;
			std::list<char *> wordlist;
		};
		union {
			std::list<char *> *wordlist;
			std::list<WordTreeElement *> *wordtree;
		};
	};
	class Cmd {
	public:
		int command;
		struct AuthInfo {
			std::string user;
			std::string md5saltpasswd;
		};
		union {
			char *data;
			AuthInfo *auth;
		};
		int reading_status;
		unsigned int seq;
		union {
			struct LookupResponse *lookup_response;
			std::list<char *> *wordlist_response;
		};
		Cmd(int cmd, ...);
		~Cmd();
	private:
		static unsigned int next_seq;
	};
};

class StarDictCache {
public:
	StarDictCache();
	~StarDictCache();
	char *get_cache_str(const char *key);
	void save_cache_str(const char *key, char *str);
	void clean_cache_str(const char *key);
	STARDICT::LookupResponse *get_cache_lookup_response(const char *key);
	void save_cache_lookup_response(const char *key, STARDICT::LookupResponse *lookup_response);
	void clean_cache_lookup_response();
	void clean_all_cache();
private:
	static const unsigned int str_pool_size = 30;
	size_t cur_str_pool_pos;
	struct StrElement {
		std::string key;
		char *data;
	};
	std::vector<StrElement *> str_pool;

	static const unsigned int lookup_response_pool_size = 60;
	size_t cur_lookup_response_pool_pos;
	struct LookupResponseElement {
		std::string key;
		struct STARDICT::LookupResponse *lookup_response;
	};
	std::vector<LookupResponseElement *> lookup_response_pool;
};

class StarDictClient : private StarDictCache {
public:
	static sigc::signal<void, const char *> on_error_;
	static sigc::signal<void, const struct STARDICT::LookupResponse *, unsigned int> on_lookup_end_;
	static sigc::signal<void, const struct STARDICT::LookupResponse *, unsigned int> on_floatwin_lookup_end_;
	static sigc::signal<void, const char *> on_register_end_;
	static sigc::signal<void, const char *> on_changepassword_end_;
	static sigc::signal<void, const char *> on_getdictmask_end_;
	static sigc::signal<void, const char *> on_getadinfo_end_;
	static sigc::signal<void, const char *> on_dirinfo_end_;
	static sigc::signal<void, const char *> on_dictinfo_end_;
	static sigc::signal<void, int> on_maxdictcount_end_;
	static sigc::signal<void, std::list<char *> *> on_previous_end_;
	static sigc::signal<void, std::list<char *> *> on_next_end_;

	int RSA_Public_Key_e[RSA_MAX];
	int RSA_Public_Key_n[RSA_MAX];

	StarDictClient();
	~StarDictClient();

	void set_server(const char *host, int port = 2629);
	void set_auth(const char *user, const char *md5saltpasswd);
	bool try_cache(STARDICT::Cmd *c);
	void send_commands(int num, ...);
	void try_cache_or_send_commands(int num, ...);
private:
	int sd_;
	GIOChannel *channel_;
	guint in_source_id_;
	guint out_source_id_;
	std::string host_;
	int port_;
	bool host_resolved;
	in_addr_t sa;
	std::string user_;
	std::string md5saltpasswd_;
	bool is_connected_;
	bool waiting_banner_;
	std::list<STARDICT::Cmd *> cmdlist;
	struct reply {
		std::string daemonStamp;
	} cmd_reply;
	enum ReadType {
		READ_LINE,
		READ_STRING,
		READ_SIZE,
	} reading_type_;
	char *size_data;
	gsize size_count;
	gsize size_left;

	void clean_command();
	void request_command();
	void disconnect();
	static gboolean on_io_in_event(GIOChannel *, GIOCondition, gpointer);
	static gboolean on_io_out_event(GIOChannel *, GIOCondition, gpointer);
	void connect();
	static void on_resolved(gpointer data, bool resolved, in_addr_t sa);
	static void on_connected(gpointer data, bool succeeded);
	void write_str(const char *str, GError **err);
	bool parse(gchar *line);
	int parse_banner(gchar *line);
	int parse_command_client(gchar *line);
	int parse_command_auth(gchar *line);
	int parse_command_register(gchar *line);
	int parse_command_changepassword(gchar *line);
	int parse_command_setdictmask(gchar *line);
	int parse_command_getdictmask(STARDICT::Cmd* cmd, gchar *line);
	int parse_command_getadinfo(STARDICT::Cmd* cmd, gchar *line);
	int parse_command_dirinfo(STARDICT::Cmd* cmd, gchar *line);
	int parse_command_dictinfo(STARDICT::Cmd* cmd, gchar *line);
	int parse_command_maxdictcount(STARDICT::Cmd* cmd, gchar *line);
	int parse_command_quit(gchar *line);
	int parse_dict_result(STARDICT::Cmd* cmd, gchar *buf);
	int parse_wordlist(STARDICT::Cmd* cmd, gchar *buf);
};

#endif
