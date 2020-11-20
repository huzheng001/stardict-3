/*
 * Copyright (C) 2006 Hu Zheng <huzheng001@gmail.com>
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

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <glib.h>
#include <glib/gi18n.h>

#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <stdlib.h>
#include <stdio.h>
#include "sockets.h"
#include "md5.h"
#include "rsa.h"
#include "utils.h"

#include "stardict_client.h"


// StarDict dictionary server document:
// http://stardict-4.sourceforge.net/StarDict-Protocol_v04

// http://stardictd.sourceforge.net



#define PROTOCOL_VERSION "0.4"

#define CODE_HELLO                   220 /* text msg-id */
#define CODE_GOODBYE                 221 /* Closing Connection */
#define CODE_OK                      250 /* ok */
#define CODE_TEMPORARILY_UNAVAILABLE 420 /* server unavailable */
#define CODE_SYNTAX_ERROR            500 /* syntax, command not recognized */
#define CODE_DENIED                  521
#define CODE_DICTMASK_NOTSET         522
#define CODE_USER_NOT_REGISTER       523


unsigned int STARDICT::Cmd::next_seq = 1;

sigc::signal<void, const char *> StarDictClient::on_error_;
sigc::signal<void, const struct STARDICT::LookupResponse *, unsigned int> StarDictClient::on_lookup_end_;
sigc::signal<void, const struct STARDICT::LookupResponse *, unsigned int> StarDictClient::on_floatwin_lookup_end_;
sigc::signal<void, const char *> StarDictClient::on_register_end_;
sigc::signal<void, const char *> StarDictClient::on_changepassword_end_;
sigc::signal<void, const char *> StarDictClient::on_getdictmask_end_;
sigc::signal<void, const char *> StarDictClient::on_getadinfo_end_;
sigc::signal<void, const char *> StarDictClient::on_dirinfo_end_;
sigc::signal<void, const char *> StarDictClient::on_dictinfo_end_;
sigc::signal<void, int> StarDictClient::on_maxdictcount_end_;
sigc::signal<void, std::list<char *> *> StarDictClient::on_previous_end_;
sigc::signal<void, std::list<char *> *> StarDictClient::on_next_end_;

static void arg_escape(std::string &earg, const char *arg)
{
    earg.clear();
    while (*arg) {
        if (*arg=='\\') {
            earg+="\\\\";
        } else if (*arg==' ') {
            earg+="\\ ";
        } else if (*arg=='\n') {
            earg+="\\n";
        } else {
            earg+=*arg;
        }
        arg++;
    }
}

