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

#include <cstring>
#include <vector>
#include <cstdlib>
#include <glib/gstdio.h>
#include <glib.h>
#include <algorithm>
#include <memory>

#ifdef _WIN32
#  include <windows.h>
#else
#  include <unistd.h>
#endif

#include "lib_res_store.h"
#include "libcommon.h"
#include "ifo_file.h"
#include "lib_dict_data_block.h"
#include "lib_dict_verify.h"
#include "lib_chars.h"


size_t data_field_t::get_size(void) const
{
	if(g_ascii_islower(type_id))
		return data.size() - 1;
	else
		return data.size();
}

const char* data_field_t::get_data(void) const
{
	if(data.empty())
		return NULL;
	else
		return &data[0];
}

void data_field_t::set_data(const char* p, size_t size, bool add_null)
{
	if(add_null)
		data.reserve(size+1);
	data.assign(p, p+size);
	if(add_null)
		data.push_back('\0');
}


/* must load at least 1 field, otherwise - error. */
VerifResult dictionary_data_block::load(const char* const data, size_t data_size,
	const std::string& sametypesequence, const char* word,
	data_field_vect_t* fields)
{
	this->fields = fields;
	this->word = word;
	if(fields)
		fields->clear();
	if(data_size == 0) {
		g_warning(empty_block_err, word);
		return VERIF_RESULT_FATAL;
	}
	field_num = 0;
	VerifResult result = VERIF_RESULT_OK;
	if (!sametypesequence.empty()) {
		result = combine_result(result, load_sametypesequence(data, data_size, sametypesequence));
	} else {
		result = combine_result(result, load_no_sametypesequence(data, data_size));
	}
	if(VERIF_RESULT_FATAL <= result) {
		if(fields)
			fields->clear();
		return result;
	}
	if(field_num == 0) {
		g_warning(data_block_no_fields_err, word);
		return VERIF_RESULT_FATAL;
	}
	return result;
}

VerifResult dictionary_data_block::load_sametypesequence(const char* const data, size_t data_size,
	const std::string& sametypesequence)
{
	const char* p = data;
	size_t size_remain; // to the end of the data block
	VerifResult result = VERIF_RESULT_OK;
	for (size_t i=0; i<sametypesequence.length()-1; i++) {
		g_assert(static_cast<size_t>(p-data) <= data_size);
		size_remain = data_size - (p - data); // 0 is OK
		const char type_id = sametypesequence[i];
		ext_result_t ext_result(load_field(type_id, p, size_remain));
		if(FIELD_VERIF_RES_ABORT <= ext_result.field || VERIF_RESULT_FATAL <= ext_result.content) {
			g_critical(fields_extraction_faild_err, word);
			return VERIF_RESULT_CRITICAL;
		}
		result = combine_result(result, ext_result.content);
	}
	// last item
	g_assert(static_cast<size_t>(p-data) <= data_size);
	size_remain = data_size - (p - data);
	const char type_id = sametypesequence[sametypesequence.length()-1];
	ext_result_t ext_result;
	if(g_ascii_isupper(type_id)) {
		ext_result = load_field_sametypesequence_last_upper(type_id, p, size_remain);
	} else if(g_ascii_islower(type_id)) {
		ext_result = load_field_sametypesequence_last_lower(type_id, p, size_remain);
	} else {
		g_warning(unknown_type_id_err, word, type_id);
		result = combine_result(result, VERIF_RESULT_WARNING);
		if(fix_errors) {
			g_message(fixed_ignore_field_msg);
		}
		g_warning(fields_extraction_faild_err, word);
		return result;
	}
	if(FIELD_VERIF_RES_ABORT <= ext_result.field || VERIF_RESULT_FATAL <= ext_result.content) {
		g_critical(fields_extraction_faild_err, word);
		return VERIF_RESULT_CRITICAL;
	} else
		result = combine_result(result, ext_result.content);
	if(!strchr(known_type_ids, type_id)) {
		g_warning(unknown_type_id_err, word, type_id);
		result = combine_result(result, VERIF_RESULT_WARNING);
		if(fix_errors) {
			g_message(fixed_accept_unknown_field_msg);
		}
	}
	g_assert(static_cast<size_t>(p-data) <= data_size);
	size_remain = data_size - (p - data);
	if(size_remain > 0) {
		g_warning(incorrect_data_block_size_err, word);
		result = combine_result(result, VERIF_RESULT_WARNING);
	}
	return result;
}

