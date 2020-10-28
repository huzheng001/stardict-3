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
#include <errno.h>

#ifdef _WIN32
#  include <windows.h>
#else
#  include <unistd.h>
#endif

#include "lib_res_store.h"
#include "libcommon.h"
#include "ifo_file.h"
#include "lib_dict_verify.h"

struct fileitem_t {
	std::string filename;
	guint32 offset;
	guint32 size;
};

static bool compare_fileitem(const fileitem_t& left, const fileitem_t& right)
{
	return 0 > strcmp(left.filename.c_str(), right.filename.c_str());
}

static bool compare_fileitem_by_offset(const fileitem_t* left, const fileitem_t* right)
{
	return left->offset < right->offset;
}

class resource_database
{
public:
	resource_database()
	:
		verif_result(VERIF_RESULT_OK)
	{
	}
	TLoadResult load(const std::string& dirname);
	// filename uses database directory separator
	bool have_file(const std::string& filename) const;
	VerifResult get_verif_result(void) const { return verif_result; }
	/* true if res.ridx.gz used, res.ridx otherwise */
	bool res_ridx_compressed(void) const
	{
		return ridxfilename != ridxfilename_orig;
	}
	/* true if res.rdic.dz used, res.rdic otherwise */
	bool res_rdic_compressed(void) const
	{
		return rdicfilename != rdicfilename_orig;
	}
private:
	int prepare_ridx_file(void);
	int prepare_rdic_file(void);
	int load_rifo_file(void);
	int load_ridx_file(void);
	VerifResult load_rdic_file(void);
	void print_index(void);
	VerifResult verify_data_blocks_overlapping(void);

	std::string rifofilename;
	std::string ridxfilename;
	std::string ridxfilename_orig;
	std::string rdicfilename;
	std::string rdicfilename_orig;
	std::string dirname;
	TempFile ridxtemp;
	TempFile rdictemp;
	DictInfo dict_info;
	std::vector<fileitem_t> index;
	guint32 rdicfilesize;
	VerifResult verif_result;
};

TLoadResult resource_database::load(const std::string& dirname)
{
	this->dirname = dirname;
	verif_result = VERIF_RESULT_OK;

	rifofilename = build_path(dirname, "res.rifo");
	if(!g_file_test(rifofilename.c_str(), G_FILE_TEST_EXISTS))
		return lrNotFound;

	if(load_rifo_file()) {
		verif_result = combine_result(verif_result, VERIF_RESULT_FATAL);
		return lrError;
	}
	if(load_ridx_file()) {
		verif_result = combine_result(verif_result, VERIF_RESULT_FATAL);
		return lrError;
	}
	verif_result = combine_result(verif_result, load_rdic_file());
	return VERIF_RESULT_CRITICAL <= verif_result ? lrError : lrOK;
}

int resource_database::prepare_ridx_file(void)
{
	const std::string index_file_name_gz = build_path(dirname, "res.ridx.gz");
	const std::string index_file_name_ridx = build_path(dirname, "res.ridx");
	if(g_file_test(index_file_name_gz.c_str(), G_FILE_TEST_EXISTS)
		&& g_file_test(index_file_name_ridx.c_str(), G_FILE_TEST_EXISTS)) {
		g_warning(rdb_two_index_files_msg, index_file_name_gz.c_str(), index_file_name_ridx.c_str());
		verif_result = combine_result(verif_result, VERIF_RESULT_WARNING);
	}
	ridxfilename_orig = index_file_name_gz;
	if(g_file_test(ridxfilename_orig.c_str(), G_FILE_TEST_EXISTS)) {
		ridxfilename = ridxtemp.create_temp_file();
		if(ridxfilename.empty())
			return EXIT_FAILURE;
		if(EXIT_FAILURE == unpack_zlib(ridxfilename_orig.c_str(), ridxfilename.c_str()))
			return EXIT_FAILURE;
	} else {
		ridxfilename_orig = index_file_name_ridx;
		ridxfilename = ridxfilename_orig;
	}
	return EXIT_SUCCESS;
}