STARDICT::Cmd::Cmd(int cmd, ...)
{
	this->seq = this->next_seq;
	this->next_seq++;
	this->reading_status = 0;
	this->command = cmd;
	va_list    ap;
	va_start( ap, cmd );
	switch (cmd) {
	case CMD_CLIENT:
	{
		const char *client_name = va_arg( ap, const char * );
		std::string earg1, earg2;
		arg_escape(earg1, PROTOCOL_VERSION);
		arg_escape(earg2, client_name);
		this->data = g_strdup_printf("client %s %s\n", earg1.c_str(), earg2.c_str());
		break;
	}
	case CMD_REGISTER:
	{
		const char *user = va_arg( ap, const char * );
		const char *passwd = va_arg( ap, const char * );
		const char *email = va_arg( ap, const char * );
		int *RSA_Public_Key_e = va_arg( ap, int * );
		int *RSA_Public_Key_n = va_arg( ap, int * );

		std::string passwd1 = passwd;
		std::vector<unsigned char> v;
		string_to_vector(passwd1, v);
		std::vector<unsigned char> v2;
		rsa_encrypt(v, v2, RSA_Public_Key_e, RSA_Public_Key_n);

		std::string v2_passwd;
		vector_to_string(v2, v2_passwd);
		gchar *str_base64 = g_base64_encode((const guchar *)(v2_passwd.c_str()), v2_passwd.length());

		std::string earg1, earg2, earg3;
		arg_escape(earg1, user);
		arg_escape(earg2, str_base64);
		g_free(str_base64);
		arg_escape(earg3, email);
		this->data = g_strdup_printf("register %s %s %s\n", earg1.c_str(), earg2.c_str(), earg3.c_str());
		break;
	}
	case CMD_CHANGE_PASSWD:
	{
		const char *user = va_arg( ap, const char * );
		const char *old_passwd = va_arg( ap, const char * );
		const char *new_passwd = va_arg( ap, const char * );
		int *RSA_Public_Key_e = va_arg( ap, int * );
		int *RSA_Public_Key_n = va_arg( ap, int * );

		std::string old_passwd1 = old_passwd;
		std::vector<unsigned char> v;
		string_to_vector(old_passwd1, v);
		std::vector<unsigned char> v2;
		rsa_encrypt(v, v2, RSA_Public_Key_e, RSA_Public_Key_n);

		std::string v2_passwd;
		vector_to_string(v2, v2_passwd);
		gchar *str_base64 = g_base64_encode((const guchar *)(v2_passwd.c_str()), v2_passwd.length());

		std::string new_passwd1 = new_passwd;
		string_to_vector(new_passwd1, v);
		rsa_encrypt(v, v2, RSA_Public_Key_e, RSA_Public_Key_n);

		vector_to_string(v2, v2_passwd);
		gchar *str_base64_2 = g_base64_encode((const guchar *)(v2_passwd.c_str()), v2_passwd.length());

		std::string earg1, earg2, earg3;
		arg_escape(earg1, user);
		arg_escape(earg2, str_base64);
		g_free(str_base64);
		arg_escape(earg3, str_base64_2);
		g_free(str_base64_2);
		this->data = g_strdup_printf("change_password %s %s %s\n", earg1.c_str(), earg2.c_str(), earg3.c_str());
		break;
	}
	case CMD_AUTH:
        	this->auth = new AuthInfo();
		this->auth->user = va_arg( ap, const char * );
		this->auth->md5saltpasswd = va_arg( ap, const char * );
		break;
	case CMD_LOOKUP:
	{
		std::string earg;
		arg_escape(earg, va_arg( ap, const char * ));
		this->data = g_strdup_printf("lookup %s 30\n", earg.c_str());
		this->lookup_response = NULL;
		break;
	}
	case CMD_PREVIOUS:
	{
		std::string earg;
		arg_escape(earg, va_arg( ap, const char * ));
		this->data = g_strdup_printf("previous %s 15\n", earg.c_str());
        this->wordlist_response = NULL;
		break;
	}
	case CMD_NEXT:
	{
		std::string earg;
		arg_escape(earg, va_arg( ap, const char * ));
		this->data = g_strdup_printf("next %s 30\n", earg.c_str());
		this->wordlist_response = NULL;
		break;
	}
	/*case CMD_QUERY:
	{
		std::string earg;
		arg_escape(earg, va_arg( ap, const char * ));
		this->data = g_strdup_printf("query %s\n", earg.c_str());
		this->lookup_response = NULL;
		break;
	}*/
	case CMD_SELECT_QUERY:
	{
		std::string earg;
		arg_escape(earg, va_arg( ap, const char * ));
		this->data = g_strdup_printf("selectquery %s\n", earg.c_str());
		this->lookup_response = NULL;
		break;
	}
	case CMD_SMART_QUERY:
	{
		std::string earg;
		arg_escape(earg, va_arg( ap, const char * ));
		int BeginPos = va_arg( ap, int );
		this->data = g_strdup_printf("smartquery %s %d\n", earg.c_str(), BeginPos);
		this->lookup_response = NULL;
		break;
	}
	case CMD_DEFINE:
	{
		std::string earg;
		arg_escape(earg, va_arg( ap, const char * ));
		this->data = g_strdup_printf("define %s\n", earg.c_str());
		this->lookup_response = NULL;
		break;
	}
	case CMD_SET_DICT_MASK:
	{
		std::string earg;
		arg_escape(earg, va_arg( ap, const char * ));
		this->data = g_strdup_printf("setdictmask %s\n", earg.c_str());
		break;
	}
	case CMD_GET_DICT_MASK:
		this->data = g_strdup("getdictmask\n");
		break;
	/*case CMD_SET_COLLATE_FUNC:
	{
		std::string earg;
		arg_escape(earg, va_arg( ap, const char * ));
		this->data = g_strdup_printf("setcollatefunc %s\n", earg.c_str());
		break;
	}
	case CMD_GET_COLLATE_FUNC:
		this->data = g_strdup("getcollatefunc\n");
		break;
	case CMD_SET_LANGUAGE:
	{
		std::string earg;
		arg_escape(earg, va_arg( ap, const char * ));
		this->data = g_strdup_printf("setlanguage %s\n", earg.c_str());
		break;
	}
	case CMD_GET_LANGUAGE:
		this->data = g_strdup("getlanguage\n");
		break;
	case CMD_SET_EMAIL:
	{
		std::string earg;
		arg_escape(earg, va_arg( ap, const char * ));
		this->data = g_strdup_printf("setemail %s\n", earg.c_str());
		break;
	}
	case CMD_GET_EMAIL:
		this->data = g_strdup("getemail\n");
		break;
	case CMD_FROMTO:
		this->data = g_strdup("fromto\n");
		break;
	case CMD_TMP_DICTMASK:
	{
		std::string earg;
		arg_escape(earg, va_arg( ap, const char * ));
		this->data = g_strdup_printf("tmpdictmask %s\n", earg.c_str());
		break;
	}
	case CMD_DICTS_LIST:
	{
		std::string earg;
		arg_escape(earg, va_arg( ap, const char * ));
		this->data = g_strdup_printf("dictslist %s\n", earg.c_str());
		break;
	}
	case CMD_GET_USER_LEVEL:
		this->data = g_strdup("getuserlevel\n");
		break;*/
	case CMD_MAX_DICT_COUNT:
		this->data = g_strdup("maxdictcount\n");
		break;
	case CMD_DIR_INFO:
	{
		std::string earg;
		arg_escape(earg, va_arg( ap, const char * ));
		this->data = g_strdup_printf("dirinfo %s\n", earg.c_str());
		break;
	}
	case CMD_DICT_INFO:
	{
		std::string earg;
		arg_escape(earg, va_arg( ap, const char * ));
		this->data = g_strdup_printf("dictinfo %s\n", earg.c_str());
		break;
	}
	/*case CMD_USER_LEVEL:
	{
		std::string earg1, earg2, earg3;
		arg_escape(earg1, va_arg( ap, const char * ));
		arg_escape(earg2, va_arg( ap, const char * ));
		arg_escape(earg3, va_arg( ap, const char * ));
		this->data = g_strdup_printf("userlevel %s %s %s\n", earg1.c_str(), earg2.c_str(), earg3.c_str());
		break;
	}*/
	case CMD_GET_ADINFO:
		this->data = g_strdup("getadinfo\n");
		break;
	case CMD_QUIT:
		this->data = g_strdup("quit\n");
		break;
	}
	va_end( ap );
}

STARDICT::LookupResponse::DictResponse::DictResponse()
{
	oword = NULL;
}

STARDICT::LookupResponse::DictResponse::~DictResponse()
{
	g_free(oword);
	for (std::list<DictResult *>::iterator i = dict_result_list.begin(); i != dict_result_list.end(); ++i) {
		delete *i;
	}
}

STARDICT::LookupResponse::DictResponse::DictResult::DictResult()
{
	bookname = NULL;
}

STARDICT::LookupResponse::DictResponse::DictResult::~DictResult()
{
	g_free(bookname);
	for (std::list<WordResult *>::iterator i = word_result_list.begin(); i != word_result_list.end(); ++i) {
		delete *i;
	}
}

STARDICT::LookupResponse::DictResponse::DictResult::WordResult::WordResult()
{
	word = NULL;
}

