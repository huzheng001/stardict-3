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

#include "config_file.hpp"
#include "inifile.hpp"
#ifdef CONFIG_GNOME
# include "gconf_file.hpp"
#endif

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

#define TEST(type, def_val, some_val) do { \
		type type##_val=def_val; \
		cf->read_##type("/stardict-test", #type, type##_val); \
		if (type##_val!=def_val) { \
			std::cerr<<"type: "<<#type<<", default value was changed"<<std::endl;	\
			std::cerr<<"default: "<<def_val<<std::endl<<"have: "<<type##_val<<std::endl; \
			return false; \
		}												\
		cf->write_##type("/stardict-test", #type, some_val); \
		cf->read_##type("/stardict-test", #type, type##_val); \
		if (type##_val!=some_val) { \
			std::cerr<<"type: "<<#type<<", default value was NOT changed"<<std::endl;	\
			std::cerr<<"set to: "<<some_val<<std::endl<<"get: "<<type##_val<<std::endl; \
			return false; \
		}											\
	}	while (0)

static bool is_test_passed(config_file *cf)
{
	TEST(bool, true, false);
	TEST(int, 17, 18);
	TEST(string, "aaa", "bbb");
	strlist def_val, some_val;
	def_val.push_back("a");
	def_val.push_back("bbb");
	def_val.push_back("cdad");

	some_val.push_back("a");
	some_val.push_back("bbb");
	some_val.push_back("cdad");
	some_val.push_back("fdfad");

	TEST(strlist, def_val, some_val);

	return true;
}

#ifdef CONFIG_GNOME
static bool test_gconf()
{
	string tmp1=string(g_get_home_dir())+G_DIR_SEPARATOR_S+".gconf/stardict-test";
	string tmp2=string(g_get_home_dir())+G_DIR_SEPARATOR_S+".gconf/stardict-test/%gconf.xml";
	remove(tmp1.c_str());
	remove(tmp2.c_str());
	std::auto_ptr<config_file> cf;
	cf.reset(new gconf_file("/apps/stardict"));
	tmp_file tf=tmp1;
	tmp_file tf1=tmp2;
	bool res=is_test_passed(cf.get());
	cf.reset(0);
	system("gconftool-2 --shutdown");
	return res;
}
#endif

static bool test_inifile()
{
	std::auto_ptr<inifile> cf;
	string fname("/tmp/stardict.cfg");
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
	gtk_set_locale();
	if (!test_inifile()) {
		std::cerr<<"ini file test failed"<<std::endl;
		return EXIT_FAILURE;
	}
		
#ifdef CONFIG_GNOME
	gtk_init(&argc, &argv);
	if (!test_gconf()) {
		std::cerr<<"Gconf test failed"<<std::endl;
		return EXIT_FAILURE;
	}
#endif
	
	return EXIT_SUCCESS;
}
