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
#include "resourcewrap.hpp"
#include "libcommon.h"
#include "ifo_file.hpp"
#include "lib_dict_data_block.h"
#include "libstardictverify.h"
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
int dictionary_data_block::load(const char* const data, size_t data_size,
	const std::string& sametypesequence, const char* word,
	data_field_vect_t* fields)
{
	this->fields = fields;
	this->word = word;
	if(fields)
		fields->clear();
	if(data_size == 0) {
		print_info("Index item %s. data size = 0.\n", word);
		return EXIT_FAILURE;
	}
	field_num = 0;
	int res;
	if (!sametypesequence.empty()) {
		res = load_sametypesequence(data, data_size, sametypesequence);
	} else {
		res = load_no_sametypesequence(data, data_size);
	}
	if(res == EXIT_FAILURE) {
		if(fields)
			fields->clear();
		return EXIT_FAILURE;
	}
	if(field_num == 0) {
		print_info("Index item %s. no fields were extracted.\n", word);
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}

int dictionary_data_block::load_sametypesequence(const char* const data, size_t data_size,
	const std::string& sametypesequence)
{
	const char* p = data;
	size_t size_remain; // to the end of the data block
	ext_result_t res(EXIT_SUCCESS, EXIT_SUCCESS);
	for (size_t i=0; i<sametypesequence.length()-1; i++) {
		g_assert(static_cast<size_t>(p-data) <= data_size);
		size_remain = data_size - (p - data); // 0 is OK
		const char type_id = sametypesequence[i];
		res.append(load_field(type_id, p, size_remain));
		if(res.extract)
			return EXIT_FAILURE;
	}
	// last item
	g_assert(static_cast<size_t>(p-data) <= data_size);
	size_remain = data_size - (p - data);
	const char type_id = sametypesequence[sametypesequence.length()-1];
	if(g_ascii_isupper(type_id)) {
		res.append(load_field_sametypesequence_last_upper(type_id, p, size_remain));
	} else if(g_ascii_islower(type_id)) {
		res.append(load_field_sametypesequence_last_lower(type_id, p, size_remain));
	} else {
		print_info(unknown_type_id_err, word, type_id);
		if(fix_errors) {
			print_info(fixed_ignore_field_msg);
			res.append(ext_result_t(EXIT_SUCCESS, EXIT_SUCCESS));
		} else
			res.append(ext_result_t(EXIT_SUCCESS, EXIT_FAILURE));
		return res.summary();
	}
	if(!strchr(known_type_ids, type_id)) {
		print_info(unknown_type_id_err, word, type_id);
		if(fix_errors) {
			print_info(fixed_accept_unknown_field_msg);
			res.append(ext_result_t(EXIT_SUCCESS, EXIT_SUCCESS));
		} else
			res.append(ext_result_t(EXIT_SUCCESS, EXIT_FAILURE));
	}
	return res.summary();
}

int dictionary_data_block::load_no_sametypesequence(const char* const data, size_t data_size)
{
	const char* p = data;
	size_t size_remain; // to the end of the data block
	ext_result_t res(EXIT_SUCCESS, EXIT_SUCCESS);
	while(true) {
		size_remain = data_size - (p - data);
		if(size_remain == 0)
			return res.summary();
		const char type_id = *p;
		++p;
		--size_remain;
		res.append(load_field(type_id, p, size_remain));
		if(res.extract)
			return EXIT_FAILURE;
	}
	g_assert_not_reached();
	return EXIT_SUCCESS;
}

ext_result_t dictionary_data_block::load_field(const char type_id,
	const char*& p, const size_t size_remain)
{
	if(size_remain == 0) {
		print_info(empty_field_err, word, type_id);
		if(fix_errors) {
			print_info(fixed_ignore_field_msg);
			return ext_result_t(EXIT_SUCCESS, EXIT_SUCCESS);
		} else
			return ext_result_t(EXIT_SUCCESS, EXIT_FAILURE);
	}
	ext_result_t res(EXIT_SUCCESS, EXIT_SUCCESS);
	if(g_ascii_isupper(type_id)) {
		res.append(load_field_upper(type_id, p, size_remain));
	} else if(g_ascii_islower(type_id)) {
		res.append(load_field_lower(type_id, p, size_remain));
	} else {
		print_info(unknown_type_id_err, word, type_id);
		if(fix_errors) {
			p += size_remain;
			print_info(fixed_ignore_field_msg);
			return ext_result_t(EXIT_SUCCESS, EXIT_SUCCESS);
		} else
			return ext_result_t(EXIT_SUCCESS, EXIT_FAILURE);
	}
	if(!strchr(known_type_ids, type_id)) {
		print_info(unknown_type_id_err, word, type_id);
		if(fix_errors) {
			print_info(fixed_accept_unknown_field_msg);
			res.append(ext_result_t(EXIT_SUCCESS, EXIT_SUCCESS));
		} else
			res.append(ext_result_t(EXIT_SUCCESS, EXIT_FAILURE));
	}
	return res;
}

ext_result_t dictionary_data_block::load_field_upper(const char type_id,
	const char*& p, const size_t size_remain)
{
	if(size_remain < sizeof(guint32)) {
		print_info(incorrect_data_block_size_err, word);
		p += size_remain;
		if(fix_errors) {
			print_info(fixed_ignore_field_msg);
			return ext_result_t(EXIT_SUCCESS, EXIT_SUCCESS);
		} else
			return ext_result_t(EXIT_SUCCESS, EXIT_FAILURE);
	}
	guint32 size = g_ntohl(*reinterpret_cast<const guint32 *>(p));
	if(size_remain < sizeof(guint32) + size) {
		print_info(incorrect_data_block_size_err, word);
		if(fix_errors) {
			size = size_remain - sizeof(guint32);
			print_info("fixed. change field size.\n");
		} else {
			p += size_remain;
			return ext_result_t(EXIT_SUCCESS, EXIT_FAILURE);
		}
	}
	p += sizeof(guint32);
	if(size == 0) {
		print_info(empty_field_err, word, type_id);
		if(fix_errors) {
			print_info(fixed_ignore_field_msg);
			return ext_result_t(EXIT_SUCCESS, EXIT_SUCCESS);
		} else
			return ext_result_t(EXIT_SUCCESS, EXIT_FAILURE);
	}
	if(verify_field_content(type_id, p, size)) {
		std::string temp(p, size);
		print_info(invalid_field_content_err, word, type_id, temp.c_str());
		p += size;
		if(fix_errors) {
			print_info(fixed_ignore_field_msg);
			return ext_result_t(EXIT_SUCCESS, EXIT_SUCCESS);
		} else
			return ext_result_t(EXIT_SUCCESS, EXIT_FAILURE);
	}
	++field_num;
	if(fields) {
		data_field_t field;
		field.type_id = type_id;
		field.set_data(p, size);
		fields->push_back(field);
	}
	p += size;
	return ext_result_t(EXIT_SUCCESS, EXIT_SUCCESS);
}

ext_result_t dictionary_data_block::load_field_lower(const char type_id,
	const char*& p, const size_t size_remain)
{
	if(size_remain < 1) { // data must contain at least '\0'
		print_info(incorrect_data_block_size_err, word);
		p += size_remain;
		if(fix_errors) {
			print_info(fixed_ignore_field_msg);
			return ext_result_t(EXIT_SUCCESS, EXIT_SUCCESS);
		} else
			return ext_result_t(EXIT_SUCCESS, EXIT_FAILURE);
	}
	const char* field_end = reinterpret_cast<const char*>(memchr(p, '\0', size_remain));
	if(!field_end) {
		print_info(incorrect_data_block_size_err, word);
		if(fix_errors) {
			print_info("fixed. take the longest string.\n");
			field_end = p + size_remain;
		} else {
			p += size_remain;
			return ext_result_t(EXIT_SUCCESS, EXIT_FAILURE);
		}
	}
	/* In case we need to apply changes to data, we'll store modified copy here. */
	std::string data_str;
	const char* data = p;
	int datalen = field_end - p;
	p += std::min<size_t>(datalen + 1, size_remain); // shift the pointer to the next field
	if(datalen == 0) {
		print_info(empty_field_err, word, type_id);
		if(fix_errors) {
			print_info(fixed_ignore_field_msg);
			return ext_result_t(EXIT_SUCCESS, EXIT_SUCCESS);
		} else
			return ext_result_t(EXIT_SUCCESS, EXIT_FAILURE);
	}
	if (!g_utf8_validate(data, datalen, NULL)) {
		print_info(invalid_utf8_field_err, word, type_id, data);
		if(fix_errors) {
			print_info(fixed_ignore_field_msg);
			return ext_result_t(EXIT_SUCCESS, EXIT_SUCCESS);
		} else
			return ext_result_t(EXIT_SUCCESS, EXIT_FAILURE);
	}
	{	// check for invalid chars
		typedef std::list<const char*> str_list_t;
		str_list_t invalid_chars;
		if(check_xml_string_chars(data, datalen, invalid_chars)) {
			std::string temp(data, datalen);
			print_info(invalid_field_content_err, word, type_id, temp.c_str());
			for(str_list_t::const_iterator it = invalid_chars.begin(); it != invalid_chars.end(); ++it) {
				print_info(invalid_char_value_err, g_utf8_get_char(*it));
			}
			if(fix_errors) {
				fix_xml_string_chars(data, datalen, data_str);
				data = data_str.c_str();
				datalen = data_str.length();
				print_info(fixed_drop_invalid_char_msg);
			}
		}
	}
	if(verify_field_content(type_id, data, datalen)) {
		std::string temp(data, datalen);
		print_info(invalid_field_content_err, word, type_id, temp.c_str());
		if(fix_errors) {
			print_info(fixed_ignore_field_msg);
			return ext_result_t(EXIT_SUCCESS, EXIT_SUCCESS);
		} else
			return ext_result_t(EXIT_SUCCESS, EXIT_FAILURE);
	}
	++field_num;
	if(fields) {
		data_field_t field;
		field.type_id = type_id;
		field.set_data(data, datalen, true);
		fields->push_back(field);
	}
	return ext_result_t(EXIT_SUCCESS, EXIT_SUCCESS);
}

ext_result_t dictionary_data_block::load_field_sametypesequence_last_upper(const char type_id,
	const char*& p, const size_t size_remain)
{
	guint32 size = size_remain;
	if(size == 0) {
		if(fix_errors) {
			print_info(fixed_ignore_field_msg);
			return ext_result_t(EXIT_SUCCESS, EXIT_SUCCESS);
		} else
			return ext_result_t(EXIT_SUCCESS, EXIT_FAILURE);
	}
	if(verify_field_content(type_id, p, size)) {
		std::string temp(p, size);
		print_info(invalid_field_content_err, word, type_id, temp.c_str());
		p += size;
		if(fix_errors) {
			print_info(fixed_ignore_field_msg);
			return ext_result_t(EXIT_SUCCESS, EXIT_SUCCESS);
		} else
			return ext_result_t(EXIT_SUCCESS, EXIT_FAILURE);
	}
	++field_num;
	if(fields) {
		data_field_t field;
		field.type_id = type_id;
		field.set_data(p, size);
		fields->push_back(field);
	}
	p += size;
	return ext_result_t(EXIT_SUCCESS, EXIT_SUCCESS);
}

ext_result_t dictionary_data_block::load_field_sametypesequence_last_lower(const char type_id,
	const char*& p, const size_t size_remain)
{
	size_t datalen = size_remain;
	if(datalen == 0) {
		print_info(empty_field_err, word, type_id);
		if(fix_errors) {
			print_info(fixed_ignore_field_msg);
			return ext_result_t(EXIT_SUCCESS, EXIT_SUCCESS);
		} else
			return ext_result_t(EXIT_SUCCESS, EXIT_FAILURE);
	}
	/* In case we need to apply changes to data, we'll store modified copy here. */
	std::string data_str;
	const char* data = p;
	p += size_remain; // shift the pointer to the next field
	const char* p2 = reinterpret_cast<const char*>(memchr(data, '\0', datalen));
	if(p2) {
		// '\0' found in the last record
		print_info(incorrect_data_block_size_err, word);
		if(fix_errors) {
			datalen = p2 - data;
			if(datalen == 0) {
				print_info(fixed_ignore_field_msg);
				return ext_result_t(EXIT_SUCCESS, EXIT_SUCCESS);
			}
			print_info("fixed. take a zero-terminated string.\n");
		} else
			return ext_result_t(EXIT_SUCCESS, EXIT_FAILURE);
	}
	if (!g_utf8_validate(data, datalen, NULL)) {
		std::string tmp(data, datalen);
		print_info(invalid_utf8_field_err, word, type_id, tmp.c_str());
		if(fix_errors) {
			print_info(fixed_ignore_field_msg);
			return ext_result_t(EXIT_SUCCESS, EXIT_SUCCESS);
		} else
			return ext_result_t(EXIT_SUCCESS, EXIT_FAILURE);
	}
	{	// check for invalid chars
		typedef std::list<const char*> str_list_t;
		str_list_t invalid_chars;
		if(check_xml_string_chars(data, datalen, invalid_chars)) {
			std::string temp(data, datalen);
			print_info(invalid_field_content_err, word, type_id, temp.c_str());
			for(str_list_t::const_iterator it = invalid_chars.begin(); it != invalid_chars.end(); ++it) {
				print_info(invalid_char_value_err, g_utf8_get_char(*it));
			}
			if(fix_errors) {
				fix_xml_string_chars(data, datalen, data_str);
				data = data_str.c_str();
				datalen = data_str.length();
				print_info(fixed_drop_invalid_char_msg);
			}
		}
	}
	if(verify_field_content(type_id, data, datalen)) {
		std::string temp(data, datalen);
		print_info(invalid_field_content_err, word, type_id, temp.c_str());
		if(fix_errors) {
			print_info(fixed_ignore_field_msg);
			return ext_result_t(EXIT_SUCCESS, EXIT_SUCCESS);
		} else
			return ext_result_t(EXIT_SUCCESS, EXIT_FAILURE);
	}
	++field_num;
	if(fields) {
		data_field_t field;
		field.type_id = type_id;
		field.set_data(data, datalen, true);
		fields->push_back(field);
	}
	return ext_result_t(EXIT_SUCCESS, EXIT_SUCCESS);
}

int dictionary_data_block::verify_field_content(const char type_id, const char* data, guint32 size)
{
	if(type_id == 'x')
		return verify_field_content_x(data, size);
	if(type_id == 'r')
		return verify_field_content_r(data, size);
	return EXIT_SUCCESS;
}

int dictionary_data_block::verify_field_content_x(const char* data, guint32 size)
{
	const char type_id = 'x';
	// create a '\0'-terminated string
	std::string temp(data, size);
	std::string key;
	const char* p;
	const char* tag;
	bool have_errors = false;
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
			print_info("Warning. Index item %s. Type id '%c'. The field refers to resource \"%s\", "
				"that is not found in resource storage.\n",
				word, type_id, key.c_str());
			if(fix_errors) {
				print_info(fixed_ignore_msg);
			}
		}
		p = tag + sizeof("</rref>") - 1;
	}
	return have_errors ? EXIT_FAILURE : EXIT_SUCCESS;
}