STARDICT::LookupResponse::DictResponse::DictResult::WordResult::~WordResult()
{
	g_free(word);
	for (std::list<char *>::iterator i = datalist.begin(); i != datalist.end(); ++i) {
		g_free(*i);
	}
}

STARDICT::LookupResponse::~LookupResponse()
{
    if (listtype == ListType_List) {
    	for (std::list<char *>::iterator i = wordlist->begin(); i != wordlist->end(); ++i) {
	    	g_free(*i);
	    }
        delete wordlist;
    } else if (listtype == ListType_Tree) {
        for (std::list<WordTreeElement *>::iterator i = wordtree->begin(); i != wordtree->end(); ++i) {
            g_free((*i)->bookname);
            for (std::list<char *>::iterator j = (*i)->wordlist.begin(); j != (*i)->wordlist.end(); ++j) {
                g_free(*j);
            }
            delete *i;
        }
        delete wordtree;
    }
}

STARDICT::Cmd::~Cmd()
{
    if (this->command == CMD_AUTH) {
        delete this->auth;
    } else {
        g_free(this->data);
    }
    if (this->command == CMD_LOOKUP || this->command == CMD_DEFINE || this->command == CMD_SELECT_QUERY || this->command == CMD_SMART_QUERY) {
        delete this->lookup_response;
    } else if (this->command == CMD_PREVIOUS || this->command == CMD_NEXT) {
        if (this->wordlist_response) {
            for (std::list<char *>::iterator i = this->wordlist_response->begin(); i != this->wordlist_response->end(); ++i) {
                g_free(*i);
            }
            delete this->wordlist_response;
        }
    }
}

StarDictCache::StarDictCache()
{
    str_pool.resize(str_pool_size);
    for (size_t i = 0; i< str_pool_size; i++) {
        str_pool[i] = NULL;
    }
    cur_str_pool_pos = 0;

    lookup_response_pool.resize(lookup_response_pool_size);
    for (size_t i = 0; i< lookup_response_pool_size; i++) {
        lookup_response_pool[i] = NULL;
    }
    cur_lookup_response_pool_pos = 0;
}

StarDictCache::~StarDictCache()
{
    clean_all_cache();
}

void StarDictCache::clean_all_cache()
{
    for (std::vector<StrElement *>::iterator i = str_pool.begin(); i != str_pool.end(); ++i) {
        if (*i) {
            g_free((*i)->data);
            delete *i;
            *i = NULL;
        }
    }
    clean_cache_lookup_response();
}

void StarDictCache::clean_cache_lookup_response()
{
    for (std::vector<LookupResponseElement *>::iterator i = lookup_response_pool.begin(); i != lookup_response_pool.end(); ++i) {
        if (*i) {
            delete ((*i)->lookup_response);
            delete *i;
            *i = NULL;
        }
    }
}

char *StarDictCache::get_cache_str(const char *key_str)
{
    for (std::vector<StrElement *>::iterator i = str_pool.begin(); i != str_pool.end(); ++i) {
        if (*i) {
            if ((*i)->key == key_str)
                return (*i)->data;
        }
    }
    return NULL;
}

STARDICT::LookupResponse *StarDictCache::get_cache_lookup_response(const char *key_str)
{
    for (std::vector<LookupResponseElement *>::iterator i = lookup_response_pool.begin(); i != lookup_response_pool.end(); ++i) {
        if (*i) {
            if ((*i)->key == key_str)
                return (*i)->lookup_response;
        }
    }
    return NULL;
}

void StarDictCache::clean_cache_str(const char *key_str)
{
    for (std::vector<StrElement *>::iterator i = str_pool.begin(); i != str_pool.end(); ++i) {
        if (*i) {
            if ((*i)->key == key_str) {
                g_free((*i)->data);
                delete *i;
                *i = NULL;
                //return;
            }
        }
    }
}

void StarDictCache::save_cache_str(const char *key_str, char *data)
{
    if (str_pool[cur_str_pool_pos]) {
        g_free(str_pool[cur_str_pool_pos]->data);
        delete str_pool[cur_str_pool_pos];
    }
    str_pool[cur_str_pool_pos] = new StrElement();
    str_pool[cur_str_pool_pos]->key = key_str;
    str_pool[cur_str_pool_pos]->data = data;
    cur_str_pool_pos++;
    if (cur_str_pool_pos == str_pool_size)
        cur_str_pool_pos = 0;
}

void StarDictCache::save_cache_lookup_response(const char *key_str, STARDICT::LookupResponse *lookup_response)
{
    if (lookup_response_pool[cur_lookup_response_pool_pos]) {
        delete lookup_response_pool[cur_lookup_response_pool_pos]->lookup_response;
        delete lookup_response_pool[cur_lookup_response_pool_pos];
    }
    lookup_response_pool[cur_lookup_response_pool_pos] = new LookupResponseElement();
    lookup_response_pool[cur_lookup_response_pool_pos]->key = key_str;
    lookup_response_pool[cur_lookup_response_pool_pos]->lookup_response = lookup_response;
    cur_lookup_response_pool_pos++;
    if (cur_lookup_response_pool_pos == lookup_response_pool_size)
        cur_lookup_response_pool_pos = 0;
}

StarDictClient::StarDictClient()
{
	sd_ = -1;
    channel_ = NULL;
    in_source_id_ = 0;
    out_source_id_ = 0;
    is_connected_ = false;
}

StarDictClient::~StarDictClient()
{
    disconnect();
}

void StarDictClient::set_server(const char *host, int port)
{
    if (host_ != host || port_ != port) {
        host_ = host;
        port_ = port;
	host_resolved = false;
        clean_all_cache();
    }
}

void StarDictClient::set_auth(const char *user, const char *md5saltpasswd)
{
    if (user_ != user || md5saltpasswd_ != md5saltpasswd) {
        user_ = user;
        md5saltpasswd_ = md5saltpasswd;
        clean_all_cache();
    }
}

