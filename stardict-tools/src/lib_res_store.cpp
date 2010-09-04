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
#include "libstardictverify.h"

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
	TLoadResult load(const std::string& dirname, print_info_t print_info);
	// filename uses database directory separator
	bool have_file(const std::string& filename) const;
private:
	int prepare_ridx_file(void);
	int prepare_rdic_file(void);
	int load_rifo_file(void);
	int load_ridx_file(void);
	int load_rdic_file(void);
	void print_index(void);
	void verify_data_blocks_overlapping(void);

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
	print_info_t print_info;
	guint32 rdicfilesize;
};

TLoadResult resource_database::load(const std::string& dirname,
	print_info_t print_info)
{
	this->print_info = print_info;
	this->dirname = dirname;
	ridxtemp.set_print_info(print_info);
	rdictemp.set_print_info(print_info);

	rifofilename = dirname + G_DIR_SEPARATOR_S "res.rifo";
	if(!g_file_test(rifofilename.c_str(), G_FILE_TEST_EXISTS))
		return lrNotFound;

	if(EXIT_FAILURE == load_rifo_file())
		return lrError;
	if(EXIT_FAILURE == load_ridx_file())
		return lrError;
	if(EXIT_FAILURE == load_rdic_file())
		return lrError;
	return lrOK;
}

int resource_database::prepare_ridx_file(void)
{
	ridxfilename_orig = dirname + G_DIR_SEPARATOR_S "res.ridx.gz";
	if(g_file_test(ridxfilename_orig.c_str(), G_FILE_TEST_EXISTS)) {
		ridxfilename = ridxtemp.create_temp_file();
		if(ridxfilename.empty())
			return EXIT_FAILURE;
		if(EXIT_FAILURE == unpack_zlib(ridxfilename_orig.c_str(), ridxfilename.c_str(), print_info))
			return EXIT_FAILURE;
	} else {
		ridxfilename_orig = dirname + G_DIR_SEPARATOR_S "res.ridx";
		ridxfilename = ridxfilename_orig;
	}
	return EXIT_SUCCESS;
}

int resource_database::prepare_rdic_file(void)
{
	rdicfilename_orig = dirname + G_DIR_SEPARATOR_S "res.rdic.dz";
	if(g_file_test(rdicfilename_orig.c_str(), G_FILE_TEST_EXISTS)) {
		rdicfilename = rdictemp.create_temp_file();
		if(rdicfilename.empty())
			return EXIT_FAILURE;
		if(EXIT_FAILURE == unpack_zlib(rdicfilename_orig.c_str(), rdicfilename.c_str(), print_info))
			return EXIT_FAILURE;
	} else {
		rdicfilename_orig = dirname + G_DIR_SEPARATOR_S "res.rdic";
		rdicfilename = rdicfilename_orig;
	}
	return EXIT_SUCCESS;
}

int resource_database::load_rifo_file(void)
{
	dict_info.set_print_info(print_info);
	if(!dict_info.load_from_ifo_file(rifofilename, DictInfoType_ResDb))
		return EXIT_FAILURE;
	bool have_errors = false;
	if(dict_info.get_filecount() == 0) {
		print_info("Resource database %s. filecount = 0.\n", rifofilename.c_str());
		have_errors = true;
	}
	if(dict_info.get_index_file_size() == 0) {
		print_info("Resource database %s. ridxfilesize = 0.\n", rifofilename.c_str());
		have_errors = true;
	}
	return have_errors ? EXIT_FAILURE : EXIT_SUCCESS;
}

int resource_database::load_ridx_file(void)
{
	if(EXIT_FAILURE == prepare_ridx_file())
		return EXIT_FAILURE;

	stardict_stat_t stats;
	if (g_stat (ridxfilename.c_str(), &stats) == -1) {
		print_info("Unable to find index file %s\n", ridxfilename.c_str());
		return EXIT_FAILURE;
	}
	print_info("Verifying resource index file: %s\n", ridxfilename_orig.c_str());
	if (dict_info.get_index_file_size()!=(guint)stats.st_size) {
		print_info("Incorrect size of the index file: in .rifo file, ridxfilesize=%d, "
			"real file size is %ld\n",
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
		fread(buffer_beg, 1, stats.st_size, idxfile);
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
			print_info(index_file_truncated_err);
			have_errors=true;
			break;
		}
		filenamelen = p2 - p;
		if (filenamelen==0) {
			print_info("Empty file name in index.\n");
			have_errors=true;
		}
		if (!g_utf8_validate(p, filenamelen, NULL)) {
			print_info("Index item %s. Invalid utf8 string.\n", p);
			have_errors=true;
		}
		if(strchr(p, '\\')) {
			print_info("Index item %s. Found '\\' character. '/' must be used as directory separator.\n", p);
			have_errors=true;
		}
		if(p[0] == '/') {
			print_info("Index item %s. File name must not start with directory separator '/'.\n", p);
			have_errors=true;
		}
		if(strstr(p, "//")) {
			print_info("Index item %s. Empty directory in file path.\n", p);
			have_errors=true;
		}
		if (prefilename) {
			int cmpvalue=strcmp(prefilename, p);
			if (cmpvalue>0) {
				print_info("Wrong order, first filename = %s, second file name = %s\n", prefilename, p);
				have_errors=true;
			}
			if(cmpvalue==0) {
				print_info("Multiple index items with the same file name %s.\n", p);
				have_errors=true;
			}
		}
		prefilename=p;
		fileitem.filename = p;
		p += filenamelen + 1;
		size_remain = buffer_end - p;
		if(size_remain < 2 * sizeof(guint32)) {
			print_info(index_file_truncated_err);
			have_errors=true;
			break;
		}
		fileitem.offset = g_ntohl(*reinterpret_cast<guint32 *>(p));
		p += sizeof(guint32);
		fileitem.size = g_ntohl(*reinterpret_cast<guint32 *>(p));
		p += sizeof(guint32);
		if (fileitem.size==0) {
			print_info("Index item %s. Data block size = 0.\n", prefilename);
		}
		filecount++;
		index.push_back(fileitem);
	} // while

	g_assert(p <= buffer_end);

	if (filecount!=dict_info.get_filecount()) {
		print_info("Incorrect number of files: in .rifo file, filecount=%d, "
			"while the real file count is %d\n", dict_info.get_filecount(), filecount);
		have_errors=true;
	}

	return have_errors ? EXIT_FAILURE : EXIT_SUCCESS;
}

