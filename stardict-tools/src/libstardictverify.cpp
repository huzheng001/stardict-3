#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <cstring>
#include <cstdlib>
#include <glib/gstdio.h>
#include <glib.h>
#include <string>
#include <vector>
#include <algorithm>
#include <zlib.h>

#ifdef _WIN32
#  include <windows.h>
#else
#  include <unistd.h>
#endif

#include "libstardictverify.h"
#include "libcommon.h"
#include "ifo_file.hpp"
#include "resourcewrap.hpp"

/* Terminology

Index file is a sequence of index items.
An index item consists of:
word - the key of the item;
size and offset of data block containing definition in the dictionary file.
A data block consists of a number of fields.
A field has a type specified by type identifer (one character, an ASCII letter).
*/

struct worditem_t {
	std::string word;
	guint32 offset;
	guint32 size;
};

struct fileitem_t {
	std::string filename;
	guint32 offset;
	guint32 size;
};

struct synitem_t {
	std::string word;
	guint32 index;
};

struct region_t {
	guint32 offset;
	guint32 size;
};

class i_resource_storage {
public:
	virtual bool have_file(const std::string& filename) const = 0;
};

static const char* const key_forbidden_chars = "\t\n\r";
static const char* const index_file_truncated_err = 
	"Index file is truncated, last record is truncated.\n";
static const char* const syn_file_truncated_err = 
	"Synonyms file is truncated, last record is truncated.\n";
static const char* const incorrect_data_block_size_err = 
	"Index item %s. Fields do not fit into the data block, incorrect data block size.\n";
static const char* const unknown_type_id_err =
	"Index item %s. Unknown type identifier '%c'.\n";
static const char* const empty_field_err =
	"Index item %s. Empty field in definition data block. Type ID '%c'.\n";
static const char* const invalid_utf8_field_err =
	"Index item %s. Invalid field. Type id = '%c'. Invalid utf8 string %s.\n";
static const char* const empty_word_err =
	"Empty word in index.\n";
static const char* const long_word_err =
	"Index item %s. wordlen>=256, wordlen = %d.\n";
static const char* const word_begin_space_err =
	"Warning: Index item %s. word begins with space.\n";
static const char* const word_end_space_err =
	"Warning: Index item %s. word ends with space.\n";
static const char* const word_forbidden_chars_err =
	"Warning: Index item %s. word contains forbidden characters.\n";
static const char* const word_invalid_utf8_err =
	"Index item %s. Invalid utf8 string.\n";
static const char* const wrong_word_order_err =
	"Wrong order, first word = %s, second word = %s\n";
static const char* const read_file_err =
	"Error reading file %s.\n";
static const char* const write_file_err =
	"Error writing file %s.\n";
static const char* const known_type_ids = "mtygxkwhnr";

namespace zip {
typedef ResourceWrapper<void, void, int, gzclose> gzFile;
}
enum TResult { rOK, rError, rNotFound };

static bool compare_fileitem(const fileitem_t& left, const fileitem_t& right)
{
	return 0 > strcmp(left.filename.c_str(), right.filename.c_str());
}

static bool compare_worditem_by_offset(const worditem_t* left, const worditem_t* right)
{
	return left->offset < right->offset;
}

/* Create a new temporary file. Return file name in file name encoding.
Return an empty string if file cannot be created. */
static std::string create_temp_file(void)
{
#ifdef _WIN32
	/* g_file_open_tmp does not work reliably on Windows 
	Use platform specific API here. */
	{
		UINT uRetVal   = 0;
		DWORD dwRetVal = 0;
		TCHAR szTempFileName[MAX_PATH];
		TCHAR lpTempPathBuffer[MAX_PATH];
		dwRetVal = GetTempPath(MAX_PATH, lpTempPathBuffer);
		if (dwRetVal > MAX_PATH || (dwRetVal == 0))
			return "";

		uRetVal = GetTempFileName(lpTempPathBuffer, // directory for tmp files
			TEXT("temp"),     // temp file name prefix 
			0,                // create unique name 
			szTempFileName);  // buffer for name 
		if (uRetVal == 0)
			return "";
		std::string tmp_url_utf8;
		std::string tmp_url;
		if(!windows_to_utf8(szTempFileName, tmp_url_utf8)
			|| !utf8_to_file_name(tmp_url_utf8, tmp_url))
			return "";
		FILE * f = g_fopen(tmp_url.c_str(), "wb");
		if(!f)
			return "";
		fwrite(" ", 1, 1, f);
		fclose(f);
		return tmp_url;
	}
#else
	{
		std::string tmp_url;
		gchar * buf = NULL;
		gint fd = g_file_open_tmp(NULL, &buf, NULL);
		if(fd == -1)
			return "";
		tmp_url = buf;
		g_free(buf);
		write(fd, " ", 1);
		close(fd);
		return tmp_url;
	}
#endif
}

