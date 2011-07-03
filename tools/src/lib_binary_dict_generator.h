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

#ifndef _LIB_BINARY_DICT_GENERATOR_H_
#define _LIB_BINARY_DICT_GENERATOR_H_

#include <string>
#include <glib.h>
#include "lib_common_dict.h"
#include "libcommon.h"

/* generate binary normal dictionary */
class binary_dict_gen_t
{
public:
	binary_dict_gen_t(void);
	int generate(const std::string& ifofilename, common_dict_t *norm_dict);
	void clear(void);
	void set_use_same_type_sequence(bool b)
	{
		use_same_type_sequence = b;
	}
	void set_compress_dict(bool b)
	{
		compress_dict = b;
	}
private:
	int generate_dict_and_idx(void);
	int generate_syn(void);
	int prepare_dict(void);
	int prepare_idx(void);
	int prepare_syn(void);
	int generate_dict_definition(const article_def_t& def, const std::string& key);
	int generate_dict_definition_sts(const article_def_t& def, const std::string& key, bool last);
	int generate_dict_definition_r(const resource_vect_t& resources, const std::string& key);
	int generate_dict_definition_r_sts(const resource_vect_t& resources, const std::string& key, bool last);
	int generate_index_item(const std::string& key, guint32 offset, guint32 size);
	void decide_on_same_type_sequence(void);
	std::string build_type_sequence(const article_data_t& article) const;
	common_dict_t *norm_dict;
	std::string basefilename;
	std::string ifofilename;
	std::string dictfilename;
	std::string idxfilename;
	std::string synfilename;
	clib::File dictfile;
	clib::File idxfile;
	clib::File synfile;
	/* use same_type_sequence if possible */
	bool use_same_type_sequence;
	/* If use_same_type_sequence == true and all articles have the same sequence of types,
	 * this string contains the sequence of types to use.
	 * Otherwise this is an empty string. */
	std::string same_type_sequence;
	/* run dictzip on the generated file if enabled */
	bool compress_dict;
};

#endif
