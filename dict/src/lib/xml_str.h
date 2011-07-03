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

#ifndef XML_STR_H_
#define XML_STR_H_

#include <string>

/* xml strings in utf8 encoding 
 * calculate string lengths, next, previous characters, etc ignoring xml tags 
 * */

size_t xml_utf8_strlen(const char *str);
void xml_utf8_decode(const char *str, std::string& decoded);
size_t xml_utf8_get_index_at_offset(const std::string& str, size_t char_offset);
const char* xml_utf8_offset_to_pointer(const char *str, size_t char_offset);
const char* xml_utf8_end_of_char(const char *str);

/* A structure for easily access character data of xml string. */
struct XMLCharData {
	XMLCharData(void)
	:
	byte_inds(NULL),
	char_data_str(NULL),
	size(0),
	xml_str(NULL)
	{
	}
	~XMLCharData(void)
	{
		clear();
	}
	void assign_xml(const char *xml_str_);
	/* mark a substring with specified markup
	 * We enclose every continuous peace (not interrupted with markup) of 
	 * the specified substring in start_tag - end_tag tags. That guarantees 
	 * proper tag nesting.
	 * 
	 * Parameters:
	 * out - write output here (append)
	 * start_tag, end_tag - tags for marking the substring
	 * cd_begin_ind - index of the first char of the substring in char_data_str
	 * cd_len - substring length in bytes */
	void mark_substring(std::string& out, const char* start_tag,
		const char* end_tag, size_t cd_begin_ind, size_t cd_len) const;
	/* copy xml between specified chars 
	 * 
	 * Parameters:
	 * out - write output here (append)
	 * cd_begin_ind, cd_end_ind - specifies a range of bytes to copy 
	 * We copy all xml data meeting the following conditions:
	 * 1) following the char including byte number cd_begin_ind-1 in 
	 * the char_data_str string (if cd_begin_ind = 0, we start from the beginning
	 * of the string),
	 * 2) preceding the char including byte number cd_end_ind in
	 * the char_data_str string. 
	 * must be: 0 <= cd_begin_ind <= cd_end_ind <= strlen(char_data_str) */
	void copy_xml(std::string& out, size_t cd_begin_ind, size_t cd_end_ind) const;
	const char* get_char_data_str(void) const
	{
		return char_data_str;
	}
	size_t get_char_data_str_length(void) const
	{
		return size ? size - 1 : 0;
	}
private:
	void clear(void);
	void allocate(const char *xml_str_);

	/* byte indexes. byte_inds[ind] - byte-index of the first byte of the char in 
	 * the xml string corresponding to the char at byte-index ind in 
	 * the char_data_str string. */
	size_t *byte_inds;
	/* character data string extracted from xml. All character and entity 
	 * references are resolved. In utf-8, 0-terminated. */
	char* char_data_str;
	/* = strlen(char_data_str) + 1 = char data string length in bytes including 
	 * the terminating '\0'. */
	size_t size;
	char* xml_str;
};

#endif /* XML_STR_H_ */