class TempFile
{
public:
	explicit TempFile(print_info_t print_info = g_print)
		: print_info(print_info)
	{
	}
	~TempFile(void)
	{
		clear();
	}
	const std::string& create_temp_file(void)
	{
		clear();
		file_name = ::create_temp_file();
		if(file_name.empty())
			print_info("Unable to create a temporary file.\n");
		return file_name;
	}
	const std::string& get_file_name(void) const
	{
		return file_name;
	}
	void set_print_info(print_info_t print_info)
	{
		this->print_info = print_info;
	}
private:
	void clear(void)
	{
		if(!file_name.empty()) {
			if(g_remove(file_name.c_str()))
				print_info("Unable to remove temp file %s\n", file_name.c_str());
			file_name.clear();
		}
	}
	std::string file_name;
	print_info_t print_info;
};

static int unpack(const char* arch_file_name, const char* out_file_name, print_info_t print_info)
{
	zip::gzFile in(gzopen(arch_file_name, "rb"));
	if(!in) {
		print_info("Unable to open archive file %s\n", arch_file_name);
		return EXIT_FAILURE;
	}
	const size_t buffer_size = 1024*1024;
	std::vector<char> buffer(buffer_size);
	char* buf = &buffer[0];
	gulong len;
	clib::File out_file(g_fopen(out_file_name, "wb"));
	if(!out_file) {
		print_info("Unable to open file %s for writing\n", out_file_name);
		return EXIT_FAILURE;
	}
	while(true) {
		len = gzread(get_impl(in), buf, buffer_size);
		if(len < 0) {
			print_info(read_file_err, arch_file_name);
			return EXIT_FAILURE;
		}
		if(len == 0)
			break;
		if(1 != fwrite(buf, len, 1, get_impl(out_file))) {
			print_info(write_file_err, out_file_name);
			return EXIT_FAILURE;
		}
	}
	return EXIT_SUCCESS;
}

class dictionary_data_block {
public:
	int verify(const char* const data, size_t data_size, 
		const std::string& sametypesequence, print_info_t print_info,
		const char* word,
		i_resource_storage* p_res_storage)
	{
		if(data_size == 0) {
			print_info("Index item %s. data size = 0.\n", word);
			return EXIT_FAILURE;
		}
		this->word = word;
		this->print_info = print_info;
		this->p_res_storage = p_res_storage;
		if (!sametypesequence.empty()) {
			return verify_sametypesequence(data, data_size, 
				sametypesequence);
		} else {
			return verify_no_sametypesequence(data, data_size);
		}
	}
private:
	const char* word;
	print_info_t print_info;
	i_resource_storage* p_res_storage;

	int verify_no_sametypesequence(const char* const data, size_t data_size);
	int verify_sametypesequence(const char* const data, size_t data_size, 
		const std::string& sametypesequence);
	int verify_field(const char type_id, 
		const char*& p, size_t size_remain);
	int verify_field_content(const char type_id, const char* data, guint32 size);
	int verify_field_content_x(const char type_id, const char* data, guint32 size);
};


