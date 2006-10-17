#ifndef _STARDICT_CLIENT_HPP_
#define _STARDICT_CLIENT_HPP_

#include <glib.h>

#include "sigc++/sigc++.h"

#include <list>

namespace STARDICT {
	enum {
		CMD_CLIENT,
		CMD_AUTH,
		CMD_LOOKUP,
		CMD_PREVIOUS,
		CMD_NEXT,
		//CMD_QUERY,
		CMD_SELECT_QUERY,
		CMD_DEFINE,
		CMD_REGISTER,
		CMD_CHANGE_PASSWD,
		CMD_SET_DICT_MASK,
		CMD_GET_DICT_MASK,
		CMD_SET_COLLATE_FUNC,
		CMD_GET_COLLATE_FUNC,
		CMD_SET_LANGUAGE,
		CMD_GET_LANGUAGE,
		CMD_SET_EMAIL,
		CMD_GET_EMAIL,
		CMD_MAX_DICT_COUNT,
		CMD_DIR_INFO,
		CMD_DICT_INFO,
		CMD_USER_LEVEL,
		CMD_GET_USER_LEVEL,
		CMD_QUIT,
	};
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
    struct LookupResponse {
	~LookupResponse();
        struct DictResponse dict_response;
        std::list<char *> wordlist;
    };
	class Cmd {
	public:
		int command;
		struct AuthInfo {
            std::string user;
            std::string passwd;
		};
		union {
			char *data;
            AuthInfo *auth;
		};
        int reading_status;
        union {
            struct LookupResponse *lookup_response;
            struct DictResponse *dict_response;
        };
		Cmd(int cmd, ...);
        ~Cmd();
	};
};

class StarDictClient {
public:
    static sigc::signal<void, const std::string&> on_error_;
    static sigc::signal<void, const struct STARDICT::LookupResponse *> on_lookup_end_;
    static sigc::signal<void, const struct STARDICT::DictResponse *> on_define_end_;

	StarDictClient();
	~StarDictClient();

    void set_server(const char *host, int port = 2628);
    void set_auth(const char *user, const char *md5passwd);
	void send_commands(int num, ...);
private:
	GIOChannel *channel_;
	guint source_id_;
	std::string host_;
	int port_;
    std::string user_;
    std::string md5passwd_;
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
	static gboolean on_io_event(GIOChannel *, GIOCondition, gpointer);
	bool connect();
    void write_str(const char *str, GError **err);
    bool parse(gchar *line);
    int parse_banner(gchar *line);
    int parse_command_client(gchar *line);
    int parse_command_quit(gchar *line);
    int parse_dict_result(STARDICT::Cmd* cmd, gchar *buf);
};

#endif