bool StarDictClient::try_cache(STARDICT::Cmd *c)
{
    if (c->command == STARDICT::CMD_LOOKUP || c->command == STARDICT::CMD_DEFINE || c->command == STARDICT::CMD_SELECT_QUERY || c->command == STARDICT::CMD_SMART_QUERY) {
        STARDICT::LookupResponse *res = get_cache_lookup_response(c->data);
        if (res) {
            if (c->command == STARDICT::CMD_LOOKUP || c->command == STARDICT::CMD_DEFINE)
                on_lookup_end_.emit(res, 0);
            else if (c->command == STARDICT::CMD_SELECT_QUERY || c->command == STARDICT::CMD_SMART_QUERY)
                on_floatwin_lookup_end_.emit(res, 0);
            delete c;
            return true;
        } else {
            return false;
        }
    }
    if (c->command == STARDICT::CMD_PREVIOUS || c->command == STARDICT::CMD_NEXT) {
        // Not implemented yet.
        return false;
    }
    char *data = get_cache_str(c->data);
    if (data) {
        switch (c->command) {
            case STARDICT::CMD_DIR_INFO:
                on_dirinfo_end_.emit(data);
                break;
            case STARDICT::CMD_DICT_INFO:
                on_dictinfo_end_.emit(data);
                break;
            case STARDICT::CMD_GET_DICT_MASK:
                on_getdictmask_end_.emit(data);
                break;
            case STARDICT::CMD_MAX_DICT_COUNT:
                on_maxdictcount_end_.emit(atoi(data));
                break;
            case STARDICT::CMD_GET_ADINFO:
                on_getadinfo_end_.emit(data);
                break;
        }
        delete c;
        return true;
    } else {
        return false;
    }
}

void StarDictClient::send_commands(int num, ...)
{
    STARDICT::Cmd *c;
    if (!is_connected_) {
#ifdef _WIN32
        c = new STARDICT::Cmd(STARDICT::CMD_CLIENT, "StarDict Windows");
#else
        c = new STARDICT::Cmd(STARDICT::CMD_CLIENT, "StarDict Linux");
#endif
        cmdlist.push_back(c);
        if (!user_.empty() && !md5saltpasswd_.empty()) {
            c = new STARDICT::Cmd(STARDICT::CMD_AUTH, user_.c_str(), md5saltpasswd_.c_str());
            cmdlist.push_back(c);
        }
    }
    va_list    ap;
    va_start( ap, num);
    for (int i = 0; i< num; i++) {
        c = va_arg( ap, STARDICT::Cmd *);
	    cmdlist.push_back(c);
    }
    va_end( ap );
    if (!is_connected_) {
        waiting_banner_ = true;
        connect();
    }
}

void StarDictClient::try_cache_or_send_commands(int num, ...)
{
    STARDICT::Cmd *c;
    std::list<STARDICT::Cmd *> send_cmdlist;
    va_list    ap;
    va_start( ap, num);
    for (int i = 0; i< num; i++) {
        c = va_arg( ap, STARDICT::Cmd *);
        if (!try_cache(c)) {
            send_cmdlist.push_back(c);
        }
    }
    va_end( ap );
    if (send_cmdlist.empty())
        return;

    if (!is_connected_) {
#ifdef _WIN32
        c = new STARDICT::Cmd(STARDICT::CMD_CLIENT, "StarDict Windows");
#else
        c = new STARDICT::Cmd(STARDICT::CMD_CLIENT, "StarDict Linux");
#endif
        cmdlist.push_back(c);
        if (!user_.empty() && !md5saltpasswd_.empty()) {
            c = new STARDICT::Cmd(STARDICT::CMD_AUTH, user_.c_str(), md5saltpasswd_.c_str());
            cmdlist.push_back(c);
        }
    }
    for (std::list<STARDICT::Cmd *>::iterator i = send_cmdlist.begin(); i!= send_cmdlist.end(); ++i) {
	    cmdlist.push_back(*i);
    }
    if (!is_connected_) {
        waiting_banner_ = true;
        connect();
    }
}

void StarDictClient::write_str(const char *str, GError **err)
{
    int len = strlen(str);
    int left_byte = len;
    GIOStatus res;
    gsize bytes_written;
    while (left_byte) {
        res = g_io_channel_write_chars(channel_, str+(len - left_byte), left_byte, &bytes_written, err);
        if (res == G_IO_STATUS_ERROR) {
            disconnect();
            return;
        }
        left_byte -= bytes_written;
    }
    res = g_io_channel_flush(channel_, err);
    if (res == G_IO_STATUS_ERROR) {
        disconnect();
    }
	if (out_source_id_ == 0)
		out_source_id_ = g_io_add_watch(channel_, GIOCondition(G_IO_OUT), on_io_out_event, this);
}

void StarDictClient::request_command()
{
    reading_type_ = READ_LINE;
    if (cmdlist.empty()) {
        cmdlist.push_back(new STARDICT::Cmd(STARDICT::CMD_QUIT));
    }
    STARDICT::Cmd *c = cmdlist.front();
	switch (c->command) {
        case STARDICT::CMD_AUTH:
		{
			struct MD5Context ctx;
			unsigned char digest[16];
			char hex[33];
			int i;
			MD5Init(&ctx);
			MD5Update(&ctx, (const unsigned char*)cmd_reply.daemonStamp.c_str(), cmd_reply.daemonStamp.length());
			MD5Update(&ctx, (const unsigned char*)(c->auth->md5saltpasswd.c_str()), c->auth->md5saltpasswd.length());
			MD5Final(digest, &ctx );
			for (i = 0; i < 16; i++)
				sprintf( hex+2*i, "%02x", digest[i] );
			hex[32] = '\0';
			std::string earg1, earg2;
			arg_escape(earg1, c->auth->user.c_str());
			arg_escape(earg2, hex);
			char *data = g_strdup_printf("auth %s %s\n", earg1.c_str(), earg2.c_str());
            GError *err = NULL;
            write_str(data, &err);
			g_free(data);
			if (err) {
                on_error_.emit(err->message);
                g_error_free(err);
				return;
			}
			break;
		}
		default:
        {
            GError *err = NULL;
            write_str(c->data, &err);
			if (err) {
                on_error_.emit(err->message);
                g_error_free(err);
				return;
            }
			break;
        }
	}
	return;
}

