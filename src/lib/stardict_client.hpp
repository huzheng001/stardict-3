#ifndef _STARDICT_CLIENT_HPP_
#define _STARDICT_CLIENT_HPP_

#include <glib.h>

#include "sigc++/sigc++.h"

namespace STARDICT {
	enum {
		CMD_CLIENT,
		CMD_AUTH,
		CMD_LOOKUP,
		CMD_PREVIOUS,
		CMD_NEXT,
		CMD_QUERY,
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
        char *oword;
        struct DictResult {
            char *bookname;
            struct WordResult {
                char *word;
                std::list<char *> datalist;
            };
            std::list<WordResult *> word_result_list;
        };
        std::list<DictResult *> dict_result_list;
    };
    struct LookupResponse {
        struct DictResponse dict_response;
        std::list<char *> wordlist;
    };
	class Cmd {
	public:
		int command;
		union {
			char *data;
			struct {
				const char *user;
				const char *passwd;
			} auth;
		};
        int reading_status;
        union {
            struct LookupResponse *lookup_response;
        };
		Cmd(int cmd, ...);
        ~Cmd();
	};
};

class StarDictClient {
public:
    static sigc::signal<void, const std::string&> on_error_;

	StarDictClient(const char *host, int port = 2628);
	~StarDictClient();

	void append_command(STARDICT::Cmd *c);
	bool request_command(STARDICT::Cmd *c);
	void process_command();
	void clean_command();
private:
	GIOChannel *channel_;
	guint source_id_;
	std::string host_;
	int port_;
	bool is_connected_;
    bool waiting_banner_;
	std::list<STARDICT::Cmd *> cmdlist;
    struct reply {
            std::string daemonStamp;
    } cmd_reply;
    enum ReadState {
        READ_LINE,
        READ_STRING,
        READ_SIZE,
    } reading_status_;
    char *size_data;
    gsize size_count;
    gsize size_left;

	void disconnect();
	static gboolean on_io_event(GIOChannel *, GIOCondition, gpointer);
	bool connect();
    void write_str(const char *str, GError **err);
    bool parse(gchar *line);
    bool parse_banner(gchar *line);
    bool parse_command_client(gchar *line);
    bool parse_command_quit(gchar *line);
    bool parse_dict_result(STARDICT::Cmd* cmd, gchar *buf);
};

#endif
