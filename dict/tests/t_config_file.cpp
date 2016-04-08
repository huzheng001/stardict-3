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

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <memory>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iterator>
#include <iostream>
#include <gtk/gtk.h>
#include <memory>
#include <glib.h>
#include <glib/gstdio.h>

#include "config_file.h"
#include "inifile.h"
#ifdef CONFIG_GNOME
# include "gconf_file.h"
#endif
#include "libcommon.h"

using std::string;
using std::ostream_iterator;

typedef std::list<string> strlist;

std::ostream& operator<<(std::ostream& os, const strlist &sl)
{
	std::copy(sl.begin(), sl.end(), ostream_iterator<string>(os, "\n"));
	return os;
}

struct tmp_file {
	string fname;
	tmp_file(const string& fn) : fname(fn) {}
	~tmp_file() { 
		remove(fname.c_str());
	}
};

#define TEST(type, def_val, some_val, sect) do { \
		type type##_val=def_val; \
		cf->read_##type(sect, #type, type##_val); \
		if (type##_val!=def_val) { \
			std::cerr<<"type: "<<#type<<", default value was changed"<<std::endl;	\
			std::cerr<<"default: "<<def_val<<std::endl<<"have: "<<type##_val<<std::endl; \
			return false; \
		}												\
		cf->write_##type(sect, #type, some_val); \
		cf->read_##type(sect, #type, type##_val); \
		if (type##_val!=some_val) { \
			std::cerr<<"type: "<<#type<<", default value was NOT changed"<<std::endl;	\
			std::cerr<<"set to: "<<some_val<<std::endl<<"get: "<<type##_val<<std::endl; \
			return false; \
		}											\
	}	while (0)

static bool is_test_passed(config_file *cf)
{
	glib::CharStr c_sect(g_strdup_printf("/stardict-test-%d", getpid()));
	TEST(bool, true, false, get_impl(c_sect));
	TEST(int, 17, 18, get_impl(c_sect));
	TEST(string, "aaa", "bbb", get_impl(c_sect));
	strlist def_val, some_val;
	def_val.push_back("a");
	def_val.push_back("bbb");
	def_val.push_back("cdad");

	some_val.push_back("a");
	some_val.push_back("bbb");
	some_val.push_back("cdad");
	some_val.push_back("fdfad");

	TEST(strlist, def_val, some_val, get_impl(c_sect));

	return true;
}

#ifdef CONFIG_GNOME
static bool test_gconf()
{
	glib::CharStr c_tmp(g_strdup_printf("%s/.gconf/stardict-test-%d", g_get_home_dir(), getpid()));
	string tmp1=get_impl(c_tmp);
	string tmp2=tmp1 + string("/%gconf.xml");
	remove(tmp2.c_str());
	remove(tmp1.c_str());
	std::unique_ptr<config_file> cf;
	cf.reset(new gconf_file("/apps/stardict"));
	tmp_file tf=tmp1;
	tmp_file tf1=tmp2;
	bool res=is_test_passed(cf.get());
	cf.reset(0);
	return res;
}
#endif

static bool test_inifile()
{
	std::unique_ptr<inifile> cf;
	glib::CharStr c_fname(g_strdup_printf("/tmp/stardict-%d.cfg", getpid()));
	string fname(get_impl(c_fname));
	tmp_file tf(fname);
	
	// normal config
	remove(fname.c_str());
	cf.reset(new inifile());
	if(!cf->load(fname))
		return false;
	if(!is_test_passed(cf.get()))
		return false;
	// destroy inifile and make sure that fname will not be changed by inifile
	cf.reset(0);
	
	// read-only config
	remove(fname.c_str());
	{	// create an empty file, but the file must contain at least 1 byte
		FILE *f = g_fopen(fname.c_str(), "w");
		if (!f)
			return false;
		fwrite(" ", 1, 1, f);
		fclose(f);
	}
	cf.reset(new inifile());
	if(!cf->load(fname, true, false))
		return false;
	if(!is_test_passed(cf.get()))
		return false;
	cf.reset(0);
	
	// custom test 1
	if(!g_file_set_contents(fname.c_str(),
		"[general]\nkey=value\n", -1, NULL))
		return false;
	cf.reset(new inifile());
	if(!cf->load(fname, true, false))
		return false;
	std::string val;
	if(!cf->read_string("general", "key", val))
		return false;
	if(val != "value")
		return false;
	
	return true;
}

int main(int argc, char *argv[])
{
#ifdef CONFIG_GNOME
	gtk_init(&argc, &argv);
#else
	setlocale(LC_ALL, "");
#endif

	if (!test_inifile()) {
		std::cerr<<"ini file test failed"<<std::endl;
		return EXIT_FAILURE;
	}
		
#ifdef CONFIG_GNOME
	if (!test_gconf()) {
		std::cerr<<"Gconf test failed"<<std::endl;
		return EXIT_FAILURE;
	}
#endif
	
	return EXIT_SUCCESS;
}