void StarDictClient::clean_command()
{
	for (std::list<STARDICT::Cmd *>::iterator i=cmdlist.begin(); i!=cmdlist.end(); ++i) {
		delete *i;
	}
	cmdlist.clear();
}

void StarDictClient::connect()
{
    if (host_resolved) {
	    on_resolved(this, true, sa);
    } else {
        Socket::resolve(host_, this, on_resolved);
    }
}

void StarDictClient::on_resolved(gpointer data, bool resolved, in_addr_t sa_)
{
    StarDictClient *oStarDictClient = (StarDictClient *)data;
    if (!resolved) {
	static bool showed_once = false;
	if (!showed_once) {
		showed_once = true;
	        gchar *mes = g_strdup_printf(_("Can not reslove %s: %s\n"),
                         oStarDictClient->host_.c_str(), Socket::get_error_msg().c_str());
        	on_error_.emit(mes);
	        g_free(mes);
	}
        return;
    }

    if (oStarDictClient->host_resolved == false) {
            oStarDictClient->sa = sa_;
	    oStarDictClient->host_resolved = true;
    }

    oStarDictClient->sd_ = Socket::socket();

    if (oStarDictClient->sd_ == -1) {
        std::string str = _("Can not create socket: ") + Socket::get_error_msg();
        on_error_.emit(str.c_str());
        return;
    }
    Socket::connect(oStarDictClient->sd_, sa_, oStarDictClient->port_, oStarDictClient, on_connected);
}

void StarDictClient::on_connected(gpointer data, bool succeeded)
{
    StarDictClient *oStarDictClient = (StarDictClient *)data;
    if (!succeeded) {
	static bool showed_once = false;
	if (!showed_once) {
		showed_once = true;
	        gchar *mes = g_strdup_printf(_("Can not connect to %s: %s\n"), oStarDictClient->host_.c_str(), Socket::get_error_msg().c_str());
        	on_error_.emit(mes);
	        g_free(mes);
	}
        return;
    }
#ifdef _WIN32
    oStarDictClient->channel_ = g_io_channel_win32_new_socket(oStarDictClient->sd_);
#else
    oStarDictClient->channel_ = g_io_channel_unix_new(oStarDictClient->sd_);
#endif

    g_io_channel_set_encoding(oStarDictClient->channel_, NULL, NULL);

    /* make sure that the channel is non-blocking */
    int flags = g_io_channel_get_flags(oStarDictClient->channel_);
    flags |= G_IO_FLAG_NONBLOCK;
    GError *err = NULL;
    g_io_channel_set_flags(oStarDictClient->channel_, GIOFlags(flags), &err);
    if (err) {
        g_io_channel_unref(oStarDictClient->channel_);
        oStarDictClient->channel_ = NULL;
        gchar *str = g_strdup_printf(_("Unable to set the channel as non-blocking: %s"), err->message);
        on_error_.emit(str);
        g_free(str);
        g_error_free(err);
        return;
    }

    oStarDictClient->is_connected_ = true;
    oStarDictClient->waiting_banner_ = true;
    oStarDictClient->reading_type_ = READ_LINE;
    oStarDictClient->in_source_id_ = g_io_add_watch(oStarDictClient->channel_, GIOCondition(G_IO_IN | G_IO_ERR), on_io_in_event, oStarDictClient);
}

void StarDictClient::disconnect()
{
    clean_command();
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
	if (sd_ != -1) {
		Socket::close(sd_);
		sd_ = -1;
	}
    is_connected_ = false;
}

gboolean StarDictClient::on_io_out_event(GIOChannel *ch, GIOCondition cond,
                 gpointer user_data)
{
	StarDictClient *stardict_client = static_cast<StarDictClient *>(user_data);
	GError *err = NULL;
	GIOStatus res = g_io_channel_flush(stardict_client->channel_, &err);
	if (res == G_IO_STATUS_AGAIN) {
		return TRUE;
	} else if (err) {
		on_error_.emit(err->message);
		g_error_free(err);
	}
	stardict_client->out_source_id_ = 0;
	return FALSE;
}

gboolean StarDictClient::on_io_in_event(GIOChannel *ch, GIOCondition cond,
                 gpointer user_data)
{
    StarDictClient *stardict_client = static_cast<StarDictClient *>(user_data);

    if (!stardict_client->channel_) {
        //g_warning(_("No channel available\n"));
        return FALSE;
    }
    if (cond & G_IO_ERR) {
        /*gchar *mes =
            g_strdup_printf(_("Connection failed to the dictionary server at %s:%d"),
                    stardict_client->host_.c_str(), stardict_client->port_);
        on_error_.emit(mes);
        g_free(mes);*/
        stardict_client->disconnect();
        return FALSE;
    }
    GError *err = NULL;
    gsize term, len;
    gchar *line;
    GIOStatus res;

    for (;;) {
        if (!stardict_client->channel_)
            break;
        bool result;
        if (stardict_client->reading_type_ == READ_SIZE) {
            gsize bytes_read;
            res = g_io_channel_read_chars(stardict_client->channel_, stardict_client->size_data+(stardict_client->size_count-stardict_client->size_left), stardict_client->size_left, &bytes_read, &err);
            if (res == G_IO_STATUS_ERROR || res == G_IO_STATUS_EOF) {
                if (err) {
                    gchar *str = g_strdup_printf(_("Error while reading reply from server: %s"), err->message);
                    on_error_.emit(str);
                    g_free(str);
                    g_error_free(err);
                }
                stardict_client->disconnect();

                return FALSE;
            }
            stardict_client->size_left -= bytes_read;
            if (stardict_client->size_left == 0)
                result = stardict_client->parse(stardict_client->size_data);
            else
                break;
        } else {
            if (stardict_client->reading_type_ == READ_LINE)
                g_io_channel_set_line_term(stardict_client->channel_, "\n", 1);
            else if (stardict_client->reading_type_ == READ_STRING)
                g_io_channel_set_line_term(stardict_client->channel_, "", 1);

            res = g_io_channel_read_line(stardict_client->channel_, &line, &len, &term, &err); // May be security bug as no max length, but gtk should has fixed it!
            if (res == G_IO_STATUS_ERROR || res == G_IO_STATUS_EOF) {
                if (err) {
                    gchar *str = g_strdup_printf(_("Error while reading reply from server: %s"), err->message);
                    on_error_.emit(str);
                    g_free(str);
                    g_error_free(err);
                }
                stardict_client->disconnect();

                return FALSE;
            }

            if (!len)
                break;

            //truncate the line terminator before parsing
            line[term] = '\0';
            result = stardict_client->parse(line);
        }
        if (!result) {
            stardict_client->disconnect();
            return FALSE;
        }
    }

    return TRUE;
}

