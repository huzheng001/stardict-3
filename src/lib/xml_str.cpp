#include "xml_str.h"
#include <string.h>
#include <glib.h>

static const char* xml_entrs[] = { "lt;", "gt;", "amp;", "apos;", "quot;", 0 };
static const int xml_ent_len[] = { 3,     3,     4,      5,       5,       0 };
static const char raw_entrs[] =  { '<',   '>',   '&',    '\'',    '\"',    0 };

size_t xml_utf8_strlen(const char *str)
{
	const char *q;
	size_t cur_pos;
	int i;

	for (cur_pos = 0, q = str; *q; ++cur_pos) {
		if (*q == '&') {
			for (i = 0; xml_entrs[i]; ++i)
				if (strncmp(xml_entrs[i], q + 1, xml_ent_len[i]) == 0) {
					q += xml_ent_len[i] + 1;
					break;
				}
			if (xml_entrs[i] == NULL)
				++q;
		} else if (*q == '<') {
			const char *p = strchr(q+1, '>');
			if (p)
				q = p + 1;
			else
				/* Not closed tag, assume it will be closed later, after 
				 * appending new data. */
				break;
			--cur_pos;
		} else
			q = g_utf8_next_char(q);
	}

	return cur_pos;
}

void xml_utf8_decode(const char *str, std::string& decoded)
{
	int ient;
	const char *amp = strchr(str, '&');

	if (amp == NULL) {
		decoded = str;
		return;
	}
	decoded.assign(str, amp - str);

	while (*amp)
		if (*amp == '&') {
			for (ient = 0; xml_entrs[ient] != 0; ++ient)
				if (strncmp(amp + 1, xml_entrs[ient], xml_ent_len[ient]) == 0) {
					decoded += raw_entrs[ient];
					amp += xml_ent_len[ient]+1;
					break;
				}
			if (xml_entrs[ient] == 0)    // unrecognized sequence
				decoded += *amp++;
		} else {
			decoded += *amp++;
		}
}

/* returns string index at char_offset characters offset.
 * Index may be in range [0, str.length()].
 * returns std::string::npos if index is out of range */
size_t xml_utf8_get_index_at_offset(const std::string& str, size_t char_offset)
{
	const char *beg = str.c_str();
	const char *end = xml_utf8_offset_to_pointer(beg, char_offset);
	return (end == NULL) ? std::string::npos : end - beg;
}

/* returns a pointer char_offset chars forward from str
 * terminating '\0' character is countered as a valid char, so if the requested 
 * offset = string length, pointer to the '\0' characters is returned.
 * returns NULL if the string is exhausted before the required offset reached. */
const char* xml_utf8_offset_to_pointer(const char *str, size_t char_offset)
{
	if(!str)
		return NULL;

	const char *r = NULL, *q = NULL;
	size_t cur_pos;
	int i;

	for(cur_pos = static_cast<size_t>(-1), q = str; *q && cur_pos != char_offset; ++cur_pos) {
		r = q;
		if (*q == '&') {
			for (i = 0; xml_entrs[i]; ++i)
				if (strncmp(xml_entrs[i], q + 1, xml_ent_len[i]) == 0) {
					q += xml_ent_len[i] + 1;
					break;
				}
			if (xml_entrs[i] == NULL)
				++q;
		} else if (*q == '<') {
			const char *p = strchr(q+1, '>');
			if (p)
				q = p + 1;
			else
				/* Not closed tag, assume it will be closed later, after 
				 * appending new data. */
				break;
			--cur_pos;
		} else
			q = g_utf8_next_char(q);
	}
	if(!*q && cur_pos != char_offset) {
		r = q;
		++cur_pos;
	}
	return (cur_pos == char_offset) ? r : NULL;
}

/* returns pointer to the last char of the unicode char 
 * str must point to the first char of the unicode char 
 * str must not point to the tag start char '<' */
const char* xml_utf8_end_of_char(const char *str)
{
	if(!str)
		return NULL;
	const char *q = str;
	int i;
	if (*q == '&') {
		for (i = 0; xml_entrs[i]; ++i)
			if (strncmp(xml_entrs[i], q + 1, xml_ent_len[i]) == 0) {
				return q + xml_ent_len[i];
			}
		return q;
	} else if (*q == '<') {
		g_warning("input string points to '<' char");
		return NULL;
	} else {
		q = g_utf8_next_char(q);
		return q-1;
	}
}
