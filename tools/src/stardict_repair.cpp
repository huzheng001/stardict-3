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
#include <string>
#include <iostream>
#include <sstream>
#include "libcommon.h"
#include "lib_stardict_repair.h"

const char* repair_dict_failure = "Dictionary '%s'. Repair result: failure\n";
const char* repair_dict_success = "Dictionary '%s'. Repair result: success\n";

struct Main
{
	int main(int argc, char * argv [])
	{
		files = NULL;
		outdirpath = ".";
		quiet = FALSE;
		lot_of_memory = FALSE;
		compress_dict = FALSE;
		copy_res_store = TRUE;
		char* out_dir = NULL;
		static GOptionEntry entries[] = {
			{ "quiet", 'q', 0, G_OPTION_ARG_NONE, &quiet, "show only whether the dictionary was repaired or not", NULL },
			{ "lot-of-memory", 0, 0, G_OPTION_ARG_NONE, &lot_of_memory, "store data in memory when possible (vs. temporary files)", NULL },
			{ "compress-dict", 0, 0, G_OPTION_ARG_NONE, &compress_dict, "compress dictionary - produce DICT.dict.dz file", NULL },
			{ "no-copy-res-store", 0, G_OPTION_FLAG_REVERSE, G_OPTION_ARG_NONE, &copy_res_store, "prevent copying resource storage data", NULL },
			{ "out-dir", 'O', 0, G_OPTION_ARG_FILENAME, &out_dir, "output directory (\".\" by default)", "DIR" },
			{ NULL },
		};
		glib::OptionContext opt_cnt(g_option_context_new(" FILE.ifo ..."));
		g_option_context_add_main_entries(get_impl(opt_cnt), entries, NULL);
		g_option_context_set_help_enabled(get_impl(opt_cnt), TRUE);
		g_option_context_set_summary(get_impl(opt_cnt),
				"Repairs StarDict dictionary files\n"
				"\n"
				"The utility accepts a list of files to repair. That must be files with .ifo extensions. "
				"By default the utility creates files in the current directory, "
				"you may specify different directory with --out-dir option.\n"
				"\n"
				"Note that output directory must differ from the dictionary directory, "
				"that is the directory containing input dictionary. "
				"Output directory must exist! It is better when the output directory is empty. "
				"The utility silently overwrites any file in the output directory. "
				"Original dictionaries are never changed.\n"
				"\n"
				"Note. '--compress-dict' option requires dictzip utility.\n"
				"\n"
				"EXIT STATUS\n"
				"The utility exits with status 0 if conversion succeeds, with non-zero status otherwise."
			);
		glib::Error err;
		if (!g_option_context_parse(get_impl(opt_cnt), &argc, &argv, get_addr(err))) {
			std::cerr << "Option parsing failed: " <<  err->message << std::endl;
			return EXIT_FAILURE;
		}
		if(argc < 2) {
			std::cerr << "No files to repair." << std::endl;
			return EXIT_FAILURE;
		}
		files = argv+1;
		if(out_dir)
			outdirpath = out_dir;
		return EXIT_SUCCESS;
	}
	char** files;
	std::string outdirpath;
	gboolean quiet;
	gboolean lot_of_memory;
	gboolean compress_dict;
	gboolean copy_res_store;
};

Main gmain;

static void log_handler(const gchar * log_domain,
			       GLogLevelFlags log_level,
			       const gchar *message,
			       gpointer user_data)
{
	if(gmain.quiet)
		return;
	std::stringstream buf;
	if(log_domain && log_domain[0])
		buf << "(" << log_domain << ") ";
	if(log_level & G_LOG_LEVEL_ERROR)
		buf << "[error] ";
	else if(log_level & G_LOG_LEVEL_CRITICAL)
		buf << "[critical] ";
	else if(log_level & G_LOG_LEVEL_WARNING)
		buf << "[warning] ";
	else if(log_level & G_LOG_LEVEL_MESSAGE)
		buf << "[message] ";
	else if(log_level & G_LOG_LEVEL_INFO)
		buf << "[info] ";
	else if(log_level & G_LOG_LEVEL_DEBUG)
		buf << "[debug] ";
	if(message)
		buf << message;
	buf << "\n";
	std::cout << buf.str();
}

int main(int argc,char * argv [])
{
	setlocale(LC_ALL, "");

	if(gmain.main(argc, argv))
		return EXIT_FAILURE;
	g_log_set_default_handler(log_handler, NULL);
	RepairOptions options;
	options.lot_of_memory = !!gmain.lot_of_memory;
	options.compress_dict = !!gmain.compress_dict;
	options.copy_res_store = !!gmain.copy_res_store;
	int res_total = EXIT_SUCCESS;
	for(int i=0; gmain.files[i]; ++i) {
		int res = stardict_repair(gmain.files[i], gmain.outdirpath, options);
		if(res)
			res_total = EXIT_FAILURE;
		g_print((res ? repair_dict_failure : repair_dict_success), gmain.files[i]);
	}
	return res_total;
}
