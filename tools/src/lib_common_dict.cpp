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

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <algorithm>
#include <errno.h>
#include <cstring>
#include "lib_common_dict.h"

void article_data_t::clear(void)
{
	key.clear();
	synonyms.clear();
	definitions.clear();
}

/* Return value:
 * EXIT_FAILURE - key already exists
 * EXIT_SUCCESS - key added
 * */
int article_data_t::add_key(const std::string& new_key)
{
	if(new_key.empty())
		return EXIT_FAILURE;
	if(key == new_key)
		return EXIT_FAILURE;
	if(std::find(synonyms.begin(), synonyms.end(), new_key) != synonyms.end())
		return EXIT_FAILURE;
	if(key.empty()) {
		key = new_key;
		return EXIT_SUCCESS;
	}
	synonyms.push_back(new_key);
	return EXIT_SUCCESS;
}

/* Return value:
 * EXIT_FAILURE - key or synonym already exists
 * EXIT_SUCCESS - synonym added
 * */
int article_data_t::add_synonym(const std::string& new_synonym)
{
	if(new_synonym.empty())
		return EXIT_FAILURE;
	if(key == new_synonym)
		return EXIT_FAILURE;
	if(std::find(synonyms.begin(), synonyms.end(), new_synonym) != synonyms.end())
		return EXIT_FAILURE;
	synonyms.push_back(new_synonym);
	return EXIT_SUCCESS;
}

/* Return value:
 * EXIT_FAILURE or EXIT_SUCCESS */
int article_data_t::add_definition(const article_def_t& def)
{
	definitions.push_back(def);
	return EXIT_SUCCESS;
}

common_dict_t::common_dict_t(void)
:
	lot_of_memory(false)
{

}

/* clean articles
 * prepare a new file to write data */
int common_dict_t::reset(void)
{
	articles.clear();
	// First close the previous temporary file, otherwise we'll cannot remove it.
	contents_file.reset(NULL);
	contents_file_name.clear();
	data_store.clear();
	if(!lot_of_memory) {
		contents_file_name = contents_file_creator.create_temp_file();
		if(contents_file_name.empty())
			return EXIT_FAILURE;
		// open file for reading and writing
		contents_file.reset(g_fopen(contents_file_name.c_str(), "wb+"));
		if(!contents_file) {
			g_critical(create_temp_file_err, contents_file_name.c_str());
			return EXIT_FAILURE;
		}
	}
	dict_info.clear();
	return EXIT_SUCCESS;
}

/* write data to contents file.
 * set offset to the beginning of the record in the file */
int common_dict_t::write_data(const char* data, size_t size, size_t& offset)
{
	if(!data || size == 0) {
		g_critical(incorrect_arg_err);
		return EXIT_FAILURE;
	}
	if(lot_of_memory) {
		offset = data_store.size();
		data_store.resize(data_store.size()+size);
		memcpy(&data_store[offset], data, size);
	} else {
		if(fseek(get_impl(contents_file), 0, SEEK_END)) {
			std::string error(g_strerror(errno));
			g_critical(read_file_err, contents_file_name.c_str(), error.c_str());
			return EXIT_FAILURE;
		}
		offset = ftell(get_impl(contents_file));
		if(1 != fwrite(data, size, 1, get_impl(contents_file))) {
			g_critical(write_file_err, contents_file_name.c_str());
			return EXIT_FAILURE;
		}
	}
	return EXIT_SUCCESS;
}

/* read data from contents file
 * offset - offset of the first byte in the file,
 * size - size of the data in bytes
 * data - where to save data, must point to a buffer of enough size */
int common_dict_t::read_data(char* data, size_t size, size_t offset)
{
	if(!data || size == 0) {
		g_critical(incorrect_arg_err);
		return EXIT_FAILURE;
	}
	if(lot_of_memory) {
		if(offset+size > data_store.size())
			return EXIT_FAILURE;
		memcpy(data, &data_store[offset], size);
	} else {
		if(fseek(get_impl(contents_file), offset, SEEK_SET)) {
			std::string error(g_strerror(errno));
			g_critical(read_file_err, contents_file_name.c_str(), error.c_str());
			return EXIT_FAILURE;
		}
		if(1 != fread(data, size, 1, get_impl(contents_file))) {
			std::string error(g_strerror(errno));
			g_critical(read_file_err, contents_file_name.c_str(), error.c_str());
			return EXIT_FAILURE;
		}
	}
	return EXIT_SUCCESS;
}

int common_dict_t::add_article(const article_data_t& article)
{
	articles.push_back(article);
	return EXIT_SUCCESS;
}
