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

#ifndef LIB_COMMON_DICT_H_
#define LIB_COMMON_DICT_H_

/* internal representation of a normal dictionary
 * This format should be used by all parsers and generators. */

#include <string>
#include <vector>
#include <cstdlib>
#include "libcommon.h"
#include "ifo_file.h"

struct resource_t
{
	resource_t(const std::string& type, const std::string& key)
	:
		type(type),
		key(key)
	{

	}
	std::string type;
	std::string key;
};

typedef std::vector<resource_t> resource_vect_t;

struct article_def_t
{
	article_def_t(char type, size_t offset, size_t size)
	:
		type(type),
		offset(offset),
		size(size)
	{

	}
	article_def_t(void)
	:
		type('r'),
		offset(0),
		size(0)
	{

	}
	int add_resource(const resource_t& res)
	{
		if(type != 'r')
			return EXIT_FAILURE;
		resources.push_back(res);
		return EXIT_SUCCESS;
	}
	char type;
	// used when type == 'r'
	resource_vect_t resources;
	// used when type != 'r'
	/* offset and size of the definition data in external file
	 * We store definitions outside to conserve memory.
	 * A dictionary may be quite large, there is no guarantee it can be completely loaded into memory.
	 * Even if it can be loaded, we try to minimize memory footprint.
	 * We store in the memory only minimal part of an article: keys, synonyms and references to
	 * data stored outside.
	 *
	 * For lower case types size does not include the terminating null character.
	 * That is, size = length(definition). Do not store the terminating null char in the external file!*/
	size_t offset;
	size_t size;
};

struct article_data_t
{
	std::string key;
	std::vector<std::string> synonyms;
	std::vector<article_def_t> definitions;
	void clear(void);
	int add_key(const std::string& new_key);
	int add_synonym(const std::string& new_synonym);
	int add_definition(const article_def_t& def);
};

class common_dict_t
{
public:
	common_dict_t(void);
	std::vector<article_data_t> articles;
	DictInfo dict_info;
	int reset(void);
	int write_data(const char* data, size_t size, size_t& offset);
	int read_data(char* data, size_t size, size_t offset);
	int add_article(const article_data_t& article);
	void set_lot_of_memory(bool b)
	{
		lot_of_memory = b;
	}
private:
	TempFile contents_file_creator;
	std::string contents_file_name;
	clib::File contents_file;
	/* Then true, do not use temporary file, store data in memory. */
	bool lot_of_memory;
	std::vector<char> data_store;
};

#endif /* LIB_COMMON_DICT_H_ */