int resource_database::prepare_rdic_file(void)
{
	const std::string dict_file_name_dz = build_path(dirname, "res.rdic.dz");
	const std::string dict_file_name_rdic = build_path(dirname, "res.rdic");
	if(g_file_test(dict_file_name_dz.c_str(), G_FILE_TEST_EXISTS)
		&& g_file_test(dict_file_name_rdic.c_str(), G_FILE_TEST_EXISTS)) {
		g_warning(rdb_two_dict_files_msg, dict_file_name_dz.c_str(), dict_file_name_rdic.c_str());
		verif_result = combine_result(verif_result, VERIF_RESULT_WARNING);
	}
	rdicfilename_orig = dict_file_name_dz;
	if(g_file_test(rdicfilename_orig.c_str(), G_FILE_TEST_EXISTS)) {
		rdicfilename = rdictemp.create_temp_file();
		if(rdicfilename.empty())
			return EXIT_FAILURE;
		if(unpack_zlib(rdicfilename_orig.c_str(), rdicfilename.c_str()))
			return EXIT_FAILURE;
	} else {
		rdicfilename_orig = dict_file_name_rdic;
		rdicfilename = rdicfilename_orig;
	}
	return EXIT_SUCCESS;
}

int resource_database::load_rifo_file(void)
{
	if(!dict_info.load_from_ifo_file(rifofilename, DictInfoType_ResDb))
		return EXIT_FAILURE;
	bool have_errors = false;
	if(dict_info.get_filecount() == 0) {
		g_critical(rdb_filecnt_zero_err, rifofilename.c_str());
		have_errors = true;
	}
	if(dict_info.get_index_file_size() == 0) {
		g_critical(rdb_ridxfilesize_zero_err, rifofilename.c_str());
		have_errors = true;
	}
	return have_errors ? EXIT_FAILURE : EXIT_SUCCESS;
}

int resource_database::load_ridx_file(void)
{
	if(prepare_ridx_file())
		return EXIT_FAILURE;

	stardict_stat_t stats;
	if (g_stat (ridxfilename.c_str(), &stats) == -1) {
		std::string error(g_strerror(errno));
		g_critical(file_not_found_idx_err, ridxfilename.c_str(), error.c_str());
		return EXIT_FAILURE;
	}
	g_message(rdb_loading_ridx_file_msg, ridxfilename_orig.c_str());
	if (dict_info.get_index_file_size()!=(guint)stats.st_size) {
		g_critical(incorrect_ridx_file_size_err,
			dict_info.get_index_file_size(), (long) stats.st_size);
		return EXIT_FAILURE;
	}

	index.clear();
	index.reserve(dict_info.get_filecount());

	std::vector<gchar> buf(stats.st_size+1);
	gchar * const buffer_beg = &buf[0];
	gchar * const buffer_end = buffer_beg+stats.st_size;
	{
		FILE *idxfile = g_fopen(ridxfilename.c_str(),"rb");
		size_t fread_size;
		fread_size = fread(buffer_beg, 1, stats.st_size, idxfile);
		if (fread_size != (size_t)stats.st_size) {
			g_print("fread error!\n");
		}
		fclose(idxfile);
	}

	gchar *p=buffer_beg;
	gchar *prefilename=NULL;
	int filenamelen;
	guint filecount=0;
	bool have_errors=false;
	fileitem_t fileitem;
	size_t size_remain; // to the end of the index file

	while (p < buffer_end) {
		size_remain = buffer_end - p;
		const char* p2 = reinterpret_cast<const char*>(memchr(p, '\0', size_remain));
		if(!p2) {
			g_warning(index_file_truncated_err);
			have_errors=true;
			break;
		}
		filenamelen = p2 - p;
		if (filenamelen==0) {
			g_warning(empty_file_name_err);
			have_errors=true;
		}
		if (!g_utf8_validate(p, filenamelen, NULL)) {
			std::string tmp(p, filenamelen);
			g_warning(invalid_utf8_index_item_err, p, tmp.c_str());
			have_errors=true;
		}
		if(strchr(p, '\\')) {
			g_warning(rdb_invalid_file_name_format_back_spash_err, p);
			have_errors=true;
		}
		if(p[0] == '/') {
			g_warning(rdb_invalid_file_name_format_abs_path_err, p);
			have_errors=true;
		}
		if(strstr(p, "//")) {
			g_warning(rdb_invalid_file_name_format_empty_dir_err, p);
			have_errors=true;
		}
		if (prefilename) {
			int cmpvalue=strcmp(prefilename, p);
			if (cmpvalue>0) {
				g_warning(wrong_file_order_err, prefilename, p);
				have_errors=true;
			}
			if(cmpvalue==0) {
				g_warning(duplicate_file_name, p);
				have_errors=true;
			}
		}
		prefilename=p;
		fileitem.filename = p;
		p += filenamelen + 1;
		size_remain = buffer_end - p;
		if(size_remain < 2 * sizeof(guint32)) {
			g_warning(index_file_truncated_err);
			have_errors=true;
			break;
		}
		fileitem.offset = g_ntohl(*reinterpret_cast<guint32 *>(p));
		p += sizeof(guint32);
		fileitem.size = g_ntohl(*reinterpret_cast<guint32 *>(p));
		p += sizeof(guint32);
		if (fileitem.size==0) {
			g_warning(empty_block_err, prefilename);
		}
		filecount++;
		index.push_back(fileitem);
	} // while

	g_assert(p <= buffer_end);

	if (filecount!=dict_info.get_filecount()) {
		g_warning(rdb_incorrect_file_cnt, dict_info.get_filecount(), filecount);
		have_errors=true;
	}

	return have_errors ? EXIT_FAILURE : EXIT_SUCCESS;
}