int resource_database::load_rdic_file(void)
{
	if(EXIT_FAILURE == prepare_rdic_file())
		return EXIT_FAILURE;

	stardict_stat_t stats;
	if (g_stat (rdicfilename.c_str(), &stats) == -1) {
		print_info("Unable to find resource dictionary file %s\n", rdicfilename.c_str());
		return EXIT_FAILURE;
	}
	rdicfilesize = stats.st_size;

	print_info("Verifying resource dictionary file: %s\n", rdicfilename_orig.c_str());
	clib::File rdicfile(g_fopen(rdicfilename.c_str(), "rb"));
	if(!rdicfile) {
		print_info("Unable open resource dictionary file %s.\n", rdicfilename.c_str());
		return EXIT_FAILURE;
	}

	bool have_errors = false;
	for(size_t i=0; i<index.size(); ++i) {
		if(index[i].offset + index[i].size > rdicfilesize) {
			print_info("Index item %s. Incorrect size, offset parameters. "
				"Referenced data block is outside dictionary file.\n", index[i].filename.c_str());
			have_errors = true;
			continue;
		}
	}
	verify_data_blocks_overlapping();
	return have_errors ? EXIT_FAILURE : EXIT_SUCCESS;
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
		print_info("Info: index item '%s'\n", index[i].filename.c_str());
	}
}

void resource_database::verify_data_blocks_overlapping(void)
{
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
		print_info("Warning: Index item %s and index item %s refer to overlapping "
			"but not equal regions (offset, size): "
			"(%u, %u) and (%u, %u)\n",
			first.filename.c_str(), second.filename.c_str(),
			first.offset, first.size, second.offset, second.size);
	}
	// find not used regions
	std::vector<region_t> unused_regions;
	verify_unused_regions(sort_index, unused_regions, rdicfilesize);
	if(!unused_regions.empty()) {
		print_info("Warning: Resource database contains unreferenced blocks (offset, size):\n");
		for(size_t i = 0; i<unused_regions.size(); ++i)
			print_info("\t(%u, %u)\n", unused_regions[i].offset, unused_regions[i].size);
	}
}


class resource_files
{
public:
	TLoadResult load(const std::string& dirname, print_info_t print_info);
	// filename uses database directory separator
	bool have_file(const std::string& filename) const;
private:
	std::string dirname;
	std::string resdirname;
	print_info_t print_info;
};

TLoadResult resource_files::load(const std::string& dirname, print_info_t print_info)
{
	this->dirname = dirname;
	this->print_info = print_info;
	resdirname = dirname + "/res";
	if(!g_file_test(resdirname.c_str(), G_FILE_TEST_IS_DIR))
		return lrNotFound;
	return lrOK;
}

bool resource_files::have_file(const std::string& filename) const
{
	const std::string full_fs_filename(resdirname + "/" + dir_separator_db_to_fs(filename));
	return static_cast<bool>(g_file_test(full_fs_filename.c_str(), G_FILE_TEST_IS_REGULAR));
}


resource_storage::resource_storage(void)
:
	db(NULL),
	files(NULL)
{

}

resource_storage::~resource_storage(void)
{
	clear();
}

TLoadResult resource_storage::load(const std::string& dirname, print_info_t print_info)
{
	clear();
	std::auto_ptr<resource_database> t_db(new resource_database);
	TLoadResult res = t_db->load(dirname, print_info);
	if(res == lrOK) {
		print_info("Resource storage loaded. Type - database.\n");
		db = t_db.release();
		return lrOK;
	}
	if(res == lrError) {
		print_info("Resource storage load failed. Type - database.\n");
		return lrError;
	}
	std::auto_ptr<resource_files> t_files(new resource_files);
	res = t_files->load(dirname, print_info);
	if(res == lrOK) {
		print_info("Resource storage loaded. Type - files.\n");
		files = t_files.release();
		return lrOK;
	}
	if(res == lrError) {
		print_info("Resource storage load failed. Type - files.\n");
		return lrError;
	}
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

void resource_storage::clear(void)
{
	if(db)
		delete db;
	db = NULL;
	if(files)
		delete files;
	files = NULL;
}