VerifResult dictionary_data_block::load_no_sametypesequence(const char* const data, size_t data_size)
{
	const char* p = data;
	size_t size_remain; // to the end of the data block
	VerifResult result = VERIF_RESULT_OK;
	while(true) {
		size_remain = data_size - (p - data);
		if(size_remain == 0)
			return result;
		const char type_id = *p;
		++p;
		--size_remain;
		ext_result_t ext_result(load_field(type_id, p, size_remain));
		if(FIELD_VERIF_RES_ABORT <= ext_result.field || VERIF_RESULT_FATAL <= ext_result.content) {
			g_critical(fields_extraction_faild_err, word);
			return VERIF_RESULT_CRITICAL;
		}
		result = combine_result(result, ext_result.content);
	}
	g_assert_not_reached();
	return VERIF_RESULT_OK;
}

ext_result_t dictionary_data_block::load_field(const char type_id,
	const char*& p, const size_t size_remain)
{
	ext_result_t ext_result;
	if(size_remain == 0) {
		g_warning(empty_field_err, word, type_id);
		ext_result.append(VERIF_RESULT_WARNING);
		ext_result.append(FIELD_VERIF_RES_SKIP);
		if(fix_errors) {
			g_message(fixed_ignore_field_msg);
			return ext_result;
		} else
			return ext_result;
	}
	if(g_ascii_isupper(type_id)) {
		ext_result.append(load_field_upper(type_id, p, size_remain));
	} else if(g_ascii_islower(type_id)) {
		ext_result.append(load_field_lower(type_id, p, size_remain));
	} else {
		g_warning(unknown_type_id_err, word, type_id);
		ext_result.append(VERIF_RESULT_WARNING);
		ext_result.append(FIELD_VERIF_RES_ABORT);
		p += size_remain;
		if(fix_errors) {
			g_message(fixed_ignore_field_msg);
			return ext_result;
		} else
			return ext_result;
	}
	if(!strchr(known_type_ids, type_id)) {
		g_warning(unknown_type_id_err, word, type_id);
		ext_result.append(VERIF_RESULT_WARNING);
		if(fix_errors) {
			g_message(fixed_accept_unknown_field_msg);
		}
	}
	return ext_result;
}

ext_result_t dictionary_data_block::load_field_upper(const char type_id,
	const char*& p, const size_t size_remain)
{
	ext_result_t ext_result;
	if(size_remain < sizeof(guint32)) {
		ext_result.append(VERIF_RESULT_CRITICAL);
		ext_result.append(FIELD_VERIF_RES_ABORT);
		g_warning(incorrect_data_block_size_err, word);
		p += size_remain;
		if(fix_errors) {
			g_message(fixed_ignore_field_msg);
			return ext_result;
		} else
			return ext_result;
	}
	guint32 size = g_ntohl(*reinterpret_cast<const guint32 *>(p));
	if(size_remain < sizeof(guint32) + size) {
		g_warning(incorrect_data_block_size_err, word);
		ext_result.append(VERIF_RESULT_CRITICAL);
		if(fix_errors) {
			size = size_remain - sizeof(guint32);
			g_message(fixed_change_field_size_msg);
		} else {
			p += size_remain;
			ext_result.append(FIELD_VERIF_RES_ABORT);
			return ext_result;
		}
	}
	p += sizeof(guint32);
	if(size == 0) {
		g_warning(empty_field_err, word, type_id);
		ext_result.append(VERIF_RESULT_WARNING);
		if(fix_errors) {
			g_message(fixed_ignore_field_msg);
			ext_result.append(FIELD_VERIF_RES_SKIP);
			return ext_result;
		} else {
			add_field(type_id, NULL, 0);
			return ext_result;
		}
	}
	const char* data = p;
	p += size;
	VerifResult result = verify_field_content(type_id, data, size);
	if(VERIF_RESULT_FATAL <= result) {
		ext_result.append(VERIF_RESULT_CRITICAL);
		ext_result.append(FIELD_VERIF_RES_SKIP);
		std::string temp(data, size);
		g_warning(invalid_field_content_err, word, type_id, temp.c_str());
		if(fix_errors) {
			g_message(fixed_ignore_field_msg);
			return ext_result;
		} else
			return ext_result;
	} else
		ext_result.append(result);
	add_field(type_id, data, size);
	return ext_result;
}

