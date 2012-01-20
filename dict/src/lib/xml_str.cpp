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

#include <cstring>
#include <glib.h>
#include <algorithm>
#include "xml_str.h"

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


void XMLCharData::clear(void)
{
	delete [] byte_inds;
	byte_inds = NULL;
	delete [] char_data_str;
	char_data_str = NULL;
	size = 0;
	g_free(xml_str);
	xml_str = NULL;
}

void XMLCharData::allocate(const char *xml_str_)
{
	clear();
	if(!xml_str_)
		return;
	size = strlen(xml_str_) + 1; // allocate more than really needed
	byte_inds = new size_t[size];
	char_data_str = new char[size];
	xml_str = g_strdup(xml_str_);
}

void XMLCharData::assign_xml(const char *xml_str_)
{
	clear();
	if(!xml_str_)
		return;
	allocate(xml_str_);
	size_t xml_ind = 0;
	size_t cd_ind = 0; // char data string index
	while(xml_str[xml_ind]) {
		if(xml_str[xml_ind] == '&') {
			int i;
			for (i = 0; xml_entrs[i]; ++i)
				if (strncmp(xml_entrs[i], xml_str + xml_ind + 1, xml_ent_len[i]) == 0) {
					char_data_str[cd_ind] = raw_entrs[i];
					byte_inds[cd_ind] = xml_ind;
					cd_ind += 1;
					xml_ind += xml_ent_len[i] + 1;
					break;
				}
			if (xml_entrs[i] == NULL) {
				char_data_str[cd_ind] = xml_str[xml_ind];
				byte_inds[cd_ind] = xml_ind;
				cd_ind += 1;
				xml_ind += 1;
			}
		} else if(xml_str[xml_ind] == '<') {
			const char* p = strchr(xml_str + xml_ind + 1, '>');
			if(!p) {
				p = strchr(xml_str + xml_ind + 1, '\0');
				xml_ind = p - xml_str;
			} else {
				xml_ind = (p - xml_str) + 1;
			}
		} else {
			const char* p = g_utf8_next_char(xml_str + xml_ind);
			const size_t nbytes = p - (xml_str + xml_ind);
			strncpy(char_data_str + cd_ind, xml_str + xml_ind, nbytes);
			std::fill(byte_inds + cd_ind, byte_inds + cd_ind + nbytes, xml_ind);
			xml_ind += nbytes;
			cd_ind += nbytes;
		}
	}
	// cd_ind = number of filled in bytes in char_data_str
	char_data_str[cd_ind] = '\0';
	byte_inds[cd_ind] = xml_ind;
	g_assert(cd_ind + 1 <= size);
	size = cd_ind + 1;
}

void XMLCharData::mark_substring(std::string& out, const char* start_tag,
	const char* end_tag, size_t cd_begin_ind, size_t cd_len) const
{
	if(size == 0)
		return;
	g_assert(start_tag && end_tag);
	if(!start_tag || !end_tag)
		return;
	if(cd_begin_ind >= size-1)
		return;
	if(cd_begin_ind + cd_len >= size)
		cd_len = size - 1 - cd_begin_ind;
	if(cd_len == 0)
		return;
	const char* xml_p = xml_str + byte_inds[cd_begin_ind];
	const char* cd_p = char_data_str + cd_begin_ind;
	const char* xml_b, *xml_p2;
	//const char* cd_b;
	const char* cd_p2;
	while(true) {
		xml_b = xml_p;
		//cd_b = cd_p;
		while(true) {
			xml_p2 = xml_utf8_end_of_char(xml_p)+1;
			cd_p2 = g_utf8_next_char(cd_p);
			if(cd_p2 - (char_data_str + cd_begin_ind) >= int(cd_len)) {
				g_assert(cd_p2 - (char_data_str + cd_begin_ind) == int(cd_len));
				break;
			}
			if(xml_p2 - xml_str == int(byte_inds[cd_p2 - char_data_str])) {
				xml_p = xml_p2;
				cd_p = cd_p2;
			} else {
				g_assert(*xml_p2 == '<');
				break;
			}
		}
		out.append(start_tag);
		out.append(xml_b, xml_p2 - xml_b);
		out.append(end_tag);
		if(cd_p2 - (char_data_str + cd_begin_ind) >= int(cd_len))
			break;
		out.append(xml_p2, byte_inds[cd_p2 - char_data_str] - (xml_p2 - xml_str));
		xml_p = xml_str + byte_inds[cd_p2 - char_data_str];
		cd_p = cd_p2;
	}
}

void XMLCharData::copy_xml(std::string& out, 
	size_t cd_begin_ind, size_t cd_end_ind) const
{
	g_assert(size>0);
	if(size == 0)
		return;
	g_assert(cd_begin_ind <= cd_end_ind && cd_end_ind < size);
	if(cd_begin_ind > cd_end_ind || cd_end_ind >= size)
		return;
	const char* xml_b, *xml_e;
	if(cd_begin_ind > 0) {
		xml_b = xml_utf8_end_of_char(xml_str + byte_inds[cd_begin_ind-1]) + 1;
	} else
		xml_b = xml_str;
	xml_e = xml_str + byte_inds[cd_end_ind];
	if(xml_b < xml_e)
		out.append(xml_b, xml_e - xml_b);
}
