#ifndef _DICT_CLINET_HPP_
#define _DICT_CLINET_HPP_

#include <glib.h>
#include <string>
#include <memory>

namespace DICT {
	class Cmd {
	public:
		enum State {
			START, DATA, FINISH
		} state_;
		Cmd() : state_(START) {}
		virtual ~Cmd() {}
		virtual const std::string& query() = 0;
	};
};

class DictClient {
public:
	DictClient(const char *host, int port = 2628);
	~DictClient();
	bool lookup_simple(const gchar *word);
private:
	GIOChannel *channel_;
	guint source_id_;
	std::string host_;
	int port_;
	std::auto_ptr<DICT::Cmd> cmd_;

	void disconnect();
	static gboolean on_io_event(GIOChannel *, GIOCondition, gpointer);
	static int get_status_code(gchar *line);
	bool connect();
	bool parse(gchar *line, int status_code);
};

#endif//!_DICT_CLINET_HPP_
