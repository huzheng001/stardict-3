#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <cstring>
#include <iostream>
#include <vector>
#include <algorithm>
#include "lib_stardict_text2bin.h"
#include "libcommon.h"
#include "lib_common_dict.h"
#include "lib_textual_dict_parser.h"
#include "lib_binary_dict_generator.h"
#include "lib_dict_repair.h"

int stardict_text2bin(const std::string& xmlfilename, const std::string& ifofilename,
		bool show_xincludes, bool use_same_type_sequence)
{
	common_dict_t norm_dict;
	if(parse_textual_dict(xmlfilename, &norm_dict,
			show_xincludes))
		return EXIT_FAILURE;
	if(repair_dict(norm_dict))
		return EXIT_FAILURE;
	binary_dict_gen_t generator;
	generator.set_use_same_type_sequence(use_same_type_sequence);
	generator.set_compress_dict(true);
	g_message("Generating dictionary '%s'...", ifofilename.c_str());
	if(generator.generate(ifofilename, &norm_dict)) {
		g_critical("Generation failed.");
		return EXIT_FAILURE;
	}
	g_message("Generation done.");
	return EXIT_SUCCESS;
}