int StarDictClient::parse_banner(gchar *line)
{
	int status;
	status = atoi(line);
	if (status != CODE_HELLO) {
		if (status == CODE_TEMPORARILY_UNAVAILABLE) {
 			g_print(_("Server temporarily unavailable!\n"));
		} else {
			g_print(_("Unexpected status code %d\n"), status);
		}
		return 0;
	}
	char *p, *p1, *p2;
	p = strstr(line, "<auth>");
	if (p) {
		p += 6;
		p1 = strchr(p, '<');
		if (p1) {
			p2 = strchr(p1, '>');
			if (p2) {
				cmd_reply.daemonStamp.assign(p1, p2+1-p1);
			}
		}
	}
	std::string RSA_Public_Key;
	p = strstr(line, "<rsa_public_key>");
	if (p) {
		p += 16;
		p1 = strchr(p, '<');
		if (p1) {
			p1++;
			p2 = strchr(p1, '>');
			if (p2) {
				RSA_Public_Key.assign(p1, p2-p1);
				rsa_public_key_str_to_bin(RSA_Public_Key, RSA_Public_Key_e, RSA_Public_Key_n);
			}
		}
	}
	return 1;
}

int StarDictClient::parse_command_client(gchar *line)
{
    int status;
    status = atoi(line);
    if (status != CODE_OK) {
	gchar *str = g_strdup_printf(_("Client denied: %s"), line);
	on_error_.emit(str);
	g_free(str);
        return 0;
    }
    return 1;
}

int StarDictClient::parse_command_auth(gchar *line)
{
    int status;
    status = atoi(line);
    if (status != CODE_OK) {
        gchar *str = g_strdup_printf(_("Authentication denied: %s"), line);
        on_error_.emit(str);
        g_free(str);
        return 0;
    }
    return 1;
}

int StarDictClient::parse_command_register(gchar *line)
{
    int status;
    status = atoi(line);
    if (status != CODE_OK) {
        gchar *str = g_strdup_printf(_("Register failed: %s"), line);
        on_error_.emit(str);
        g_free(str);
        return 0;
    }
    on_register_end_.emit(_("Register success!"));
    return 1;
}

int StarDictClient::parse_command_changepassword(gchar *line)
{
    int status;
    status = atoi(line);
    if (status != CODE_OK) {
        gchar *str = g_strdup_printf(_("Change password failed: %s"), line);
        on_error_.emit(str);
        g_free(str);
        return 0;
    }
    on_changepassword_end_.emit(_("Change password success!"));
    return 1;
}

int StarDictClient::parse_command_quit(gchar *line)
{
    int status;
    status = atoi(line);
    if (status != CODE_GOODBYE) {
    }
    return 0;
}

int StarDictClient::parse_command_setdictmask(gchar *line)
{
    int status;
    status = atoi(line);
    if (status != CODE_OK) {
        gchar *str = g_strdup_printf(_("Set Dict Mask failed: %s"), line);
        on_error_.emit(str);
        g_free(str);
        return 0;
    }
    clean_cache_str("getdictmask\n");
    clean_cache_lookup_response();
    return 1;
}

int StarDictClient::parse_command_getdictmask(STARDICT::Cmd* cmd, gchar *buf)
{
	if (cmd->reading_status == 0) {
	int status;
	status = atoi(buf);
	if (status != CODE_OK) {
		if (status == CODE_USER_NOT_REGISTER) {
			g_free(buf);
			on_error_.emit(_("You haven't setup the account. Please open the \"Net Dict\" page in the Preferences dialog and register an account first."));
			return 1; // Cool.
		} else {
			gchar *str = g_strdup_printf(_("Get Dict Mask failed: %s"), buf);
			g_free(buf);
			on_error_.emit(str);
			g_free(str);
			return 0;
		}
	}
	g_free(buf);
	cmd->reading_status = 1;
	reading_type_ = READ_STRING;
	} else if (cmd->reading_status == 1) {
		on_getdictmask_end_.emit(buf);
		save_cache_str(cmd->data, buf);
		return 1;
	}
	return 2;
}

int StarDictClient::parse_command_dirinfo(STARDICT::Cmd* cmd, gchar *buf)
{
    if (cmd->reading_status == 0) {
        int status;
        status = atoi(buf);
        if (status != CODE_OK) {
            gchar *str = g_strdup_printf(_("Get dir info failed: %s"), buf);
            g_free(buf);
            on_error_.emit(str);
            g_free(str);
            return 0;
        }
        g_free(buf);
        cmd->reading_status = 1;
        reading_type_ = READ_STRING;
    } else if (cmd->reading_status == 1) {
        on_dirinfo_end_.emit(buf);
        save_cache_str(cmd->data, buf);
        return 1;
    }
    return 2;
}

