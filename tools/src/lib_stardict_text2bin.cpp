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

#include <cstdlib>
#include <cstring>
#include <string>
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