ext_result_t dictionary_data_block::load_field_lower(const char type_id,
	const char*& p, const size_t size_remain)
{
	ext_result_t ext_result;
	if(size_remain < 1) { // data must contain at least '\0'
		g_warning(incorrect_data_block_size_err, word);
		ext_result.append(VERIF_RESULT_CRITICAL);
		ext_result.append(FIELD_VERIF_RES_SKIP);
		p += size_remain;
		if(fix_errors) {
			g_message(fixed_ignore_field_msg);
			return ext_result;
		} else
			return ext_result;
	}
	const char* field_end = reinterpret_cast<const char*>(memchr(p, '\0', size_remain));
	if(!field_end) {
		g_warning(incorrect_data_block_size_err, word);
		ext_result.append(VERIF_RESULT_CRITICAL);
		ext_result.append(FIELD_VERIF_RES_ABORT);
		if(fix_errors) {
			g_message(fixed_field_take_longest_str_msg);
			field_end = p + size_remain;
		} else {
			p += size_remain;
			return ext_result;
		}
	}
	/* In case we need to apply changes to data, we'll store modified copy here. */
	std::string data_str;
	const char* data = p;
	int datalen = field_end - p;
	p += std::min<size_t>(datalen + 1, size_remain); // shift the pointer to the next field
	if(datalen == 0) {
		g_warning(empty_field_err, word, type_id);
		ext_result.append(VERIF_RESULT_WARNING);
		if(fix_errors) {
			g_message(fixed_ignore_field_msg);
			ext_result.append(FIELD_VERIF_RES_SKIP);
			return ext_result;
		} else {
			add_field(type_id, NULL, 0, true);
			return ext_result;
		}
	}
	if (!g_utf8_validate(data, datalen, NULL)) {
		g_warning(invalid_utf8_field_err, word, type_id, data);
		ext_result.append(VERIF_RESULT_CRITICAL);
		if(fix_errors) {
			data_str = fix_utf8_str(std::string(data, datalen), 0);
			data = data_str.c_str();
			datalen = data_str.length();
			g_message(fixed_utf8_drop_invalid_char_msg);
			if(datalen == 0) {
				g_warning(empty_field_err, word, type_id);
				ext_result.append(VERIF_RESULT_WARNING);
				g_message(fixed_ignore_field_msg);
				ext_result.append(FIELD_VERIF_RES_SKIP);
				return ext_result;
			}
		} else {
			ext_result.append(FIELD_VERIF_RES_SKIP);
			return ext_result;
		}
	}
	{	// check for invalid chars
		typedef std::list<const char*> str_list_t;
		str_list_t invalid_chars;
		if(check_xml_string_chars(data, datalen, invalid_chars)) {
			std::string temp(data, datalen);
			g_message(invalid_field_content_chars_err, word, type_id, temp.c_str(),
				print_char_codes(invalid_chars).c_str());
			ext_result.append(VERIF_RESULT_WARNING);
			if(fix_errors) {
				fix_xml_string_chars(data, datalen, data_str);
				data = data_str.c_str();
				datalen = data_str.length();
				g_message(fixed_drop_invalid_char_msg);
				if(datalen == 0) {
					g_warning(empty_field_err, word, type_id);
					ext_result.append(VERIF_RESULT_WARNING);
					g_message(fixed_ignore_field_msg);
					ext_result.append(FIELD_VERIF_RES_SKIP);
					return ext_result;
				}
			}
		}
	}
	VerifResult result = verify_field_content(type_id, data, datalen);
	if(VERIF_RESULT_FATAL <= result) {
		ext_result.append(VERIF_RESULT_CRITICAL);
		ext_result.append(FIELD_VERIF_RES_SKIP);
		std::string temp(data, datalen);
		g_warning(invalid_field_content_err, word, type_id, temp.c_str());
		if(fix_errors) {
			g_message(fixed_ignore_field_msg);
			return ext_result;
		} else
			return ext_result;
	} else
		ext_result.append(result);
	add_field(type_id, data, datalen, true);
	return ext_result;
}

