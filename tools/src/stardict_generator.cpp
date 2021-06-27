/*
 * This file part of makedict - convertor from any dictionary format to any
 * http://xdxf.sourceforge.net
 * Copyright (C) 2005-2006 Evgeniy <dushistov@mail.ru>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

/**
 * @file stardict_generator.cpp
 * StarDict dictionaries generator
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <glib/gi18n.h>
#include <glib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string>
#include <vector>
#include <sstream>
#include <map>
#include <algorithm>

#include "utils.h"

#include "generator.h"


namespace patch
{
    template < typename T > std::string to_string( const T& n )
    {
        std::ostringstream stm ;
        stm << n ;
        return stm.str() ;
    }
}


namespace {
	class StarDictGenerator : public GeneratorBase {
	public:
		StarDictGenerator();
		~StarDictGenerator() {
			for (std::vector<Key>::iterator p=keys_list.begin();
			     p!=keys_list.end(); ++p)
				delete []p->value;
		}
	protected:
		int generate();
		bool on_have_data(const StringList&, const std::string&);
		bool on_prepare_generator(const std::string&,
					  const std::string&);
	private:
		File idx_file_;
		guint32 cur_off_;
		std::string full_name;

		class RemoveAfterExit {
		public:
			RemoveAfterExit() {}
			RemoveAfterExit(const std::string& fn) : filename_(fn) {}
			~RemoveAfterExit() {
				if (!filename_.empty())
					remove(filename_.c_str()); 
			}
			void operator() (const std::string& fn) { filename_ = fn; }
		private:
			std::string filename_;
		} remove_after_exit_;
		File tmp_dict_file_;
		File dict_file_;
		std::string realbasename_;

		struct Key {
			char *value;
			guint32 data_off;
			guint32 data_size;
			Key(char *val=NULL, guint32 off=0, guint32 size=0) :
				value(val), data_off(off), data_size(size)
				{
				}
			Key(const std::string& val, guint32 off=0, guint32 size=0) :
				value(NULL), data_off(off), data_size(size)
				{
					value = new char [val.length()+1];
					strcpy(value, val.c_str());
				}
		};
		std::vector<Key> keys_list;
		static gint stardict_strcmp(const gchar *s1, const gchar *s2) {
			gint a = g_ascii_strcasecmp(s1, s2);
			if (a == 0)
				return strcmp(s1, s2);

			return a;
		}
		class stardict_less {
		public:
			bool operator()(const Key & lh, const Key & rh) {
				return stardict_strcmp(lh.value, rh.value)<0;
			}
		};
		bool create_ifo_file(const std::string& basename, guint32 wordcount, guint32 idx_file_size);
		std::string get_current_date(void);
	};

	REGISTER_GENERATOR(StarDictGenerator, stardict);


	StarDictGenerator::StarDictGenerator()
	:
		GeneratorBase(true) 
	{
		set_format("stardict");
		set_version(_("stardict_generator, version 0.2"));
		cur_off_ = 0;
		keys_list.reserve(20000);
		generator_options_["stardict_ver"]="2.4.2";
	}

	bool StarDictGenerator::on_prepare_generator(const std::string& workdir,
						     const std::string& bname)
	{
		std::string basename = "stardict-" + bname + "-"
			+ generator_options_["stardict_ver"];
		std::string dirname = workdir + G_DIR_SEPARATOR + basename;

		if (!make_directory(dirname))
			return false;
		g_message(_("Saving result to this directory: %s\n"), dirname.c_str());
		realbasename_ = dirname + G_DIR_SEPARATOR + bname;

		std::string file_name = realbasename_ + ".dict.tmp";
		tmp_dict_file_.reset(fopen(file_name.c_str(), "w+b"));

		if (!tmp_dict_file_) {
			g_critical("Can not open/create: %s\n", file_name.c_str());
			return false;
		}
		remove_after_exit_(file_name);


		file_name = realbasename_ + ".idx";
		idx_file_.reset(fopen(file_name.c_str(), "wb"));
		if (!idx_file_) {
			g_critical("Can not open/create: %s\n", file_name.c_str());
			return false;
		}
		g_message(_("Writing index to %s\n"), file_name.c_str());
		file_name = realbasename_ + ".dict";
		dict_file_.reset(fopen(file_name.c_str(), "wb"));

		if (!dict_file_) {
			g_critical("Can not open/create: %s\n", file_name.c_str());
			return false;
		}

		g_message(_("Writing data to %s\n"), file_name.c_str());

		return true;
	}

	int StarDictGenerator::generate()
	{
		int res = EXIT_FAILURE;

		g_message(_("Sorting..."));
		std::sort(keys_list.begin(), keys_list.end(), stardict_less());
		g_message(_("done\n"));

		guint32 wordcount=keys_list.size();
		guint32 data_size, data_offset=0;
		guint32 i=0;

		std::string encoded_str;
		Key *pitem = &keys_list[i];
		while (i < keys_list.size()) {
			char *prev_item_value=NULL;
			std::vector<char> buf;
			guint32 tmpguint32=0;
			data_size=0;

			for (;;) {
				tmp_dict_file_.seek(pitem->data_off);
				buf.resize(pitem->data_size+1);
				tmp_dict_file_.read(&buf[0], pitem->data_size);
				buf[pitem->data_size]='\0';

				if (!g_utf8_validate(&buf[0], gssize(-1), NULL)) {
					g_critical("Not valid UTF-8 std::string: %s\n",
						      encoded_str.c_str());
					return res;
				}

				if (!dict_file_.write(&buf[0], pitem->data_size)) {
					g_critical("Can not write in data file\n");
					return res;
				}

				data_size+=pitem->data_size;
				prev_item_value=pitem->value;
				++i;
				
				if (i < keys_list.size()) pitem = &keys_list[i];
				if (i < keys_list.size() &&
					strcmp(prev_item_value, keys_list[i].value) == 0) {
					g_info(_("Duplicate!: %s\n"), prev_item_value);
					wordcount--;
					dict_file_.write("\n", 1);
					++data_size;
				}
				else
					break;
			}

			idx_file_.write(prev_item_value, strlen(prev_item_value)+1);
			tmpguint32 = g_htonl(data_offset);
			idx_file_.write((char *)&tmpguint32, sizeof(guint32));
			tmpguint32 = g_htonl(data_size);
			idx_file_.write((char *)&tmpguint32, sizeof(guint32));
			data_offset+=data_size;
		}

		if (!create_ifo_file(realbasename_, wordcount, idx_file_.tell())) {
			g_critical("Creating ifo file error.\n");
			return EXIT_FAILURE;
		}
		g_message(_("Count of articles: %u\n"), wordcount);

		return EXIT_SUCCESS;
	}

	bool StarDictGenerator::on_have_data(const StringList& keys,
					     const std::string& data)
	{
		for (StringList::const_iterator p = keys.begin(); p != keys.end(); ++p)
			keys_list.push_back(Key(*p, cur_off_, data.length()));
		tmp_dict_file_.write(&data[0], data.length());
		cur_off_ += data.length();

		return !tmp_dict_file_ ? false : true;
	}

	bool StarDictGenerator::create_ifo_file(const std::string& basename,
						guint32 wordcount,
						guint32 idx_file_size)
	{
		std::string filename=basename+".ifo";
		File f(fopen(filename.c_str(), "wb"));

		if (!f) {
			g_critical("Can not create/open: %s\n", filename.c_str());
			return false;
		}

		g_message(_("Writing meta-info to %s\n"), filename.c_str());

		f << "StarDict's dict ifo file\n"
		  << "version=" << generator_options_["stardict_ver"] << "\n"
		  //<< "wordcount=" << std::to_string(wordcount) << "\n"
		  << "wordcount=" << patch::to_string(wordcount) << "\n"
		  //<< "idxfilesize=" << std::to_string(idx_file_size) << "\n";
		  << "idxfilesize=" << patch::to_string(idx_file_size) << "\n";

		if (!full_name.empty())
			f << "bookname=" << full_name << "\n";
		else
			f << "bookname=" << get_dict_info("full_name") << "\n";

		f<<"date="<<get_current_date()<<"\n"
		 <<"sametypesequence=x\n";

		// handle new lines in description
		std::string descr2;
		ReplaceStrTable newline_tbl;
		//order is important
		newline_tbl["\r\n"] = "<br>";//DOS, OS/2, Microsoft Windows, Symbian OS
		newline_tbl["\r"] = "<br>";//Mac OS up to version 9
		newline_tbl["\n"] = "<br>";//Unix and Unix-like systems
		replace(newline_tbl, get_dict_info("description").c_str(), descr2);
		xml::decode(descr2);

		if (!descr2.empty())
			f << "description=" << descr2 << "\n";

		return true;
	}

	std::string StarDictGenerator::get_current_date(void)
	{
		/* Get the current time. */
		time_t curtime = time(NULL);

		/* Convert it to local time representation. */
		struct tm *loctime = localtime (&curtime);
		gchar buf[256];
		strftime(buf, sizeof(buf), "%Y.%m.%d", loctime);

		return buf;
	}

}//end of anonymous namespace

#if 0
int main(int argc, char *argv[])
{
	return StarDictGenerator().run(argc, argv);
}
#endif
