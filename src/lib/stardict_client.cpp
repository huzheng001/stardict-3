#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <glib.h>

#include "sockets.hpp"
#include "md5.h"

#include "stardict_client.hpp"

#define CODE_HELLO                   220 /* text msg-id */
#define CODE_GOODBYE                 221 /* Closing Connection */
#define CODE_OK                      250 /* ok */
#define CODE_TEMPORARILY_UNAVAILABLE 420 /* server unavailable */
#define CODE_SYNTAX_ERROR            500 /* syntax, command not recognized */
#define CODE_DENIED                  521
#define CODE_DICTMASK_NOTSET         522

sigc::signal<void, const std::string&> StarDictClient::on_error_;

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
	this->command = cmd;
	va_list    ap;
	va_start( ap, cmd );
	switch (cmd) {
	case CMD_CLIENT:
	{
		const char *protocol_version = va_arg( ap, const char * );
		const char *client_name = va_arg( ap, const char * );
		std::string earg1, earg2;
		arg_escape(earg1, protocol_version);
		arg_escape(earg2, client_name);
		this->data = g_strdup_printf("client %s %s\n", earg1.c_str(), earg2.c_str());
		break;
	}
	case CMD_REGISTER:
	{
		const char *user = va_arg( ap, const char * );
		const char *passwd = va_arg( ap, const char * );
		const char *email = va_arg( ap, const char * );
		std::string earg1, earg2, earg3;
		arg_escape(earg1, user);
		arg_escape(earg2, passwd);
		arg_escape(earg3, email);
		this->data = g_strdup_printf("register %s %s %s\n", earg1.c_str(), earg2.c_str(), earg3.c_str());
		break;
	}
	case CMD_CHANGE_PASSWD:
	{
		const char *user = va_arg( ap, const char * );
		const char *old_passwd = va_arg( ap, const char * );
		const char *new_passwd = va_arg( ap, const char * );
		std::string earg1, earg2, earg3;
		arg_escape(earg1, user);
		arg_escape(earg2, old_passwd);
		arg_escape(earg3, new_passwd);
		this->data = g_strdup_printf("change_password %s %s %s\n", earg1.c_str(), earg2.c_str(), earg3.c_str());
		break;
	}
	case CMD_AUTH:
		this->auth.user = va_arg( ap, const char * );
		this->auth.passwd = va_arg( ap, const char * );
		break;
	case CMD_LOOKUP:
	{
		std::string earg;
		arg_escape(earg, va_arg( ap, const char * ));
		this->data = g_strdup_printf("lookup %s\n", earg.c_str());
		break;
	}
	case CMD_PREVIOUS:
	{
		std::string earg;
		arg_escape(earg, va_arg( ap, const char * ));
		this->data = g_strdup_printf("previous %s\n", earg.c_str());
		break;
	}
	case CMD_NEXT:
	{
		std::string earg;
		arg_escape(earg, va_arg( ap, const char * ));
		this->data = g_strdup_printf("next %s\n", earg.c_str());
		break;
	}
	case CMD_QUERY:
	{
		std::string earg;
		arg_escape(earg, va_arg( ap, const char * ));
		this->data = g_strdup_printf("query %s\n", earg.c_str());
		break;
	}
	case CMD_SELECT_QUERY:
	{
		std::string earg;
		arg_escape(earg, va_arg( ap, const char * ));
		this->data = g_strdup_printf("selectquery %s\n", earg.c_str());
		break;
	}
	case CMD_DEFINE:
	{
		std::string earg;
		arg_escape(earg, va_arg( ap, const char * ));
		this->data = g_strdup_printf("define %s\n", earg.c_str());
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
	case CMD_SET_COLLATE_FUNC:
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
	case CMD_GET_USER_LEVEL:
		this->data = g_strdup("getuserlevel\n");
		break;
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
	case CMD_USER_LEVEL:
	{
		std::string earg1, earg2, earg3;
		arg_escape(earg1, va_arg( ap, const char * ));
		arg_escape(earg2, va_arg( ap, const char * ));
		arg_escape(earg3, va_arg( ap, const char * ));
		this->data = g_strdup_printf("userlevel %s %s %s\n", earg1.c_str(), earg2.c_str(), earg3.c_str());
		break;
	}
	case CMD_QUIT:
		this->data = g_strdup("quit\n");
		break;
	}
	va_end( ap );
}

STARDICT::Cmd::~Cmd()
{
	switch (this->command) {
		case CMD_AUTH:
			break;
		default:
			g_free(this->data);
			break;
	}
}

StarDictClient::StarDictClient(const char *host, int port)
{
}

StarDictClient::~StarDictClient()
{
}

void StarDictClient::append_command(STARDICT::Cmd *c)
{
	cmdlist.push_back(c);
}

void StarDictClient::write_str(const char *str, GError **err)
{
    int len = strlen(str);
    int left_byte = len;
    GIOStatus res;
    gsize bytes_written;
    while (left_byte) {
        res = g_io_channel_write_chars(channel_, str+(len - left_byte), left_byte, &bytes_written, err);
        if (res != G_IO_STATUS_NORMAL) 
            return;
        left_byte -= bytes_written;
    }
    g_io_channel_flush(channel_, err);
}

bool StarDictClient::request_command(STARDICT::Cmd *c)
{
	switch (c->command) {
        case STARDICT::CMD_AUTH:
		{
			struct MD5Context ctx;
			unsigned char digest[16];
			char hex[33];
			int i;
			MD5Init(&ctx);
			MD5Update(&ctx, (const unsigned char*)cmd_reply.daemonStamp.c_str(), cmd_reply.daemonStamp.length());
			MD5Update(&ctx, (const unsigned char*)(c->auth.passwd), strlen(c->auth.passwd));
			MD5Final(digest, &ctx );
			for (i = 0; i < 16; i++)
				sprintf( hex+2*i, "%02x", digest[i] );
			hex[32] = '\0';
			std::string earg1, earg2;
			arg_escape(earg1, c->auth.user);
			arg_escape(earg2, hex);
			char *data = g_strdup_printf("auth %s %s\n", earg1.c_str(), earg2.c_str());
            GError *err = NULL;
            write_str(data, &err);
			g_free(data);
			if (err) {
                on_error_.emit(err->message);
                g_error_free(err);
				return true;
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
				return true;
            }
			break;
        }
	}
	return false;
}

void StarDictClient::clean_command()
{
	for (std::list<STARDICT::Cmd *>::iterator i=cmdlist.begin(); i!=cmdlist.end(); ++i) {
		delete *i;
	}
	cmdlist.clear();
}
