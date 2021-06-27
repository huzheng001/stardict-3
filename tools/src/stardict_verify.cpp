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
#include <sstream>

#include "lib_dict_verify.h"

const char* verif_dir_failure = "Directory '%s'. Verification result: failure\n";
const char* verif_dir_success = "Directory '%s'. Verification result: success\n";
const char* verif_dict_failure = "Dictionary '%s'. Verification result: failure\n";
const char* verif_dict_success = "Dictionary '%s'. Verification result: success\n";

struct Main
{
	int main(int argc, char * argv [])
	{
		quiet = FALSE;
		static GOptionEntry entries[] = {
			{ "quiet", 'q', 0, G_OPTION_ARG_NONE, &quiet, "show only whether the dictionary is OK or broken", NULL },
			{ NULL },
		};
		glib::OptionContext opt_cnt(g_option_context_new("[FILE | DIRECTORY]..."));
		g_option_context_add_main_entries(get_impl(opt_cnt), entries, NULL);
		g_option_context_set_help_enabled(get_impl(opt_cnt), TRUE);
		g_option_context_set_summary(get_impl(opt_cnt),
				"Verifies StarDict dictionary files.\n"
				"\n"
				"You may specify files and directories in the command line.\n"
				"Files should reference the particular dictionary to check, they must have .ifo extension.\n"
				"Directories are processed recursively, all encountered dictionary files will be verified.\n"
				"If you omit command line arguments, the utility will process known dictionary directories. "
				"That is directories where StarDict search for dictionaries by default.\n"
				"\n"
				"EXIT STATUS\n"
				"The utility exits with status 0 if all processed dictionaries are valid. "
				"If at least one dictionary is broken, the status will be non-zero.\n"
			);
		glib::Error err;
		if (!g_option_context_parse(get_impl(opt_cnt), &argc, &argv, get_addr(err))) {
			std::cerr << "Option parsing failed: " <<  err->message << std::endl;
			return EXIT_FAILURE;
		}
		files = argv+1;
		file_cnt = argc-1;
		return EXIT_SUCCESS;
	}
	char** files;
	int file_cnt;
	gboolean quiet;
};

Main gmain;

static int verify_dir(const std::string& dirname)
{
	int res = EXIT_SUCCESS;
	GDir *dir = g_dir_open(dirname.c_str(), 0, NULL);
	if (dir) {
		const gchar *filename;
		std::string fullfilename;
		while ((filename = g_dir_read_name(dir))!=NULL) {
			fullfilename = dirname + "/" + filename;
			if (g_file_test(fullfilename.c_str(), G_FILE_TEST_IS_DIR)) {
				if(verify_dir(fullfilename))
					res = EXIT_FAILURE;
			}
			else if (g_str_has_suffix(filename,".ifo")) {
				bool dict_res = (VERIF_RESULT_CRITICAL <= stardict_verify(fullfilename.c_str()));
				if(dict_res)
					res = EXIT_FAILURE;
				if(gmain.quiet)
					g_print(dict_res ? verif_dict_failure : verif_dict_success, fullfilename.c_str());
				else
					g_print("\n\n");
			}
		}
		g_dir_close(dir);
	}
	return res;
}

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
	int res = EXIT_SUCCESS;
	if (gmain.file_cnt > 0) {
		for(int i=0; i<gmain.file_cnt; ++i) {
			const char* file_path = gmain.files[i];
			if(g_file_test(file_path, G_FILE_TEST_IS_DIR)) {
				g_message("Processing directory: '%s'\n", file_path);
				if(verify_dir(file_path)) {
					res = EXIT_FAILURE;
					g_message(verif_dir_failure, file_path);
				} else
					g_message(verif_dir_success, file_path);
			} else {
				bool dict_res = (VERIF_RESULT_CRITICAL <= stardict_verify(file_path));
				if(dict_res)
					res = EXIT_FAILURE;
				if(gmain.quiet)
					g_print(dict_res ? verif_dict_failure : verif_dict_success, file_path);
			}
			if(!gmain.quiet)
				g_print("\n\n");
		}
	} else {
		const std::string system_dir("/usr/share/stardict/dic");
		const std::string home_dir = std::string(g_get_home_dir()) + "/.stardict/dic";
		g_message("Verifying dictionaries in known directories...\n");
		g_message("\nProcessing system dictionary directory: '%s'\n", system_dir.c_str());
		if(verify_dir(system_dir)) {
			res = EXIT_FAILURE;
			g_message(verif_dir_failure, system_dir.c_str());
		} else
			g_message(verif_dir_success, system_dir.c_str());
		g_message("\nProcessing private user dictionary directory: '%s'\n", home_dir.c_str());
		if(verify_dir(home_dir)) {
			res = EXIT_FAILURE;
			g_message(verif_dir_failure, home_dir.c_str());
		} else
			g_message(verif_dir_success, home_dir.c_str());
	}

	return res;
}
