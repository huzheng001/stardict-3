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

#ifndef _DICT_CLINET_HPP_
#define _DICT_CLINET_HPP_

#include <glib.h>
#include <string>
#include <memory>
#include <map>
#include <vector>

#include "stardict-sigc++.h"

namespace DICT {
	struct Definition {
		std::string word_;
		std::string data_;

		Definition(const gchar *word): word_(word) {}
	};
	typedef std::vector<Definition> DefList;

	class Cmd {
	public:
		enum State {
			START, DATA, FINISH
		} state_;

		Cmd() : state_(START) {}
		virtual ~Cmd() {}
		virtual const std::string& query() { return query_; }
		virtual bool parse(gchar *str, int code) = 0;

		void send(GIOChannel *channel, GError *&err);
		const DefList& result() const { return reslist_; }
	protected:
		std::string query_;
		DefList reslist_;
	};
};

class DictClient {
public:
	typedef std::vector<size_t> IndexList;
	typedef std::list<std::string> StringList;

	static sigc::signal<void, const std::string&> on_error_;
	static sigc::signal<void, const IndexList&> on_simple_lookup_end_;
	static sigc::signal<void, const StringList&> on_complex_lookup_end_;

	DictClient(const char *host, int port = 2628);
	~DictClient();
	void lookup_simple(const gchar *word);
	void lookup_with_rule(const gchar *word);
	void lookup_with_fuzzy(const gchar *word);
	const gchar *get_word(size_t index) const;
	const gchar *get_word_data(size_t index) const;
private:
	int sd_;
	GIOChannel *channel_;
	guint source_id_;
	std::string host_;
	int port_;
	bool is_connected_;
	std::unique_ptr<DICT::Cmd> cmd_;
	typedef std::map<size_t, DICT::Definition> DefMap;
	DefMap defmap_;
	size_t last_index_;
	bool simple_lookup_;

	void disconnect();
	static gboolean on_io_event(GIOChannel *, GIOCondition, gpointer);
	static int get_status_code(gchar *line);
	void connect();
	static void on_resolved(gpointer data, struct hostent *ret);
	bool parse(gchar *line, int status_code);
};

#endif//!_DICT_CLINET_HPP_
