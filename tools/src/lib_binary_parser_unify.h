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

#ifndef LIB_BINARY_PARSER_UNIFY_H_
#define LIB_BINARY_PARSER_UNIFY_H_

#include "lib_common_dict.h"
#include "lib_binary_dict_parser.h"
#include "libcommon.h"

/* convert binary dictionary parser results into common dictionary format */
int convert_to_parsed_dict(common_dict_t& parsed_norm_dict, const binary_dict_parser_t& norm_dict);

#endif /* LIB_BINARY_PARSER_UNIFY_H_ */
