/*
 * Copyright 2011 kubtek <kubtek@mail.com>
 *
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

#ifndef LIB_CHARS_H_
#define LIB_CHARS_H_

#include <list>
#include <string>

#define key_forbidden_chars \
	"\n\r"
#define key_forbidden_chars_ex \
	key_forbidden_chars " \t"

extern int check_xml_string_chars(const char* str, std::list<const char*>& invalid_chars);
extern int check_xml_string_chars(const char* str, const size_t len, std::list<const char*>& invalid_chars);
extern void fix_xml_string_chars(const char* src, std::string& dst);
extern void fix_xml_string_chars(const char* src, const size_t len, std::string& dst);

extern int check_stardict_string_chars(const char* str, std::list<const char*>& invalid_chars);
extern int check_stardict_string_chars(const char* str, const size_t len, std::list<const char*>& invalid_chars);
extern void fix_stardict_string_chars(const char* src, std::string& dst);
extern void fix_stardict_string_chars(const char* src, const size_t len, std::string& dst);

extern int check_stardict_key_chars(const char* str);
extern void fix_stardict_key_chars(const char* str, std::string& dst);

#endif /* LIB_CHARS_H_ */