ext_result_t dictionary_data_block::load_field_sametypesequence_last_upper(const char type_id,
	const char*& p, const size_t size_remain)
{
	guint32 size = size_remain;
	ext_result_t ext_result;
	if(size == 0) {
		g_warning(empty_field_err, word, type_id);
		ext_result.append(VERIF_RESULT_WARNING);
		if(fix_errors) {
			g_message(fixed_ignore_field_msg);
			ext_result.append(FIELD_VERIF_RES_SKIP);
			return ext_result;
		} else {
			add_field(type_id, NULL, 0);
			return ext_result;
		}
	}
	const char* data = p;
	p += size;
	VerifResult result = verify_field_content(type_id, data, size);
	if(VERIF_RESULT_FATAL <= result) {
		ext_result.append(VERIF_RESULT_CRITICAL);
		ext_result.append(FIELD_VERIF_RES_SKIP);
		std::string temp(data, size);
		g_warning(invalid_field_content_err, word, type_id, temp.c_str());
		if(fix_errors) {
			g_message(fixed_ignore_field_msg);
			return ext_result;
		} else
			return ext_result;
	} else
		ext_result.append(result);
	add_field(type_id, data, size);
	return ext_result;
}

ext_result_t dictionary_data_block::load_field_sametypesequence_last_lower(const char type_id,
	const char*& p, const size_t size_remain)
{
	size_t datalen = size_remain;
	ext_result_t ext_result;
	if(datalen == 0) {
		g_warning(empty_field_err, word, type_id);
		ext_result.append(VERIF_RESULT_WARNING);
		if(fix_errors) {
			g_message(fixed_ignore_field_msg);
			ext_result.append(FIELD_VERIF_RES_SKIP);
			return ext_result;
		} else {
			add_field(type_id, NULL, 0, true);
			return ext_result;
		}
	}
	/* In case we need to apply changes to data, we'll store the modified copy here. */
	std::string data_str;
	const char* data = p;
	p += size_remain; // shift the pointer to the next field
	const char* p2 = reinterpret_cast<const char*>(memchr(data, '\0', datalen));
	if(p2) {
		// '\0' found in the last record
		g_warning(incorrect_data_block_size_err, word);
		ext_result.append(VERIF_RESULT_WARNING);
		if(fix_errors) {
			datalen = p2 - data;
			if(datalen == 0) {
				g_message(fixed_ignore_field_msg);
				ext_result.append(FIELD_VERIF_RES_SKIP);
				return ext_result;
			}
			g_message(fixed_field_take_zero_term_str_msg);
		}
	}
	if (!g_utf8_validate(data, datalen, NULL)) {
		std::string tmp(data, datalen);
		g_warning(invalid_utf8_field_err, word, type_id, tmp.c_str());
		ext_result.append(VERIF_RESULT_CRITICAL);
		if(fix_errors) {
			data_str = fix_utf8_str(std::string(data, datalen), 0);
			data = data_str.c_str();
			datalen = data_str.length();
			g_message(fixed_utf8_drop_invalid_char_msg);
			if(datalen == 0) {
				g_warning(empty_field_err, word, type_id);
				ext_result.append(VERIF_RESULT_WARNING);
				g_message(fixed_ignore_field_msg);
				ext_result.append(FIELD_VERIF_RES_SKIP);
				return ext_result;
			}
		} else {
			ext_result.append(FIELD_VERIF_RES_SKIP);
			return ext_result;
		}
	}
	{	// check for invalid chars
		typedef std::list<const char*> str_list_t;
		str_list_t invalid_chars;
		if(check_xml_string_chars(data, datalen, invalid_chars)) {
			std::string temp(data, datalen);
			g_message(invalid_field_content_chars_err, word, type_id, temp.c_str(),
				print_char_codes(invalid_chars).c_str());
			ext_result.append(VERIF_RESULT_WARNING);
			if(fix_errors) {
				fix_xml_string_chars(data, datalen, data_str);
				data = data_str.c_str();
				datalen = data_str.length();
				g_message(fixed_drop_invalid_char_msg);
				if(datalen == 0) {
					g_warning(empty_field_err, word, type_id);
					ext_result.append(VERIF_RESULT_WARNING);
					g_message(fixed_ignore_field_msg);
					ext_result.append(FIELD_VERIF_RES_SKIP);
					return ext_result;
				}
			}
		}
	}
	VerifResult result = verify_field_content(type_id, data, datalen);
	if(VERIF_RESULT_FATAL <= result) {
		ext_result.append(VERIF_RESULT_CRITICAL);
		ext_result.append(FIELD_VERIF_RES_SKIP);
		std::string temp(data, datalen);
		g_warning(invalid_field_content_err, word, type_id, temp.c_str());
		if(fix_errors) {
			g_message(fixed_ignore_field_msg);
			return ext_result;
		} else
			return ext_result;
	} else
		ext_result.append(result);
	add_field(type_id, data, datalen, true);
	return ext_result;
}

