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
#include <memory>
#include <string>
#include <iostream>
#include <cstring>
#include <cstdlib>
#include "storage_impl.h"
#include "ifo_file.h"
#include "utils.h"

struct index_entry_t {
	index_entry_t(void): off(0), size(0) {}
	index_entry_t(const char* s, guint32 off_, guint32 size_)
	: key(s), off(off_), size(size_)
	{
	}
	std::string key;
	guint32 off, size;
};

typedef std::vector<index_entry_t> index_vect_t;
enum index_type_t {itRidx, itRidxGz};

bool load_index(const std::string& ridx_url, index_vect_t &index_vect, 
	gulong filesize, gulong filecount, index_type_t index_type)
{
	std::vector<char> buffer(filesize);
	if(index_type == itRidx) {
		FILE *f = fopen(ridx_url.c_str(), "rb");
		if(!f)
			return false;
		size_t the_size;
		the_size = fread(&buffer[0], 1, filesize, f);
		if (the_size != filesize) {
			g_print("fread error!\n");
		}
		fclose(f);
	} else if(index_type == itRidxGz) {
		gzFile f = gzopen(ridx_url.c_str(), "rb");
		if (f == NULL)
			return false;
		gulong len = gzread(f, &buffer[0], filesize);
		gzclose(f);
		if(len != filesize)
			return false;
	} else
		return false;
	index_vect.resize(filecount);
	char *p = &buffer[0];
	for(gulong i=0; i<filecount; ++i) {
		index_vect[i].key = p;
		p += strlen(p) + 1;
		index_vect[i].off = g_ntohl(get_uint32(p));
		p += sizeof(guint32);
		index_vect[i].size = g_ntohl(get_uint32(p));
		p += sizeof(guint32);
	}
	g_assert(gulong(p-&buffer[0]) == filesize);
	return true;
}

bool lookup_test_key(rindex_file *pindex, const std::string& key, 
	guint32 true_offset, guint32 true_size)
{
	guint32 offset, size;
	if(!pindex->lookup(key.c_str(), offset, size)
		|| offset != true_offset || size != true_size) {
		std::cerr << "lookup failed for key: " << key << std::endl;
		return false;
	}
	return true;
}

bool lookup_test(const index_vect_t &index_vect, rindex_file *pindex)
{
	const size_t ntest = 1000;
	std::vector<int> test_indexes(2+ntest);
	test_indexes[0] = 0;
	test_indexes[1] = index_vect.size() - 1;
	for(size_t i=2; i<test_indexes.size(); ++i)
		test_indexes[i] = rand()%index_vect.size();
	for(size_t i=0; i<test_indexes.size(); ++i) {
		if(!lookup_test_key(pindex, index_vect[test_indexes[i]].key,
			index_vect[test_indexes[i]].off, index_vect[test_indexes[i]].size))
			return false;
	}
	return true;
}

/* usage:
 * t_res_database <res database info file> */
int main(int argc, char *argv[])
{
	setlocale(LC_ALL, "");
	if(argc != 2)
		return 1;
	std::string rifo_url(argv[1]);
	if(rifo_url.rfind(".rifo") != rifo_url.length() + 1 - sizeof(".rifo")) {
		std::cerr << "incorrect parameter, expecting .rifo file" << std::endl;
		return 1;
	}
	DictInfo dict_info;
	if (!dict_info.load_from_ifo_file(rifo_url, DictInfoType_ResDb))
		return 1;

	gulong filecount = dict_info.get_filecount();
	gulong indexfilesize = dict_info.get_index_file_size();

	std::unique_ptr<rindex_file> pindex;
	std::string base_url = rifo_url.substr(0, rifo_url.length() + 1 - sizeof(".rifo"));
	std::string ridx_url = base_url + ".ridx";
	index_type_t index_type;
	if(g_file_test(ridx_url.c_str(), G_FILE_TEST_EXISTS)) {
		pindex.reset(new offset_rindex);
		index_type = itRidx;
	} else {
		ridx_url = base_url + ".ridx.gz";
		if(g_file_test(ridx_url.c_str(), G_FILE_TEST_EXISTS)) {
			pindex.reset(new compressed_rindex);
			index_type = itRidxGz;
		} else {
			std::cerr << "index file not found" << std::endl;
			return 1;
		}
	}
	if(!pindex->load(ridx_url, filecount, indexfilesize, false)) {
		std::cerr << "unable to load index: " << ridx_url << std::endl;
		return 1;
	}
	index_vect_t index_vect;
	if(!load_index(ridx_url, index_vect, indexfilesize, filecount, index_type)) {
		std::cerr << "unable to read index: " << ridx_url << std::endl;
		return 1;
	}
	if(!lookup_test(index_vect, pindex.get())) {
		std::cerr << "test failed" << std::endl;
		return 1;
	}
	std::cout << "test passed" << std::endl;
	return 0;
}