int StarDictClient::parse_command_dictinfo(STARDICT::Cmd* cmd, gchar *buf)
{
    if (cmd->reading_status == 0) {
        int status;
        status = atoi(buf);
        if (status != CODE_OK) {
            gchar *str = g_strdup_printf(_("Get dict info failed: %s"), buf);
            g_free(buf);
            on_error_.emit(str);
            g_free(str);
            return 0;
        }
        g_free(buf);
        cmd->reading_status = 1;
        reading_type_ = READ_STRING;
    } else if (cmd->reading_status == 1) {
        on_dictinfo_end_.emit(buf);
        save_cache_str(cmd->data, buf);
        return 1;
    }
    return 2;
}

int StarDictClient::parse_command_getadinfo(STARDICT::Cmd* cmd, gchar *buf)
{
    if (cmd->reading_status == 0) {
        int status;
        status = atoi(buf);
        if (status != CODE_OK) {
            gchar *str = g_strdup_printf(_("Get ad info failed: %s"), buf);
            g_free(buf);
            on_error_.emit(str);
            g_free(str);
            return 0;
        }
        g_free(buf);
        cmd->reading_status = 1;
        reading_type_ = READ_STRING;
    } else if (cmd->reading_status == 1) {
        on_getadinfo_end_.emit(buf);
        save_cache_str(cmd->data, buf);
        return 1;
    }
    return 2;
}

int StarDictClient::parse_command_maxdictcount(STARDICT::Cmd* cmd, gchar *buf)
{
	if (cmd->reading_status == 0) {
		int status;
		status = atoi(buf);
		if (status != CODE_OK) {
			if (status == CODE_USER_NOT_REGISTER) {
				on_maxdictcount_end_.emit(0);
				save_cache_str(cmd->data, g_strdup("0"));
				g_free(buf);
				return 1;
			} else {
				gchar *str = g_strdup_printf(_("Get max dict count failed: %s"), buf);
				g_free(buf);
				on_error_.emit(str);
				g_free(str);
				return 0;
			}
		}
		g_free(buf);
		cmd->reading_status = 1;
		reading_type_ = READ_STRING;
	} else if (cmd->reading_status == 1) {
		on_maxdictcount_end_.emit(atoi(buf));
		save_cache_str(cmd->data, buf);
		return 1;
	}
	return 2;
}

int StarDictClient::parse_wordlist(STARDICT::Cmd* cmd, gchar *buf)
{
    if (cmd->reading_status == 0) { // Read code.
        int status;
        status = atoi(buf);
        g_free(buf);
        if (status != CODE_OK) {
            return 0;
        }
        cmd->wordlist_response = new std::list<char *>;
        cmd->reading_status = 1;
        reading_type_ = READ_STRING;
    } else if (cmd->reading_status == 1) {
        if (*buf == '\0') {
            g_free(buf);
            if (cmd->command == STARDICT::CMD_PREVIOUS) {
                on_previous_end_.emit(cmd->wordlist_response);
                return 1;
            } else {
                on_next_end_.emit(cmd->wordlist_response);
                return 1;
            }
        } else {
            if (cmd->wordlist_response->size() >= 250) {
                // Prevent too many strings! 250 should be enough!
                // CMD_NEXT is 30.
                // CMD_PREVIOUS is 15.
            } else {
                cmd->wordlist_response->push_back(buf);
            }
        }
    }
    return 2;
}

