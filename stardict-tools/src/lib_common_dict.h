#ifndef LIB_COMMON_DICT_H_
#define LIB_COMMON_DICT_H_

/* internal representation of a normal dictionary
 * This format should be used by all parsers and generators. */

#include <string>
#include <vector>
#include <cstdlib>
#include "libcommon.h"
#include "resourcewrap.hpp"
#include "ifo_file.hpp"

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
	void set_print_info(print_info_t print_info)
	{
		this->print_info = print_info;
	}
	int add_article(const article_data_t& article);
private:
	TempFile contents_file_creator;
	std::string contents_file_name;
	clib::File contents_file;
	print_info_t print_info;
};

#endif /* LIB_COMMON_DICT_H_ */
