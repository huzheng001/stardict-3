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

#ifndef LIB_DICT_DATA_BLOCK_H_
#define LIB_DICT_DATA_BLOCK_H_

#include <algorithm>
#include "lib_res_store.h"
#include "lib_dict_verify.h"

/* field verification result */
enum FieldVerifResult {
	FIELD_VERIF_RES_OK, // everything is fine, this field may be used
	// skip (ignore) this field, may go to the next field
	// This field is not recoverable, but the end of the field is reliable.
	FIELD_VERIF_RES_SKIP,
	// abort processing the sequence of fields
	// Often because it's undefined where the next field starts.
	FIELD_VERIF_RES_ABORT
};

inline
FieldVerifResult combine_result(FieldVerifResult a, FieldVerifResult b)
{
	return std::max(a, b);
}

/* extract result */
struct ext_result_t {
	ext_result_t()
	:
		field(FIELD_VERIF_RES_OK),
		content(VERIF_RESULT_OK)
	{
	}
	ext_result_t(FieldVerifResult field, VerifResult content)
	:
		field(field),
		content(content)
	{
	}
	ext_result_t& operator=(const ext_result_t& right)
	{
		field = right.field;
		content = right.content;
		return *this;
	}
	ext_result_t& append(const ext_result_t& right)
	{
		append(right.field);
		append(right.content);
		return *this;
	}
	void append(FieldVerifResult result)
	{
		field = combine_result(field, result);
	}
	void append(VerifResult result)
	{
		content = combine_result(content, result);
	}
	VerifResult summary(void) const
	{
		return content;
	}
	FieldVerifResult field; // field extraction result.
	VerifResult content; // field content result. Is content OK or not?
};

struct data_field_t
{
	data_field_t(void)
	:
		type_id(0)
	{
	}

	char type_id;
	/* for string data types, return string length,
	 * for binary data types, return data size */
	size_t get_size(void) const;
	/* for string data types, return a '\0'-terminated string. */
	const char* get_data(void) const;
	void set_data(const char* p, size_t size, bool add_null = false);
private:
	/* for string data types, like 'm', data ends with '\0' char,
	 * for binary data types, the vector contains only data. */
	std::vector<char> data;
};

typedef std::vector<data_field_t> data_field_vect_t;

class dictionary_data_block {
public:
	dictionary_data_block(void)
	:
		word(NULL),
		p_res_storage(NULL),
		fix_errors(false),
		fields(NULL),
		field_num(0)
	{

	}
	VerifResult load(const char* const data, size_t data_size,
		const std::string& sametypesequence, const char* word,
		data_field_vect_t* fields = NULL);
	void set_resource_storage(i_resource_storage* p_res_storage)
	{
		this->p_res_storage = p_res_storage;
	}
	void set_fix_errors(bool b)
	{
		fix_errors = b;
	}
	void set_word(const char* word)
	{
		this->word = word;
	}
	/* if you use this method directly, do not forget to set_word(). NULL as argument is OK.
	 * any fatal error may be solved by ignoring this field */
	VerifResult verify_field_content_r(const char* const data, guint32 size, resitem_vect_t *items = NULL);
private:
	VerifResult load_no_sametypesequence(const char* const data, size_t data_size);
	VerifResult load_sametypesequence(const char* const data, size_t data_size,
		const std::string& sametypesequence);
	/* for all load_field* methods
	 * all method have two means to indicate processing result.
	 * ext_result_t.content holds the integral result of the processing the field.
	 * VERIF_RESULT_FATAL is counted as VERIF_RESULT_CRITICAL by caller function.
	 * Any fatal error may be solved by ignoring the field or entire field collection.
	 * ext_result_t.field indicates what we can do next (switch to the next field,
	 * or abort processing the field collection).
	 * p parameter initially point to the beginning of the data area.
	 * Field extraction method must move it past the processed field,
	 * to the beginning of the next field.
	 * size of the available data is restricted by size_remain parameter.
	 * Extraction function is not allowed to access data outside this region.
	 * The field may occupy either full region or only part of it.
	 * Extraction function should read as much data as it needs but not more.
	 *
	 * fix_errors. When true, we are working hard to all fix errors, extract as much data
	 * as possible. We performs as many tests as possible, testing fixed data.
	 *
	 * fields. When specified, all extracted fields are added here.
	 * When fix_errors is specified, we add only clean fields, after all possible fixes.
	 * When fix_errors is not specified, we fix only errors >= VERIF_RESULT_CRITICAL.
	 * What should we do in that last case? We need to fix some errors anyway,
	 * even when fix_errors is false. Otherwise we can not go forward. */
	ext_result_t load_field(const char type_id,
		const char*& p, size_t size_remain);
	ext_result_t load_field_upper(const char type_id,
		const char*& p, size_t size_remain);
	ext_result_t load_field_lower(const char type_id,
		const char*& p, size_t size_remain);
	ext_result_t load_field_sametypesequence_last_upper(const char type_id,
		const char*& p, size_t size_remain);
	ext_result_t load_field_sametypesequence_last_lower(const char type_id,
		const char*& p, size_t size_remain);
	VerifResult verify_field_content(const char type_id, const char* data, guint32 size);
	VerifResult verify_field_content_x(const char* data, guint32 size);
	void add_field(char type_id, const char* data, size_t datalen, bool add_null = false);

	const char* word;
	i_resource_storage* p_res_storage; // may be NULL
	bool fix_errors;
	data_field_vect_t* fields;
	size_t field_num; // number of fields extracted
};


#endif /* LIB_DICT_DATA_BLOCK_H_ */
