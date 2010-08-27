#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <list>
#include <string>
#include <glib.h>
#include <cstdlib>
#include <cstring>
#include <libxml/chvalid.h>
#include "lib_xml_utils.h"

int check_xml_string_chars(const char* str, std::list<const char*>& invalid_chars)
{
	return check_xml_string_chars(str, strlen(str), invalid_chars);
}

/* check string str for invalid chars
 * str - a valid utf8 string
 *
 * Only chars satisfying the following production in the XML specification are allowed:
 *
 * [2] Char ::= #x9 | #xA | #xD | [#x20-#xD7FF] | [#xE000-#xFFFD]
 *                  | [#x10000-#x10FFFF]
 *
 * The function returns EXIT_SUCCESS if all chars of the string are valid,
 * and EXIT_FAILURE otherwise. In the later case invalid_chars list is populated with
 * references to invalid chars found. invalid_chars[i] points to the first byte of
 * the invalid char in the str string.
 * */
int check_xml_string_chars(const char* str, const size_t len, std::list<const char*>& invalid_chars)
{
	invalid_chars.clear();
	for(const char* p = str; p < str + len; p = g_utf8_next_char(p)) {
		if(!xmlIsCharQ(g_utf8_get_char(p)))
			invalid_chars.push_back(p);
	}
	return invalid_chars.empty() ? EXIT_SUCCESS : EXIT_FAILURE;
}

void fix_xml_string_chars(const char* src, std::string& dst)
{
	fix_xml_string_chars(src, strlen(src), dst);
}

/* copy source string str into destination string dst dropping invalid chars.
 * For definition of an invalid char, see check_xml_string_chars function.
 * src and dst may be the same string:
 *
 * std::string str;
 * ...
 * fix_xml_string_chars(str.c_str(), str);
 * */
void fix_xml_string_chars(const char* src, const size_t len, std::string& dst)
{
	std::string temp;
	temp.reserve(len);
	for(const char* p = src; p < src + len; p = g_utf8_next_char(p)) {
		if(xmlIsCharQ(g_utf8_get_char(p))) {
			const char* q = g_utf8_next_char(p);
			temp.append(p, q-p);
		}
	}
	std::swap(dst, temp);
}