VerifResult resource_database::load_rdic_file(void)
{
	VerifResult result = VERIF_RESULT_OK;
	if(prepare_rdic_file())
		return combine_result(result, VERIF_RESULT_FATAL);

	stardict_stat_t stats;
	if (g_stat (rdicfilename.c_str(), &stats) == -1) {
		std::string error(g_strerror(errno));
		g_critical(rdb_dict_file_not_found_err, rdicfilename.c_str(), error.c_str());
		return combine_result(result, VERIF_RESULT_FATAL);
	}
	rdicfilesize = stats.st_size;

	g_message(rdb_loading_dict_file_msg, rdicfilename_orig.c_str());
	clib::File rdicfile(g_fopen(rdicfilename.c_str(), "rb"));
	if(!rdicfile) {
		std::string error(g_strerror(errno));
		g_critical(open_read_file_err, rdicfilename.c_str(), error.c_str());
		return combine_result(result, VERIF_RESULT_FATAL);
	}

	for(size_t i=0; i<index.size(); ++i) {
		if(index[i].offset + index[i].size > rdicfilesize) {
			g_warning(record_out_of_file_err, index[i].filename.c_str());
			result = combine_result(result, VERIF_RESULT_CRITICAL);
			continue;
		}
	}
	result = combine_result(result, verify_data_blocks_overlapping());
	return result;
}

bool resource_database::have_file(const std::string& filename) const
{
	fileitem_t fileitem;
	fileitem.filename = filename;
	return std::binary_search(index.begin(), index.end(), fileitem, compare_fileitem);
}

void resource_database::print_index(void)
{
	for(size_t i=0; i<index.size(); ++i) {
		g_print("Info: index item '%s'\n", index[i].filename.c_str());
	}
}

