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

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <iostream>
#include <string>
#include <stdlib.h>
#include <locale.h>
#include <glib.h>
#include "lib_stardict_bin2text.h"
#include "libcommon.h"

struct Main
{
	int main(int argc,char * argv [])
	{
		chunk_size = 0;
		static GOptionEntry entries[] = {
			{ "chunk-size", 's', 0, G_OPTION_ARG_INT, &chunk_size, "split .xml file into chunks of this size", "SIZE" },
			{ NULL },
		};
		glib::OptionContext opt_cnt(g_option_context_new("DICTIONARY.ifo OUTPUT.xml"));
		g_option_context_add_main_entries(get_impl(opt_cnt), entries, NULL);
		g_option_context_set_help_enabled(get_impl(opt_cnt), TRUE);
		g_option_context_set_summary(get_impl(opt_cnt),
			"Convert binary dictionary into textual form.\n"
			"\n"
			"If produced xml file is large, you may experience problems viewing or editing it. "
			"Try to split the output file into chunks using 'chunk-size' option. "
			"The option takes an integer argument specifying critical file size in bytes. "
			"As soon as file overgrow the limit, new file is started. "
			"We do not break articles into parts. Each article must be fully enclosed in one file. "
			"New file is started with new article, the previous article is placed in the file "
			"that overgrew the limit. "
			"Actual size of a chunk file is normally larger than the specified limit.\n"
		);
		glib::Error err;
		if (!g_option_context_parse(get_impl(opt_cnt), &argc, &argv, get_addr(err))) {
			std::cerr << "Option parsing failed: " <<  err->message << std::endl;
			return EXIT_FAILURE;
		}
		if(chunk_size < 0) {
			std::cerr << "Chunk size must be >= 0" << std::endl;
			return EXIT_FAILURE;
		}
		if(argc < 2) {
			std::cerr << "Dictionary file is not specified." << std::endl;
			return EXIT_FAILURE;
		}
		if(argc < 3) {
			std::cerr << "Output file is not specified." << std::endl;
			return EXIT_FAILURE;
		}
		if(argc > 3) {
			std::cerr << "Too many files, two files are needed. Extra files will be ignored." << std::endl;
		}
		ifofilename = argv[1];
		xmlfilename = argv[2];
		return EXIT_SUCCESS;
	}
	gint chunk_size;
	std::string ifofilename;
	std::string xmlfilename;
};

int main(int argc,char * argv [])
{
	setlocale(LC_ALL, "");
	Main oMain;
	if(oMain.main(argc, argv))
		return EXIT_FAILURE;
	return stardict_bin2text(oMain.ifofilename, oMain.xmlfilename, oMain.chunk_size);
}
