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

#ifndef LIB_STARDICT_TEXT2BIN_H_
#define LIB_STARDICT_TEXT2BIN_H_

#include <string>

extern int stardict_text2bin(const std::string& xmlfilename, const std::string& ifofilename,
		bool show_xincludes, bool use_same_type_sequence);

#endif /* LIB_STARDICT_TEXT2BIN_H_ */