int StarDictClient::parse_dict_result(STARDICT::Cmd* cmd, gchar *buf)
{
    if (cmd->reading_status == 0) { // Read code.
        int status;
        status = atoi(buf);
        g_free(buf);
        if (status != CODE_OK) {
            if (status == CODE_DICTMASK_NOTSET) {
                on_error_.emit(_("You haven't chosen any dictionaries, please choose some by clicking \"Manage Dict\"->\"Network dictionaries\"->\"Add\"."));
                return 1;
            } else {
                return 0;
            }
        }
        cmd->lookup_response = new STARDICT::LookupResponse();
        cmd->lookup_response->listtype = STARDICT::LookupResponse::ListType_None;
        cmd->reading_status = 1;
        reading_type_ = READ_STRING;
    } else if (cmd->reading_status == 1) { // Read original word.
        cmd->lookup_response->dict_response.oword = buf;
        cmd->reading_status = 2;
    } else if (cmd->reading_status == 2) { // Read book name.
        if (*buf == '\0') {
            g_free(buf);
            if (cmd->command == STARDICT::CMD_DEFINE) {
                on_lookup_end_.emit(cmd->lookup_response, cmd->seq);
                save_cache_lookup_response(cmd->data, cmd->lookup_response);
                cmd->lookup_response = NULL;
                return 1;
            } else if ( cmd->command == STARDICT::CMD_SELECT_QUERY || cmd->command == STARDICT::CMD_SMART_QUERY) {
                on_floatwin_lookup_end_.emit(cmd->lookup_response, cmd->seq);
                save_cache_lookup_response(cmd->data, cmd->lookup_response);
                cmd->lookup_response = NULL;
                return 1;
            }
            cmd->reading_status = 6;
            reading_type_ = READ_STRING;
        } else {
            struct STARDICT::LookupResponse::DictResponse::DictResult *dict_result = new STARDICT::LookupResponse::DictResponse::DictResult();
            dict_result->bookname = buf;
            cmd->lookup_response->dict_response.dict_result_list.push_back(dict_result);
            cmd->reading_status = 3;
        }
    } else if (cmd->reading_status == 3) { // Read word.
        if (*buf == '\0') {
            g_free(buf);
            cmd->reading_status = 2;
        } else {
            struct STARDICT::LookupResponse::DictResponse::DictResult::WordResult *word_result = new STARDICT::LookupResponse::DictResponse::DictResult::WordResult();
            word_result->word = buf;
            cmd->lookup_response->dict_response.dict_result_list.back()->word_result_list.push_back(word_result);;
            cmd->reading_status = 4;
            reading_type_ = READ_SIZE;
	    size_data = (char *)g_malloc(sizeof(guint32));
	    size_count = size_left = sizeof(guint32);
        }
    } else if (cmd->reading_status == 4) {
        guint32 datasize = g_ntohl(get_uint32(buf));
	memcpy(buf, &datasize, sizeof(guint32));
        if (datasize == 0) {
	    g_free(buf);
            cmd->reading_status = 3;
            reading_type_ = READ_STRING;
        } else {
	    if (datasize > 4*1024*1024) {
	    	g_print(_("Drop data for security. Data too big! More than 4M!\n"));
		return 1;
	    } else {
		    cmd->reading_status = 5;
		    size_data = (char *)g_realloc(buf, datasize + sizeof(guint32));
		    size_count = datasize + sizeof(guint32);
		    size_left = datasize;
	    }
        }
    } else if (cmd->reading_status == 5) {
            cmd->lookup_response->dict_response.dict_result_list.back()->word_result_list.back()->datalist.push_back(buf);
	    cmd->reading_status = 4;
	    size_data = (char *)g_malloc(sizeof(guint32));
	    size_count = size_left = sizeof(guint32);
    } else if (cmd->reading_status == 6) {
        if (strcmp(buf, "d") == 0) {
            cmd->reading_status = 8;
            cmd->lookup_response->listtype = STARDICT::LookupResponse::ListType_Tree;
            cmd->lookup_response->wordtree = new std::list<STARDICT::LookupResponse::WordTreeElement *>;
        } else {
            cmd->reading_status = 7;
            if (strcmp(buf, "r") == 0)
                cmd->lookup_response->listtype = STARDICT::LookupResponse::ListType_Rule_List;
            else if (strcmp(buf, "g") == 0)
                cmd->lookup_response->listtype = STARDICT::LookupResponse::ListType_Regex_List;
            else if (strcmp(buf, "f") == 0)
                cmd->lookup_response->listtype = STARDICT::LookupResponse::ListType_Fuzzy_List;
            else 
                cmd->lookup_response->listtype = STARDICT::LookupResponse::ListType_List;
            cmd->lookup_response->wordlist = new std::list<char *>;
        }
        g_free(buf);
    } else if (cmd->reading_status == 7) {
        if (*buf == '\0') {
            g_free(buf);
            on_lookup_end_.emit(cmd->lookup_response, cmd->seq);
            save_cache_lookup_response(cmd->data, cmd->lookup_response);
            cmd->lookup_response = NULL;
            return 1;
        } else {
            cmd->lookup_response->wordlist->push_back(buf);
        }
    } else if (cmd->reading_status == 8) {
        if (*buf == '\0') {
            g_free(buf);
            on_lookup_end_.emit(cmd->lookup_response, cmd->seq);
            save_cache_lookup_response(cmd->data, cmd->lookup_response);
            cmd->lookup_response = NULL;
            return 1;
        } else {
            STARDICT::LookupResponse::WordTreeElement *element = new STARDICT::LookupResponse::WordTreeElement();
            element->bookname = buf;
            cmd->lookup_response->wordtree->push_back(element);
            cmd->reading_status = 9;
        }
    } else if (cmd->reading_status == 9) {
        if (*buf == '\0') {
            g_free(buf);
            cmd->reading_status = 8;
        } else {
            cmd->lookup_response->wordtree->back()->wordlist.push_back(buf);
        }
    }
    return 2;
}

bool StarDictClient::parse(gchar *line)
{
    int result;
    if (waiting_banner_) {
        waiting_banner_ = false;
        result = parse_banner(line);
        g_free(line);
        if (!result)
            return false;
        request_command();
        return true;
    }
    STARDICT::Cmd* cmd = cmdlist.front();
    switch (cmd->command) {
        case STARDICT::CMD_CLIENT:
            result = parse_command_client(line);
            g_free(line);
            break;
        case STARDICT::CMD_AUTH:
            result = parse_command_auth(line);
            g_free(line);
            break;
        case STARDICT::CMD_REGISTER:
            result = parse_command_register(line);
            g_free(line);
            break;
        case STARDICT::CMD_CHANGE_PASSWD:
            result = parse_command_changepassword(line);
            g_free(line);
            break;
        case STARDICT::CMD_GET_DICT_MASK:
            result = parse_command_getdictmask(cmd, line);
            break;
        case STARDICT::CMD_SET_DICT_MASK:
            result = parse_command_setdictmask(line);
            break;
        case STARDICT::CMD_DIR_INFO:
            result = parse_command_dirinfo(cmd, line);
            break;
        case STARDICT::CMD_DICT_INFO:
            result = parse_command_dictinfo(cmd, line);
            break;
        case STARDICT::CMD_MAX_DICT_COUNT:
            result = parse_command_maxdictcount(cmd, line);
            break;
        case STARDICT::CMD_DEFINE:
        case STARDICT::CMD_LOOKUP:
        case STARDICT::CMD_SELECT_QUERY:
        case STARDICT::CMD_SMART_QUERY:
            result = parse_dict_result(cmd, line);
            break;
        case STARDICT::CMD_PREVIOUS:
        case STARDICT::CMD_NEXT:
            result = parse_wordlist(cmd, line);
            break;
        case STARDICT::CMD_GET_ADINFO:
            result =  parse_command_getadinfo(cmd, line);
            break;
        case STARDICT::CMD_QUIT:
            result = parse_command_quit(line);
            g_free(line);
            break;
        default:
            result = 0;
            g_free(line);
            break;
    }
    if (result == 0)
        return false;
    if (result == 1) {
        delete cmd;
        cmdlist.pop_front();
        if (cmdlist.empty()) {
            cmdlist.push_back(new STARDICT::Cmd(STARDICT::CMD_QUIT));
        }
        request_command();
    }
    return true;
}
