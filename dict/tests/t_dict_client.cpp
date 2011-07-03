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

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <cstdlib>
#include <clocale>

#include "dict_client.h"

class DictClientTest {
public:
	DictClientTest();
	~DictClientTest() { g_main_loop_unref(main_loop_); }
	int run();
private:
	DictClient dict_;
	GMainLoop *main_loop_;
	bool regexp_lookup_;

	void on_error(const std::string& mes);
	void on_simple_lookup_end(const DictClient::IndexList&);	
	void on_complex_lookup_end(const DictClient::StringList&);
};

DictClientTest::DictClientTest() : dict_("localhost")
{
	main_loop_ = g_main_loop_new(NULL, FALSE);
	dict_.on_error_.connect(sigc::mem_fun(this,
					      &DictClientTest::on_error));
	dict_.on_simple_lookup_end_.connect(
		sigc::mem_fun(this,
			      &DictClientTest::on_simple_lookup_end));
	dict_.on_complex_lookup_end_.connect(
		sigc::mem_fun(this,
			      &DictClientTest::on_complex_lookup_end));
}

int DictClientTest::run()
{	 
	dict_.lookup_simple("man");	
	g_main_loop_run(main_loop_);
	return EXIT_SUCCESS;
}

void DictClientTest::on_error(const std::string& mes)
{
	g_debug("%s: %s\n", __PRETTY_FUNCTION__, mes.c_str());
}

void DictClientTest::on_simple_lookup_end(const DictClient::IndexList& ilist)
{
	g_debug("%s: %s\n", __PRETTY_FUNCTION__,
		!ilist.empty() ? "found" : "not found");
	for (size_t i = 0; i < ilist.size(); ++i)
		g_debug("--->%s\n%s\n", dict_.get_word(ilist[i]),
			dict_.get_word_data(ilist[i]));

	regexp_lookup_ = true;
	dict_.lookup_with_rule("m*n");
}

void DictClientTest::on_complex_lookup_end(const DictClient::StringList& slist)
{
	g_debug("%s: %s\n", __PRETTY_FUNCTION__,
		!slist.empty() ? "found" : "not found");
	DictClient::StringList::const_iterator it;

	for (it = slist.begin(); it != slist.end(); ++it)
		g_debug("--->%s\n", it->c_str());
	if (regexp_lookup_) {
		regexp_lookup_ = false;
		dict_.lookup_with_fuzzy("man");
	} else
		g_main_loop_quit(main_loop_);
}

int main()
{
	setlocale(LC_ALL, "");//so g_debug and so on will print not garbage
	DictClientTest test;	

	return test.run();
}