int dictionary_data_block::verify_field_content_r(const char* const data, guint32 size,
	resitem_vect_t *items)
{
	const char type_id = 'r';
	const char* line_beg = data;
	const char* line_end;
	resitem_t resitem;
	bool have_errors = false;
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
			print_info("Index item %s. Type id '%c'. Empty resource line.\n",
				word, type_id);
			if(fix_errors) {
				print_info(fixed_ignore_line_msg);
				++line_beg;
				continue;
			} else {
				have_errors = true;
				continue;
			}
		}
		const std::string line(line_beg, line_end - line_beg);
		const char* colon = (const char*)memchr(line_beg, ':', line_end - line_beg);
		if(!colon) {
			print_info("Index item %s. Type id '%c'. Line: %s. ':' is not found.\n",
				word, type_id, line.c_str());
			if(fix_errors) {
				print_info(fixed_ignore_line_msg);
				line_beg = line_end + 1;
				continue;
			} else {
				have_errors = true;
				continue;
			}
		}
		resitem.type.assign(line_beg, colon - line_beg);
		++colon;
		resitem.key.assign(colon, line_end - colon);
		line_beg = line_end + 1;
		if(resitem.type.empty()) {
			print_info("Index item %s. Type id '%c'. Line: %s. Type is blank.\n",
				word, type_id, line.c_str());
			if(fix_errors) {
				print_info(fixed_ignore_line_msg);
				continue;
			} else {
				have_errors = true;
				continue;
			}
		}
		if(resitem.key.empty()) {
			print_info("Index item %s. Type id '%c'. Line: %s. Key is blank.\n",
				word, type_id, line.c_str());
			if(fix_errors) {
				print_info(fixed_ignore_line_msg);
				continue;
			} else {
				have_errors = true;
				continue;
			}
		}
		if(!is_known_resource_type(resitem.key.c_str())) {
			print_info("Index item %s. Type id '%c'. Line: %s. Unknown type.\n",
				word, type_id, line.c_str());
			if(fix_errors) {
				print_info(fixed_ignore_line_msg);
				continue;
			} else {
				have_errors = true;
				continue;
			}
		}
		if(resitem.key.find('\\') != std::string::npos) {
			print_info("Index item %s. Type id '%c'. Line: %s. Key contains '\\' char.\n",
				word, type_id, line.c_str());
			if(fix_errors) {
				print_info(fixed_ignore_line_msg);
				continue;
			} else {
				have_errors = true;
				continue;
			}
		}
		if(p_res_storage && !p_res_storage->have_file(resitem.key)) {
			print_info("Index item %s. Type id '%c'. Line %s. The field refers to resource \"%s\", "
				"that is not found in resource storage.\n",
				word, type_id, line.c_str(), resitem.key.c_str());
			if(fix_errors) {
				print_info(fixed_ignore_line_msg);
				continue;
			}
		}
		if(items)
			items->push_back(resitem);
		++item_num;
	}
	if(item_num == 0) {
		print_info("Index item %s. Type id '%c'. Empty resource list.\n",
			word, type_id);
		have_errors = true;
	}
	return have_errors ? EXIT_FAILURE : EXIT_SUCCESS;
}