/* any fatal error may be solved by ignoring this field
 * So VERIF_RESULT_FATAL is counted as VERIF_RESULT_CRITICAL by caller function. */
VerifResult dictionary_data_block::verify_field_content(const char type_id, const char* data, guint32 size)
{
	if(type_id == 'x')
		return verify_field_content_x(data, size);
	if(type_id == 'r')
		return verify_field_content_r(data, size);
	return VERIF_RESULT_OK;
}

VerifResult dictionary_data_block::verify_field_content_x(const char* data, guint32 size)
{
	const char type_id = 'x';
	// create a '\0'-terminated string
	std::string temp(data, size);
	std::string key;
	const char* p;
	const char* tag;
	VerifResult result = VERIF_RESULT_OK;
	for(p = temp.c_str(); p && *p && (tag = strstr(p, "<rref")); ) {
		p = tag + sizeof("<rref")-1;
		if(*p == '>')
			++p;
		else if (*p == ' ') {
			p = strchr(p, '>');
			if(!p)
				break;
			++p;
		} else { // error
			p = strchr(p, '>');
			if(!p)
				break;
			++p;
			continue;
		}
		// p points after the "<rref ...>"
		tag = strstr(p, "</rref>");
		if(!tag)
			break;
		key.assign(p, tag - p);
		if(p_res_storage && !p_res_storage->have_file(key)) {
			g_warning(resource_not_found_msg,
				word, type_id, key.c_str());
			result = combine_result(result, VERIF_RESULT_NOTE);
			if(fix_errors) {
				g_message(fixed_ignore_msg);
			}
		}
		p = tag + sizeof("</rref>") - 1;
	}
	return result;
}