VerifResult resource_database::verify_data_blocks_overlapping(void)
{
	VerifResult result = VERIF_RESULT_OK;
	std::vector<const fileitem_t*> sort_index(index.size(), NULL);
	for(size_t i=0; i<index.size(); ++i)
		sort_index[i] = &index[i];
	std::sort(sort_index.begin(), sort_index.end(), compare_fileitem_by_offset);
	// find overlapping but not equal regions (offset, size)
	std::vector<std::pair<size_t, size_t> > overlapping_blocks;
	::verify_data_blocks_overlapping(sort_index, overlapping_blocks);
	for(size_t i=0; i<overlapping_blocks.size(); ++i) {
		const fileitem_t& first = *sort_index[overlapping_blocks[i].first];
		const fileitem_t& second = *sort_index[overlapping_blocks[i].second];
		g_warning(overlapping_data_blocks_msg,
			first.filename.c_str(), second.filename.c_str(),
			first.offset, first.size, second.offset, second.size);
		result = combine_result(result, VERIF_RESULT_WARNING);
	}
	// find not used regions
	std::vector<region_t> unused_regions;
	verify_unused_regions(sort_index, unused_regions, rdicfilesize);
	if(!unused_regions.empty()) {
		g_warning(rdb_unreferenced_data_blocks_msg);
		for(size_t i = 0; i<unused_regions.size(); ++i)
			g_warning("\t(%u, %u)\n", unused_regions[i].offset, unused_regions[i].size);
		result = combine_result(result, VERIF_RESULT_NOTE);
	}
	return result;
}


class resource_files
{
public:
	TLoadResult load(const std::string& dirname);
	// filename uses database directory separator
	bool have_file(const std::string& filename) const;
private:
	std::string dirname;
	std::string resdirname;
};

TLoadResult resource_files::load(const std::string& dirname)
{
	this->dirname = dirname;
	resdirname = build_path(dirname, "res");
	if(!g_file_test(resdirname.c_str(), G_FILE_TEST_IS_DIR))
		return lrNotFound;
	return lrOK;
}

bool resource_files::have_file(const std::string& filename) const
{
	const std::string full_fs_filename(build_path(resdirname, dir_separator_db_to_fs(filename)));
	return static_cast<bool>(g_file_test(full_fs_filename.c_str(), G_FILE_TEST_IS_REGULAR));
}


resource_storage::resource_storage(void)
:
	db(NULL),
	files(NULL),
	verif_result(VERIF_RESULT_OK)
{

}

resource_storage::~resource_storage(void)
{
	clear();
}

TLoadResult resource_storage::load(const std::string& dirname)
{
	clear();
	std::unique_ptr<resource_database> t_db(new resource_database);
	TLoadResult res = t_db->load(dirname);
	if(res == lrOK) {
		g_message(rdb_loaded_db_msg);
		verif_result = t_db->get_verif_result();
		db = t_db.release();
		return lrOK;
	}
	if(res == lrError) {
		g_critical(rdb_load_db_failed_msg);
		verif_result = t_db->get_verif_result();
		return lrError;
	}
	std::unique_ptr<resource_files> t_files(new resource_files);
	res = t_files->load(dirname);
	if(res == lrOK) {
		g_message(rdb_loaded_files_msg);
		verif_result = VERIF_RESULT_OK;
		files = t_files.release();
		return lrOK;
	}
	if(res == lrError) {
		g_critical(rdb_load_files_failed_msg);
		verif_result = VERIF_RESULT_FATAL;
		return lrError;
	}
	verif_result = VERIF_RESULT_OK;
	return res;
}

bool resource_storage::have_file(const std::string& filename) const
{
	if(db)
		return db->have_file(filename);
	if(files)
		return files->have_file(filename);
	return false;
}

StorageType resource_storage::get_storage_type(void) const
{
	if(db)
		return StorageType_DATABASE;
	if(files)
		return StorageType_FILE;
	return StorageType_UNKNOWN;
}

bool resource_storage::res_ridx_compressed(void) const
{
	if(db)
		return db->res_ridx_compressed();
	return false;
}

bool resource_storage::res_rdic_compressed(void) const
{
	if(db)
		return db->res_rdic_compressed();
	return false;
}

void resource_storage::clear(void)
{
	if(db)
		delete db;
	db = NULL;
	if(files)
		delete files;
	files = NULL;
	verif_result = VERIF_RESULT_OK;
}