int dictionary_data_block::verify_sametypesequence(const char* const data, size_t data_size, 
	const std::string& sametypesequence)
{
	const char* p = data;
	size_t size_remain; // to the end of the data block
	for (size_t i=0; i<sametypesequence.length()-1; i++) {
		size_remain = data_size - (p - data);
		const char type_id = sametypesequence[i];
		if(EXIT_FAILURE == verify_field(type_id, p, size_remain))
			return EXIT_FAILURE;
	}
	// last item
	size_remain = data_size - (p - data);
	const char type_id = sametypesequence[sametypesequence.length()-1];
	if(g_ascii_isupper(type_id)) {
		guint32 size = size_remain;
		if(size == 0) {
			print_info(empty_field_err, word, type_id);
		} else
			verify_field_content(type_id, p, size);
	} else if(g_ascii_islower(type_id)) {
		if(size_remain == 0) {
			print_info(empty_field_err, word, type_id);
		} else {
			const char* p2 = reinterpret_cast<const char*>(memchr(p, '\0', size_remain));
			if(p2) {
				// '\0' found in the last record
				print_info(incorrect_data_block_size_err, word);
				return EXIT_FAILURE;
			}
			if (!g_utf8_validate(p, size_remain, NULL)) {
				std::string tmp(p, size_remain);
				print_info(invalid_utf8_field_err, word, type_id, tmp.c_str());
				return EXIT_FAILURE;
			}
			verify_field_content(type_id, p, size_remain);
		}
	} else {
		print_info(unknown_type_id_err, word, type_id);
		return EXIT_FAILURE;
	}
	if(!strchr(known_type_ids, type_id)) {
		print_info(unknown_type_id_err, word, type_id);
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}

int dictionary_data_block::verify_no_sametypesequence(const char* const data, size_t data_size)
{
	const char* p = data;
	size_t size_remain; // to the end of the data block
	while(true) {
		size_remain = data_size - (p - data);
		if(size_remain == 0)
			return EXIT_SUCCESS;
		const char type_id = *p;
		++p;
		--size_remain;
		if(EXIT_FAILURE == verify_field(type_id, p, size_remain))
			return EXIT_FAILURE;
	}
	g_assert_not_reached();
	return EXIT_SUCCESS;
}

int dictionary_data_block::verify_field(const char type_id, 
	const char*& p, const size_t size_remain)
{
	if(g_ascii_isupper(type_id)) {
		if(size_remain < sizeof(guint32)) {
			print_info(incorrect_data_block_size_err, word);
			return EXIT_FAILURE;
		}
		guint32 size = g_ntohl(*reinterpret_cast<const guint32 *>(p));
		if(size_remain < sizeof(guint32) + size) {
			print_info(incorrect_data_block_size_err, word);
			return EXIT_FAILURE;
		}
		if(size == 0) {
			print_info(empty_field_err, word, type_id);
		}
		p += sizeof(guint32);
		if(size > 0)
			verify_field_content(type_id, p, size);
		p += size;
	} else if(g_ascii_islower(type_id)) {
		if(size_remain < 1) { // data must contain at least '\0'
			print_info(incorrect_data_block_size_err, word);
			return EXIT_FAILURE;
		}
		const char* p2 = reinterpret_cast<const char*>(memchr(p, '\0', size_remain));
		if(!p2) {
			print_info(incorrect_data_block_size_err, word);
			return EXIT_FAILURE;
		}
		int datalen = p2 - p;
		if(datalen == 0) {
			print_info(empty_field_err, word, type_id);
		} else {
			if (!g_utf8_validate(p, datalen, NULL)) {
				print_info(invalid_utf8_field_err, word, type_id, p);
				return EXIT_FAILURE;
			}
			verify_field_content(type_id, p, datalen);
		}
		p = p2 + 1;
	} else {
		print_info(unknown_type_id_err, word, type_id);
		return EXIT_FAILURE;
	}
	if(!strchr(known_type_ids, type_id)) {
		print_info(unknown_type_id_err, word, type_id);
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}

int dictionary_data_block::verify_field_content(const char type_id, const char* data, guint32 size)
{
	if(type_id == 'x')
		return verify_field_content_x(type_id, data, size);
	return EXIT_SUCCESS;
}

int dictionary_data_block::verify_field_content_x(const char type_id, const char* data, guint32 size)
{
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
		if(!p_res_storage->have_file(key)) {
			print_info("Warning: Index item %s. Type id '%c'. The field refers to resource \"%s\", "
				"that is not found in resource storage.\n",
				word, type_id, key.c_str());
			have_errors = true;
		}
		p = tag + sizeof("</rref>") - 1;
	}
	return have_errors ? EXIT_FAILURE : EXIT_SUCCESS;
}

class resource_database
{
public:
	TResult load(const std::string& dirname, print_info_t print_info);
	// filename uses database directory separator
	bool have_file(const std::string& filename) const;
private:
	int prepare_ridx_file(void);
	int prepare_rdic_file(void);
	int load_rifo_file(void);
	int load_ridx_file(void);
	int load_rdic_file(void);
	void print_index(void);

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
};

TResult resource_database::load(const std::string& dirname,
	print_info_t print_info)
{
	this->print_info = print_info;
	this->dirname = dirname;
	ridxtemp.set_print_info(print_info);
	rdictemp.set_print_info(print_info);

	rifofilename = dirname + G_DIR_SEPARATOR_S "res.rifo";
	if(!g_file_test(rifofilename.c_str(), G_FILE_TEST_EXISTS))
		return rNotFound;

	if(EXIT_FAILURE == load_rifo_file())
		return rError;
	if(EXIT_FAILURE == load_ridx_file())
		return rError;
	if(EXIT_FAILURE == load_rdic_file())
		return rError;
	return rOK;
}

int resource_database::prepare_ridx_file(void)
{
	ridxfilename_orig = dirname + G_DIR_SEPARATOR_S "res.ridx.gz";
	if(g_file_test(ridxfilename_orig.c_str(), G_FILE_TEST_EXISTS)) {
		ridxfilename = ridxtemp.create_temp_file();
		if(ridxfilename.empty())
			return EXIT_FAILURE;
		if(EXIT_FAILURE == unpack(ridxfilename_orig.c_str(), ridxfilename.c_str(), print_info))
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
		if(EXIT_FAILURE == unpack(rdicfilename_orig.c_str(), rdicfilename.c_str(), print_info))
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
	if(dict_info.filecount == 0) {
		print_info("Resource database %s. filecount = 0.\n", rifofilename.c_str());
		have_errors = true;
	}
	if(dict_info.index_file_size == 0) {
		print_info("Resource database %s. ridxfilesize = 0.\n", rifofilename.c_str());
		have_errors = true;
	}
	return have_errors ? EXIT_FAILURE : EXIT_SUCCESS;
}

int resource_database::load_ridx_file(void)
{
	if(EXIT_FAILURE == prepare_ridx_file())
		return EXIT_FAILURE;

	struct stat stats;
	if (g_stat (ridxfilename.c_str(), &stats) == -1) {
		print_info("Unable to find index file %s\n", ridxfilename.c_str());
		return EXIT_FAILURE;
	}
	print_info("Verifying resource index file: %s\n", ridxfilename_orig.c_str());
	if (dict_info.index_file_size!=(guint)stats.st_size) {
		print_info("Incorrect size of the index file: in .rifo file, ridxfilesize=%d, "
			"real file size is %ld\n",
			dict_info.index_file_size, (long) stats.st_size);
		return EXIT_FAILURE;
	}

	index.clear();
	index.reserve(dict_info.filecount);

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

	if (filecount!=dict_info.filecount) {
		print_info("Incorrect number of files: in .rifo file, filecount=%d, "
			"while the real file count is %d\n", dict_info.filecount, filecount);
		have_errors=true;
	}

	return have_errors ? EXIT_FAILURE : EXIT_SUCCESS;
}

int resource_database::load_rdic_file(void)
{
	if(EXIT_FAILURE == prepare_rdic_file())
		return EXIT_FAILURE;

	struct stat stats;
	if (g_stat (rdicfilename.c_str(), &stats) == -1) {
		print_info("Unable to find resource dictionary file %s\n", rdicfilename.c_str());
		return EXIT_FAILURE;
	}
	const guint32 filesize = stats.st_size;
	
	print_info("Verifying resource dictionary file: %s\n", rdicfilename_orig.c_str());
	clib::File rdicfile(g_fopen(rdicfilename.c_str(), "rb"));
	if(!rdicfile) {
		print_info("Unable open resource dictionary file %s.\n", rdicfilename.c_str());
		return EXIT_FAILURE;
	}

	bool have_errors = false;
	for(size_t i=0; i<index.size(); ++i) {
		if(index[i].offset + index[i].size > filesize) {
			print_info("Index item %s. Incorrect size, offset parameters. "
				"Referenced data block is outside dictionary file.\n", index[i].filename.c_str());
			have_errors = true;
			continue;
		}
	}
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

class resource_files
{
public:
	TResult load(const std::string& dirname, print_info_t print_info);
	// filename uses database directory separator
	bool have_file(const std::string& filename) const;
private:
	std::string dirname;
	std::string resdirname;
	print_info_t print_info;
};

TResult resource_files::load(const std::string& dirname, print_info_t print_info)
{
	this->dirname = dirname;
	this->print_info = print_info;
	resdirname = dirname + "/res";
	if(!g_file_test(resdirname.c_str(), G_FILE_TEST_IS_DIR))
		return rNotFound;
	return rOK;
}

bool resource_files::have_file(const std::string& filename) const
{
	const std::string full_fs_filename(resdirname + "/" + dir_separator_db_to_fs(filename));
	return static_cast<bool>(g_file_test(full_fs_filename.c_str(), G_FILE_TEST_IS_REGULAR));
}

class resource_storage: public i_resource_storage
{
public:
	resource_storage(void)
	: storage_type(sNone)
	{
	}
	TResult load(const std::string& dirname, print_info_t print_info);
	// filename uses database directory separator
	bool have_file(const std::string& filename) const;
private:
	enum { sDB, sFiles, sNone } storage_type;
	resource_database db;
	resource_files files;
};

TResult resource_storage::load(const std::string& dirname, print_info_t print_info)
{
	storage_type = sNone;
	TResult res = db.load(dirname, print_info);
	if(res == rOK) {
		storage_type = sDB;
		print_info("Resource storage loaded. Type - database.\n");
		return rOK;
	}
	if(res == rError) {
		print_info("Resource storage load failed. Type - database.\n");
		return rError;
	}
	res = files.load(dirname, print_info);
	if(res == rOK) {
		storage_type = sFiles;
		print_info("Resource storage loaded. Type - files.\n");
		return rOK;
	}
	if(res == rError) {
		print_info("Resource storage load failed. Type - files.\n");
		return rError;
	}
	return res;
}

bool resource_storage::have_file(const std::string& filename) const
{
	if(storage_type == sDB)
		return db.have_file(filename);
	if(storage_type == sFiles)
		return files.have_file(filename);
	return false;
}

class norm_dict
{
public:
	int load(const std::string& ifofilename, print_info_t print_info,
			i_resource_storage* p_res_storage);
private:
	int prepare_idx_file(void);
	int prepare_dict_file(void);
	int load_ifo_file(void);
	int load_idx_file(void);
	int load_syn_file(void);
	int load_dict_file(void);
	void verify_data_blocks_overlapping(void);

	std::string basefilename;
	std::string ifofilename;
	std::string idxfilename; // may be archive
	std::string idxfilename_orig; // file to read, uncompressed
	std::string dictfilename;
	std::string dictfilename_orig;
	std::string synfilename;
	/* This dictionary has a syn file. */
	bool have_synfile;
	DictInfo dict_info;
	TempFile idxtemp;
	TempFile dicttemp;
	guint32 dictfilesize;
	std::vector<worditem_t> index;
	std::vector<synitem_t> synindex;
	print_info_t print_info;
	i_resource_storage* p_res_storage;
};

int norm_dict::load(const std::string& ifofilename, print_info_t print_info,
	i_resource_storage* p_res_storage)
{
	this->ifofilename = ifofilename;
	this->print_info = print_info;
	this->p_res_storage = p_res_storage;

	basefilename.assign(ifofilename, 0, ifofilename.length()-4);
	if(EXIT_FAILURE == load_ifo_file())
		return EXIT_FAILURE;
	if(EXIT_FAILURE == load_idx_file())
		return EXIT_FAILURE;
	if(EXIT_FAILURE == load_syn_file())
		return EXIT_FAILURE;
	if(EXIT_FAILURE == load_dict_file())
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}

int norm_dict::prepare_idx_file(void)
{
	idxfilename_orig=basefilename + ".idx.gz";
	if(g_file_test(idxfilename_orig.c_str(), G_FILE_TEST_EXISTS)) {
		idxfilename = idxtemp.create_temp_file();
		if(idxfilename.empty())
			return EXIT_FAILURE;
		if(EXIT_FAILURE == unpack(idxfilename_orig.c_str(), idxfilename.c_str(), print_info))
			return EXIT_FAILURE;
	} else {
		idxfilename_orig = basefilename + ".idx";
		idxfilename = idxfilename_orig;
	}
	return EXIT_SUCCESS;
}

int norm_dict::prepare_dict_file(void)
{
	dictfilename_orig=basefilename + ".dict.dz";
	if(g_file_test(dictfilename_orig.c_str(), G_FILE_TEST_EXISTS)) {
		dictfilename = dicttemp.create_temp_file();
		if(dictfilename.empty())
			return EXIT_FAILURE;
		if(EXIT_FAILURE == unpack(dictfilename_orig.c_str(), dictfilename.c_str(), print_info))
			return EXIT_FAILURE;
	} else {
		dictfilename_orig = basefilename + ".dict";
		dictfilename = dictfilename_orig;
	}
	return EXIT_SUCCESS;
}

int norm_dict::load_ifo_file(void)
{
	dict_info.set_print_info(print_info);
	if(!dict_info.load_from_ifo_file(ifofilename, DictInfoType_NormDict))
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}

int norm_dict::load_idx_file(void)
{
	if(EXIT_FAILURE == prepare_idx_file())
		return EXIT_FAILURE;

	struct stat stats;
	if (g_stat (idxfilename.c_str(), &stats) == -1) {
		print_info("Unable to find index file %s\n", idxfilename.c_str());
		return EXIT_FAILURE;
	}
	print_info("Verifying index file: %s\n", idxfilename_orig.c_str());
	if (dict_info.index_file_size!=(guint)stats.st_size) {
		print_info("Incorrect size of the index file: in .ifo file, idxfilesize=%d, "
			"real file size is %ld\n",
			dict_info.index_file_size, (long) stats.st_size);
		return EXIT_FAILURE;
	}

	index.clear();
	index.reserve(dict_info.wordcount);

	std::vector<gchar> buf(stats.st_size+1);
	gchar * const buffer_beg = &buf[0];
	gchar * const buffer_end = buffer_beg+stats.st_size;
	{
		FILE *idxfile = g_fopen(idxfilename.c_str(),"rb");
		fread(buffer_beg, 1, stats.st_size, idxfile);
		fclose(idxfile);
	}

	gchar *p=buffer_beg;
	gchar *preword=NULL;
	int wordlen;
	gint cmpvalue;
	guint wordcount=0;
	bool have_errors=false;
	worditem_t worditem;
	size_t size_remain; // to the end of the index file

	while (p < buffer_end) {
		size_remain = buffer_end - p;
		const char* p2 = reinterpret_cast<const char*>(memchr(p, '\0', size_remain));
		if(!p2) {
			print_info(index_file_truncated_err);
			have_errors=true;
			break;
		}
		wordlen = p2 - p;
		if (wordlen==0) {
			print_info(empty_word_err);
			have_errors=true;
		} else {
			if (wordlen>=256) {
				print_info(long_word_err, p, wordlen);
				have_errors=true;
			}
			if (g_ascii_isspace(*p)) {
				print_info(word_begin_space_err, p);
			}
			if (g_ascii_isspace(*(p+wordlen-1))) {
				print_info(word_end_space_err, p);
			}
		}
		if (strpbrk(p, key_forbidden_chars)) {
			print_info(word_forbidden_chars_err, p);
		}
		if (!g_utf8_validate(p, wordlen, NULL)) {
			print_info(word_invalid_utf8_err, p);
			have_errors=true;
		}
		if (preword) {
			cmpvalue=stardict_strcmp(preword, p);
			if (cmpvalue>0) {
				print_info(wrong_word_order_err, preword, p);
				have_errors=true;
			}
		}
		preword=p;
		worditem.word = p;
		p += wordlen + 1;
		size_remain = buffer_end - p;
		if(size_remain < 2 * sizeof(guint32)) {
			print_info(index_file_truncated_err);
			have_errors=true;
			break;
		}
		worditem.offset = g_ntohl(*reinterpret_cast<guint32 *>(p));
		p += sizeof(guint32);
		worditem.size = g_ntohl(*reinterpret_cast<guint32 *>(p));
		p += sizeof(guint32);
		if (worditem.size==0) {
			print_info("Index item %s. Data block size = 0.\n", preword);
		}
		wordcount++;
		index.push_back(worditem);
	} // while
	
	g_assert(p <= buffer_end);

	if (wordcount!=dict_info.wordcount) {
		print_info("Incorrect number of words: in .ifo file, wordcount=%d, "
			"while the real word count is %d\n", dict_info.wordcount, wordcount);
		have_errors=true;
	}

	for(size_t i=0; i < index.size(); ++i) {
		for(size_t j=i+1; j < index.size() && index[i].word == index[j].word; ++j) {
			if(index[i].offset == index[j].offset && index[i].size == index[j].size) {
				print_info("Multiple index items have the same word = %s, offset = %d, size = %d\n",
					index[i].word.c_str(), index[i].offset, index[i].size);
				break;
			}
		}
	}

	return have_errors ? EXIT_FAILURE : EXIT_SUCCESS;
}

int norm_dict::load_syn_file(void)
{
	synfilename = basefilename + ".syn";
	have_synfile = dict_info.synwordcount != 0;

	if (dict_info.synwordcount == 0) {
		struct stat stats;
		if (g_stat (synfilename.c_str(), &stats) != -1) {
			print_info(".syn file exists but no \"synwordcount=\" entry in .ifo file\n");
			return EXIT_FAILURE;
		}
		return EXIT_SUCCESS;
	}

	struct stat stats;
	if (g_stat (synfilename.c_str(), &stats) == -1) {
		print_info("Unable to find synonyms file %s\n", synfilename.c_str());
		return EXIT_FAILURE;
	}
	print_info("Verifying synonyms file: %s\n", synfilename.c_str());

	synindex.clear();
	synindex.reserve(dict_info.synwordcount);

	std::vector<gchar> buf(stats.st_size+1);
	gchar *buffer_begin = &buf[0];
	gchar *buffer_end = buffer_begin+stats.st_size;
	{
		FILE *synfile = g_fopen(synfilename.c_str(),"rb");
		fread (buffer_begin, 1, stats.st_size, synfile);
		fclose (synfile);
	}

	gchar *p=buffer_begin;
	gchar *preword=NULL;
	int wordlen;
	gint cmpvalue;
	guint wordcount=0;
	bool have_errors=false;
	synitem_t synitem;
	size_t size_remain; // to the end of the synonyms file

	while (p < buffer_end) {
		size_remain = buffer_end - p;
		const char* p2 = reinterpret_cast<const char*>(memchr(p, '\0', size_remain));
		if(!p2) {
			print_info(syn_file_truncated_err);
			have_errors=true;
			break;
		}
		wordlen = p2 - p;
		if (wordlen==0) {
			print_info(empty_word_err);
			have_errors=true;
		} else {
			if (wordlen>=256) {
				print_info(long_word_err, p, wordlen);
				have_errors=true;
			}
			if (g_ascii_isspace(*p)) {
				print_info(word_begin_space_err, p);
			}
			if (g_ascii_isspace(*(p+wordlen-1))) {
				print_info(word_end_space_err, p);
			}
		}
		if (strpbrk(p, key_forbidden_chars)) {
			print_info(word_forbidden_chars_err, p);
		}
		if (!g_utf8_validate(p, wordlen, NULL)) {
			print_info(word_invalid_utf8_err, p);
			have_errors=true;
		}
		if (preword) {
			cmpvalue=stardict_strcmp(preword, p);
			if (cmpvalue>0) {
				print_info(wrong_word_order_err, preword, p);
				have_errors=true;
			}
		}
		preword=p;
		synitem.word = p;
		p += wordlen +1;
		size_remain = buffer_end - p;
		if(size_remain < sizeof(guint32)) {
			print_info(syn_file_truncated_err);
			have_errors=true;
			break;
		}
		synitem.index = g_ntohl(*reinterpret_cast<guint32 *>(p));
		if (synitem.index>=dict_info.wordcount) {
			print_info("Index item %s. wrong index %d.\n", preword, synitem.index);
			have_errors=true;
		}
		p+=sizeof(guint32);
		wordcount++;
		synindex.push_back(synitem);
	} // while

	g_assert(p <= buffer_end);

	if (wordcount!=dict_info.synwordcount) {
		print_info("Incorrect number of words: in .ifo file, synwordcount=%d, "
			"while the real synwordcount is %d\n",
			dict_info.synwordcount, wordcount);
		have_errors=true;
	}

	for(size_t i=0; i < synindex.size(); ++i) {
		for(size_t j=i+1; j < synindex.size() && synindex[i].word == synindex[j].word; ++j) {
			if(synindex[i].index == synindex[j].index) {
				print_info("Multiple synonym items with the same word = %s, index =%d\n", 
					synindex[i].word.c_str(), synindex[i].index);
				break;
			}
		}
	}

	return have_errors ? EXIT_FAILURE : EXIT_SUCCESS;
}

int norm_dict::load_dict_file(void)
{
	if(EXIT_FAILURE == prepare_dict_file())
		return EXIT_FAILURE;

	struct stat stats;
	if (g_stat (dictfilename.c_str(), &stats) == -1) {
		print_info("Dictionary file does not exist: %s\n", dictfilename.c_str());
		return EXIT_FAILURE;
	}
	dictfilesize = stats.st_size;

	print_info("Verifying dictionary file: %s\n", dictfilename_orig.c_str());
	clib::File dictfile(g_fopen(dictfilename.c_str(), "rb"));
	if(!dictfile) {
		print_info("Unable open dictionary file %s.\n", dictfilename.c_str());
		return EXIT_FAILURE;
	}

	bool have_errors = false;
	std::vector<char> buffer;
	dictionary_data_block block_verifier;
	for(size_t i=0; i<index.size(); ++i) {
		if(index[i].offset + index[i].size > dictfilesize) {
			print_info("Index item %s. Incorrect size, offset parameters. "
				"Referenced data block is outside dictionary file.\n", index[i].word.c_str());
			have_errors = true;
			continue;
		}
		buffer.resize(index[i].size);
		if(fseek(get_impl(dictfile), index[i].offset, SEEK_SET)) {
			print_info(read_file_err, dictfilename.c_str());
			return EXIT_FAILURE;
		}
		if(1 != fread(&buffer[0], index[i].size, 1, get_impl(dictfile))) {
			print_info(read_file_err, dictfilename.c_str());
			return EXIT_FAILURE;
		}
		block_verifier.verify(&buffer[0], index[i].size, dict_info.sametypesequence, 
			print_info, index[i].word.c_str(), p_res_storage);
	}
	verify_data_blocks_overlapping();
	return have_errors ? EXIT_FAILURE : EXIT_SUCCESS;
}

void norm_dict::verify_data_blocks_overlapping(void)
{
	std::vector<const worditem_t*> sort_index(index.size(), NULL);
	for(size_t i=0; i<index.size(); ++i)
		sort_index[i] = &index[i];
	std::sort(sort_index.begin(), sort_index.end(), compare_worditem_by_offset);
	// find overlapping but not equal regions (offset, size)
	for(size_t i=0; i<sort_index.size(); ++i) {
		for(size_t j=i+1; j<sort_index.size()
			&& sort_index[i]->offset + sort_index[i]->size > sort_index[j]->offset; ++j) {
				if(sort_index[i]->offset == sort_index[j]->offset
					&& sort_index[i]->size == sort_index[j]->size)
					continue;
				print_info("Warning: Index item %s and index item %s refer to overlapping "
					"but not equal regions (offset, size): "
					"(%u, %u) and (%u, %u)\n",
					sort_index[i]->word.c_str(), sort_index[j]->word.c_str(),
					sort_index[i]->offset);
		}
	}
	// find not used regions
	std::vector<region_t> unused_regions;
	region_t region;
	guint32 low_boundary=0;
	for(size_t i=0; i<sort_index.size(); ++i) {
		if(sort_index[i]->offset < low_boundary)
			continue;
		if(sort_index[i]->offset == low_boundary) {
			low_boundary = sort_index[i]->offset + sort_index[i]->size;
			continue;
		}
		region.offset = low_boundary;
		region.size = sort_index[i]->offset - low_boundary;
		unused_regions.push_back(region);
		low_boundary = sort_index[i]->offset + sort_index[i]->size;
	}
	if(low_boundary < dictfilesize) {
		region.offset = low_boundary;
		region.size = dictfilesize - low_boundary;
		unused_regions.push_back(region);
	}
	if(!unused_regions.empty()) {
		print_info("Warning: Dictionary contains unreferenced blocks (offset, size):\n");
		for(size_t i = 0; i<unused_regions.size(); ++i)
			print_info("\t(%u, %u)\n", unused_regions[i].offset, unused_regions[i].size);
	}
}

int stardict_verify(const char *ifofilename, print_info_t print_info)
{
	bool have_errors = false;

	glib::CharStr cdirname(g_path_get_dirname(ifofilename));
	resource_storage res_storage;
	if(rError == res_storage.load(get_impl(cdirname), print_info))
		have_errors = true;

	norm_dict dict;
	if(EXIT_FAILURE == dict.load(ifofilename, print_info, static_cast<i_resource_storage*>(&res_storage)))
		have_errors = true;

	return have_errors ? EXIT_FAILURE : EXIT_SUCCESS;
}
