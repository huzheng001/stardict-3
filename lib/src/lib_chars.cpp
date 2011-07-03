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

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <list>
#include <string>
#include <glib.h>
#include <cstdlib>
#include <cstring>
#include <libxml/chvalid.h>
#include "lib_chars.h"

/*
 * Only chars satisfying the following production in the XML specification are allowed:
 *
 * [2] Char ::= #x9 | #xA | #xD | [#x20-#xD7FF] | [#xE000-#xFFFD]
 *                  | [#x10000-#x10FFFF]
 * */
inline
bool is_valid_xml_char(gunichar gch)
{
	return xmlIsCharQ(gch);
}

/* characters allowed in StarDict dictionary (keys, synonyms, definition text).
 * We allow all valid Unicode chars excluding control chars. */
inline
bool is_valid_stardict_char(gunichar gch)
{
	return gch == 0x9 || gch == 0xa || gch == 0xd
		|| (0x20 <= gch && gch <= 0x7e)
		|| gch == 0x85
		|| (0xa0 <= gch && gch <= 0xff)
		|| (0x100 <= gch && gch <= 0xd7ff)
		|| (0xe000 <= gch && gch <= 0xfffd)
		|| (0x10000 <= gch && gch <= 0x10ffff);
}

/* check string str for invalid chars
 * str - a valid utf8 string
 *
 * The function returns EXIT_SUCCESS if all chars of the string are valid,
 * and EXIT_FAILURE otherwise. In the later case invalid_chars list is populated with
 * references to invalid chars found. invalid_chars[i] points to the first byte of
 * the invalid char in the str string.
 * */
template<class Func>
int check_string_chars(const char* str, const size_t len, std::list<const char*>& invalid_chars, Func is_valid_char)
{
	invalid_chars.clear();
	for(const char* p = str; p < str + len; p = g_utf8_next_char(p)) {
		if(!is_valid_char(g_utf8_get_char(p)))
			invalid_chars.push_back(p);
	}
	return invalid_chars.empty() ? EXIT_SUCCESS : EXIT_FAILURE;
}

/* copy source string str into destination string dst dropping invalid chars.
 * For definition of an invalid char, see check_xml_string_chars function.
 * src and dst may be the same string:
 *
 * std::string str;
 * ...
 * fix_xml_string_chars(str.c_str(), str);
 * */
template<class Func>
void fix_string_chars(const char* src, const size_t len, std::string& dst,  Func is_valid_char)
{
	std::string temp;
	temp.reserve(len);
	for(const char* p = src; p < src + len; p = g_utf8_next_char(p)) {
		if(is_valid_char(g_utf8_get_char(p))) {
			const char* q = g_utf8_next_char(p);
			temp.append(p, q-p);
		}
	}
	std::swap(dst, temp);
}

int check_xml_string_chars(const char* str, std::list<const char*>& invalid_chars)
{
	return check_xml_string_chars(str, strlen(str), invalid_chars);
}

int check_xml_string_chars(const char* str, const size_t len, std::list<const char*>& invalid_chars)
{
	return check_string_chars(str, len, invalid_chars, is_valid_xml_char);
}

void fix_xml_string_chars(const char* src, std::string& dst)
{
	fix_xml_string_chars(src, strlen(src), dst);
}

void fix_xml_string_chars(const char* src, const size_t len, std::string& dst)
{
	fix_string_chars(src, len, dst, is_valid_xml_char);
}

int check_stardict_string_chars(const char* str, std::list<const char*>& invalid_chars)
{
	return check_stardict_string_chars(str, strlen(str), invalid_chars);
}

int check_stardict_string_chars(const char* str, const size_t len, std::list<const char*>& invalid_chars)
{
	return check_string_chars(str, len, invalid_chars, is_valid_stardict_char);
}

void fix_stardict_string_chars(const char* src, std::string& dst)
{
	fix_stardict_string_chars(src, strlen(src), dst);
}

void fix_stardict_string_chars(const char* src, const size_t len, std::string& dst)
{
	fix_string_chars(src, len, dst, is_valid_stardict_char);
}

int check_stardict_key_chars(const char* str)
{
	return strpbrk(str, key_forbidden_chars) ?  EXIT_FAILURE : EXIT_SUCCESS;
}

/* in addition to removing key_forbidden_chars
 * - remove leading and trailing ' ' and '\t',
 * - consecutive ' ' and '\t' transform to one ' '*/
void fix_stardict_key_chars(const char* str, std::string& dst)
{
	dst.clear();
	dst.reserve(strlen(str));
	while(*str && strchr(key_forbidden_chars_ex, *str))
		++str;
	if(!*str)
		return;
	while(true) {
		while(*str && !strchr(key_forbidden_chars_ex, *str)) {
			dst += *str;
			++str;
		}
		if(!*str)
			return;
		while(*str && strchr(key_forbidden_chars_ex, *str))
			++str;
		if(!*str)
			return;
		dst += ' ';
	}
}