VerifResult dictionary_data_block::verify_field_content_r(const char* const data, guint32 size,
	resitem_vect_t *items)
{
	const char type_id = 'r';
	const char* line_beg = data;
	const char* line_end;
	resitem_t resitem;
	VerifResult result = VERIF_RESULT_OK;
	size_t item_num = 0; // number of successfully extracted items

	if(items)
		items->clear();
	while(true) {
		const gint size_remain = static_cast<gint>(size) - (line_beg - data);
		if(size_remain <= 0)
			break;
		line_end = (const char*)memchr(line_beg, '\n', size_remain);
		if(!line_end)
			line_end = data + size;
		if(line_beg == line_end) {
			g_warning(resource_invalid_format_empty_line_msg,
				word, type_id);
			result = combine_result(result, VERIF_RESULT_NOTE);
			if(fix_errors) {
				g_message(fixed_ignore_resource_line_msg);
				++line_beg;
				continue;
			} else {
				continue;
			}
		}
		const std::string line(line_beg, line_end - line_beg);
		const char* colon = (const char*)memchr(line_beg, ':', line_end - line_beg);
		if(!colon) {
			g_warning(resource_invalid_format_colon_msg,
				word, type_id, line.c_str());
			result = combine_result(result, VERIF_RESULT_WARNING);
			if(fix_errors) {
				g_message(fixed_ignore_resource_line_msg);
				line_beg = line_end + 1;
				continue;
			} else {
				continue;
			}
		}
		resitem.type.assign(line_beg, colon - line_beg);
		++colon;
		resitem.key.assign(colon, line_end - colon);
		line_beg = line_end + 1;
		if(resitem.type.empty()) {
			g_warning(resource_invalid_format_type_blank_msg,
				word, type_id, line.c_str());
			result = combine_result(result, VERIF_RESULT_WARNING);
			if(fix_errors) {
				g_message(fixed_ignore_resource_line_msg);
				continue;
			} else {
				continue;
			}
		}
		if(resitem.key.empty()) {
			g_warning(resource_invalid_format_key_blank_msg,
				word, type_id, line.c_str());
			result = combine_result(result, VERIF_RESULT_WARNING);
			if(fix_errors) {
				g_message(fixed_ignore_resource_line_msg);
				continue;
			} else {
				continue;
			}
		}
		if(!is_known_resource_type(resitem.type.c_str())) {
			g_warning(resource_invalid_format_unknown_type_msg,
				word, type_id, line.c_str());
			result = combine_result(result, VERIF_RESULT_WARNING);
			if(fix_errors) {
				g_message(fixed_ignore_resource_line_msg);
				continue;
			} else {
				continue;
			}
		}
		if(resitem.key.find('\\') != std::string::npos) {
			g_warning(resource_invalid_format_back_spash_msg,
				word, type_id, line.c_str());
			result = combine_result(result, VERIF_RESULT_WARNING);
			if(fix_errors) {
				g_message(fixed_ignore_resource_line_msg);
				continue;
			} else {
				continue;
			}
		}
		if(p_res_storage && !p_res_storage->have_file(resitem.key)) {
			g_warning(resource_resource_nof_found_msg,
				word, type_id, line.c_str(), resitem.key.c_str());
			result = combine_result(result, VERIF_RESULT_NOTE);
			if(fix_errors) {
				g_message(fixed_ignore_resource_line_msg);
				continue;
			}
		}
		if(items)
			items->push_back(resitem);
		++item_num;
	}
	if(item_num == 0) {
		g_warning(resource_empty_list_msg,
			word, type_id);
		result = combine_result(result, VERIF_RESULT_WARNING);
	}
	return result;
}

void dictionary_data_block::add_field(char type_id, const char* data, size_t datalen, bool add_null)
{
	++field_num;
	if(fields) {
		data_field_t field;
		field.type_id = type_id;
		field.set_data(data, datalen, add_null);
		fields->push_back(field);
	}
}
