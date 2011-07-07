/*
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

#include <string>
#include <iostream>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <iomanip>
#include "libcommon.h"


class Main {
public:
	int main(int argc, char * argv [])
	{
		if(ParseCommandLine(argc, argv))
			return EXIT_FAILURE;
		print_index(idx_file_name);
		return EXIT_SUCCESS;
	}
private:
	int ParseCommandLine(int argc, char * argv [])
	{
		quiet_mode = FALSE;
		key_only = FALSE;
		static GOptionEntry entries[] = {
			{ "quiet", 'q', 0, G_OPTION_ARG_NONE, &quiet_mode, "no additional information, only index entries", NULL },
			{ "key-only", 'k', 0, G_OPTION_ARG_NONE, &key_only, "print only keys (implies --quiet option)", NULL },
			{ NULL },
		};
		glib::OptionContext opt_cnt(g_option_context_new("INDEX_FILE"));
		g_option_context_add_main_entries(get_impl(opt_cnt), entries, NULL);
		g_option_context_set_help_enabled(get_impl(opt_cnt), TRUE);
		g_option_context_set_summary(get_impl(opt_cnt),
			"Print context of StarDict index file in human readable form.\n"
			"\n"
			"Supported files: .idx, .ridx, .syn\n"
			);
		glib::Error err;
		if (!g_option_context_parse(get_impl(opt_cnt), &argc, &argv, get_addr(err))) {
			std::cerr << "Option parsing failed: " <<  err->message << std::endl;
			return EXIT_FAILURE;
		}
		if(argc == 1) {
			std::cerr << "Index file is not specified." << std::endl;
			return EXIT_FAILURE;
		}
		if(argc > 2) {
			std::cerr << "warning: only the first file will be processed." <<std::endl;
		}
		idx_file_name = argv[1];
		if(!g_str_has_suffix(idx_file_name.c_str(), ".idx")
				&& !g_str_has_suffix(idx_file_name.c_str(), ".syn")
				&& !g_str_has_suffix(idx_file_name.c_str(), ".ridx")) {
			std::cerr << "Unsupported index type." << std::endl;
			return EXIT_FAILURE;
		}
		syn_file = g_str_has_suffix(idx_file_name.c_str(), ".syn");
		if(key_only)
			quiet_mode = TRUE;
		return EXIT_SUCCESS;
	}
	void print_index(std::string& idx_file_name)
	{
		glib::CharStr contents;
		gsize cont_len;
		if(!g_file_get_contents(idx_file_name.c_str(), get_addr(contents), &cont_len, NULL)) {
			std::cerr << "Unable to open file " << idx_file_name << std::endl;
			return;
		}
		if(!quiet_mode) {
			if(syn_file)
				std::cout << "     INDEX KEY" << std::endl;
			else
				std::cout << "    OFFSET       SIZE KEY" << std::endl;
		}
		gchar *p1 = get_impl(contents);
		gchar *end = p1+cont_len;
		int rec_no = 0;
		const gchar* key;
		if(syn_file) {
			guint32 index;
			while(p1<end) {
				key = p1;
				p1 += strlen(p1) + 1;
				index = g_ntohl(*reinterpret_cast<guint32*>(p1));
				p1 += sizeof(guint32);
				++rec_no;
				if(key_only)
					std::cout << key << std::endl;
				else
					std::cout << std::setw(10) << index << " " << key << std::endl;
			}
		} else {
			guint32 offset, size;
			while(p1<end) {
				key = p1;
				p1 += strlen(p1) + 1;
				offset = g_ntohl(*reinterpret_cast<guint32*>(p1));
				p1 += sizeof(guint32);
				size = g_ntohl(*reinterpret_cast<guint32*>(p1));
				p1 += sizeof(guint32);
				++rec_no;
				if(key_only)
					std::cout << key << std::endl;
				else
					std::cout << std::setw(10) << offset << " " << std::setw(10) << size << " "<< key << std::endl;
			}
		}
		if(!quiet_mode)
			std::cout << "number of entries: " << rec_no << std::endl;
	}
private:
	std::string idx_file_name;
	gboolean quiet_mode;
	gboolean key_only;
	gboolean syn_file;
};


int
main(int argc, char * argv [])
{
	setlocale(LC_ALL, "");
	Main oMain;
	return oMain.main(argc, argv);
}
