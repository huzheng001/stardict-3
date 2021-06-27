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

#include <stdlib.h>
#include <locale.h>
#include <glib.h>
#include <string>
#include <iostream>
#include "libcommon.h"
#include "lib_stardict_text2bin.h"


struct Main
{
	int main(int argc,char * argv [])
	{
		show_xinclude = FALSE;
		use_same_type_sequence = TRUE;
		static GOptionEntry entries[] = {
			{ "show-xinclude", 'i', 0, G_OPTION_ARG_NONE, &show_xinclude, "show each processed xinclude", NULL },
			{ "no-same-type-sequence", 0, G_OPTION_FLAG_REVERSE, G_OPTION_ARG_NONE, &use_same_type_sequence, "no sametypesequence optimization", NULL },
			{ NULL },
		};
		glib::OptionContext opt_cnt(g_option_context_new("TEXT_DICTIONARY.xml DICTIONARY.ifo"));
		g_option_context_add_main_entries(get_impl(opt_cnt), entries, NULL);
		g_option_context_set_help_enabled(get_impl(opt_cnt), TRUE);
		g_option_context_set_summary(get_impl(opt_cnt),
				"Convert textual dictionary into binary form.\n"
				"\n"
				"BUGS\n"
				"\n"
				"Line numbers larger than 2^16-1 = 65535 are printed incorrectly (libxml limitation).\n"
				"\n"
				"Name of the current file is not printed. You can only see a line number. "
				"When xinclude-s are used, you can only guess the file the error message applies to. "
				"Try the show-xinclude option.\n"
				"\n"
				"show-xinclude option does not print all xinclude-d files. "
				"It prints only every seventh processed file. (libxml limitation?)\n"
				"\n"
				"You cannot detect when an xinclude-d file is done and the parser continues "
				"to process the xinclude-ing file.\n"
			);
		glib::Error err;
		if (!g_option_context_parse(get_impl(opt_cnt), &argc, &argv, get_addr(err))) {
			std::cerr << "Option parsing failed: " <<  err->message << std::endl;
			return EXIT_FAILURE;
		}
		if(argc < 2) {
			std::cerr << "Textual dictionary file is not specified." << std::endl;
			return EXIT_FAILURE;
		}
		if(argc < 3) {
			std::cerr << "Output file is not specified." << std::endl;
			return EXIT_FAILURE;
		}
		if(argc > 3) {
			std::cerr << "Too many files, two files are needed. Extra files will be ignored." << std::endl;
		}
		xmlfilename = argv[1];
		ifofilename = argv[2];
		return EXIT_SUCCESS;
	}
	std::string ifofilename;
	std::string xmlfilename;
	gboolean show_xinclude;
	gboolean use_same_type_sequence;
};

int main(int argc,char * argv [])
{
	setlocale(LC_ALL, "");
	Main oMain;
	if(oMain.main(argc, argv))
		return EXIT_FAILURE;
	return stardict_text2bin(oMain.xmlfilename, oMain.ifofilename,
		oMain.show_xinclude, oMain.use_same_type_sequence);
}
