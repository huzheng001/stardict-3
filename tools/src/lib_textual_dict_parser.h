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

#ifndef LIB_TEXTUAL_DICT_PARSER_H_
#define LIB_TEXTUAL_DICT_PARSER_H_

#include <string>

class common_dict_t;

/* Return value:
 * EXIT_FAILURE or EXIT_SUCCESS */
int parse_textual_dict(const std::string& xmlfilename, common_dict_t* norm_dict,
		bool show_xincludes);

#endif /* LIB_TEXTUAL_DICT_PARSER_H_ */
