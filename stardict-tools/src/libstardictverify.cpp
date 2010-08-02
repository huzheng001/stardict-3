#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <cstring>
#include <cstdlib>
#include <glib/gstdio.h>
#include <glib.h>
#include <string>
#include <vector>
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

struct worditem_t {
	std::string word;
	guint32 offset;
	guint32 size;
};

struct synitem_t {
	std::string word;
	guint32 index;
};

static const char* const key_forbidden_chars = "\t\n\r";

namespace zip {
typedef ResourceWrapper<void, void, int, gzclose> gzFile;
}

/* Create a new temporary file. Return file name is in file name encoding.
Return an empty string if file cannot be created. */
static std::string create_temp_file(void)
{
#ifdef _WIN32
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
	explicit TempFile(print_info_t print_info)
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
private:
	void clear(void)
	{
		if(!file_name.empty()) {
			if(g_remove(file_name.c_str()))
				print_info("Error: Unable to remove temp file %s\n", file_name.c_str());
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
			print_info("Error reading archive file %s\n", arch_file_name);
			return EXIT_FAILURE;
		}
		if(len == 0)
			break;
		if(1 != fwrite(buf, len, 1, get_impl(out_file))) {
			print_info("Error writing to %s\n", out_file_name);
			return EXIT_FAILURE;
		}
	}
	return EXIT_SUCCESS;
}

static int verify_synfile(const gchar *synfilename, guint synwordcount, guint idxwordcount,
													print_info_t print_info, std::vector<synitem_t>& synindex)
{
	struct stat stats;
	if (g_stat (synfilename, &stats) == -1) {
		print_info("Synonyms file does not exist: %s\n", synfilename);
		return EXIT_FAILURE;
	}
	print_info("Verifing synonyms file: %s\n", synfilename);

	synindex.clear();
	synindex.reserve(synwordcount);

	std::vector<gchar> buf(stats.st_size+1);
	gchar *buffer_begin = &buf[0];
	gchar *buffer_end = buffer_begin+stats.st_size;
	FILE *synfile = g_fopen(synfilename,"rb");
	fread (buffer_begin, 1, stats.st_size, synfile);
	fclose (synfile);

	gchar *p=buffer_begin;
	gchar *preword=NULL;
	int wordlen;
	gint cmpvalue;
	guint32 index;
	guint wordcount=0;
	bool have_errors=false;
	synitem_t synitem;

	while (p < buffer_end) {
		wordlen=strlen(p);
		if (wordlen==0) {
			print_info("Error: wordlen==0\n");
		} else {
			if (wordlen>=256) {
				print_info("Error: wordlen>=256, %s\n", p);
			}
			if (g_ascii_isspace(*p)) {
				print_info("Warning: begin with space, %s\n", p);
			}
			if (g_ascii_isspace(*(p+wordlen-1))) {
				print_info("Warning: end with space, %s\n", p);
			}
		}
		if (strpbrk(p, key_forbidden_chars)) {
			print_info("Warning: contain invalid character, %s\n", p);
		}
		if (!g_utf8_validate(p, wordlen, NULL)) {
			print_info("Error: invalid utf8 string, %s\n", p);
			have_errors=true;
		}
		if (preword) {
			cmpvalue=stardict_strcmp(preword, p);
			if (cmpvalue>0) {
				print_info("Error: wrong string order, first word = %s, second word = %s\n", preword, p);
				have_errors=true;
			}
		}
		preword=p;
		synitem.word = p;
		p += wordlen +1;
		synitem.index = index = g_ntohl(*reinterpret_cast<guint32 *>(p));
		if (index>=idxwordcount) {
			print_info("Error: index is wrong, %s\n", preword);
			have_errors=true;
		}
		p+=sizeof(guint32);
		wordcount++;
		synindex.push_back(synitem);
	} // while

	if(p > buffer_end) {
		print_info("Error: synonyms file is truncated, last record is truncated.\n");
		have_errors=true;
	}

	if (wordcount!=synwordcount) {
		print_info("Error: in .ifo file, synwordcount=%d, while the real synwordcount is %d\n", synwordcount, wordcount);
		have_errors=true;
	}

	for(size_t i=0; i < synindex.size(); ++i) {
		for(size_t j=i+1; j < synindex.size() && synindex[i].word == synindex[j].word; ++j) {
			if(synindex[i].index == synindex[j].index) {
				print_info("Multiple synonym items with word = %s, index =%d\n", 
					synindex[i].word.c_str(), synindex[i].index);
				break;
			}
		}
	}

	return have_errors ? EXIT_FAILURE : EXIT_SUCCESS;
}

static int verify_idxfile(const gchar *idxfilename_orig, const gchar *idxfilename, guint ifo_wordcount,
	guint ifo_index_file_size, print_info_t print_info,
	std::vector<worditem_t>& index)
{
	struct stat stats;
	if (g_stat (idxfilename, &stats) == -1) {
		print_info("Index file does not exist: %s\n", idxfilename);
		return EXIT_FAILURE;
	}
	print_info("Verifing index file: %s\n", idxfilename_orig);
	if (ifo_index_file_size!=(guint)stats.st_size) {
		print_info("Incorrect size of the index file: in .ifo file, idxfilesize=%d, "
			"real file size is %ld\n",
			ifo_index_file_size, (long) stats.st_size);
		return EXIT_FAILURE;
	}

	index.clear();
	index.reserve(ifo_wordcount);

	std::vector<gchar> buf(stats.st_size+1);
	gchar *buffer_beg = &buf[0];
	gchar *buffer_end = buffer_beg+stats.st_size;
	{
		FILE *idxfile = g_fopen(idxfilename,"rb");
		fread(buffer_beg, 1, stats.st_size, idxfile);
		fclose(idxfile);
	}

	gchar *p=buffer_beg;
	gchar *preword=NULL;
	int wordlen;
	gint cmpvalue;
	guint32 size, offset;
	guint wordcount=0;
	bool have_errors=false;
	worditem_t worditem;

	while (p < buffer_end) {
		wordlen=strlen(p);
		if (wordlen==0) {
			print_info("Error: wordlen==0\n");
		} else {
			if (wordlen>=256) {
				print_info("Error: wordlen>=256, wordlen = %d, word = %s\n", wordlen, p);
			}
			if (g_ascii_isspace(*p)) {
				print_info("Warning: begin with space, %s\n", p);
			}
			if (g_ascii_isspace(*(p+wordlen-1))) {
				print_info("Warning: end with space, %s\n", p);
			}
		}
		if (strpbrk(p, key_forbidden_chars)) {
			print_info("Warning: contain forbidden characters, %s\n", p);
		}
		if (!g_utf8_validate(p, wordlen, NULL)) {
			print_info("Error: invalid utf8 string, %s\n", p);
			have_errors=true;
		}
		if (preword) {
			cmpvalue=stardict_strcmp(preword, p);
			if (cmpvalue>0) {
				print_info("Error: wrong string order, first word = %s, second word = %s\n", preword, p);
				have_errors=true;
			}
		}
		preword=p;
		worditem.word = p;
		p += wordlen + 1;
		worditem.offset = offset = g_ntohl(*reinterpret_cast<guint32 *>(p));
		p += sizeof(guint32);
		worditem.size = size = g_ntohl(*reinterpret_cast<guint32 *>(p));
		p += sizeof(guint32);
		if (size==0) {
			print_info("Error: definition size==0, %s\n", preword);
		}
		wordcount++;
		index.push_back(worditem);
	} // while
	
	if(p > buffer_end) {
		print_info("Error: index file is truncated, last record is truncated.\n");
		have_errors=true;
	}

	if (wordcount!=ifo_wordcount) {
		print_info("Error: incorrect number of words: in .ifo file, wordcount=%d, "
			"while the real word count is %d\n", ifo_wordcount, wordcount);
		have_errors=true;
	}

	for(size_t i=0; i < index.size(); ++i) {
		for(size_t j=i+1; j < index.size() && index[i].word == index[j].word; ++j) {
			if(index[i].offset == index[j].offset && index[i].size == index[j].size) {
				print_info("Multiple index items with word = %s, offset = %d, size = %d\n",
					index[i].word.c_str(), index[i].offset, index[i].size);
				break;
			}
		}
	}

	return have_errors ? EXIT_FAILURE : EXIT_SUCCESS;
}

int stardict_verify(const char *ifofilename, print_info_t print_info)
{
	DictInfo dict_info;
	dict_info.set_print_info(print_info);
	if(!dict_info.load_from_ifo_file(ifofilename, DictInfoType_NormDict))
		return EXIT_FAILURE;

	guint wordcount = dict_info.wordcount;
	guint synwordcount=dict_info.synwordcount;
	guint index_file_size = dict_info.index_file_size;

	bool have_errors = false;
	std::vector<worditem_t> index;
	const std::string basefilename(ifofilename, strlen(ifofilename)-4);
	std::string idxfilename_orig; // may be archive
	std::string idxfilename; // file to read, uncompressed
	TempFile idxtemp(print_info);
	idxfilename_orig=basefilename + ".idx.gz";
	if(g_file_test(idxfilename_orig.c_str(), G_FILE_TEST_IS_REGULAR)) {
		idxfilename = idxtemp.create_temp_file();
		if(idxfilename.empty())
			return EXIT_FAILURE;
		if(EXIT_FAILURE == unpack(idxfilename_orig.c_str(), idxfilename.c_str(), print_info))
			return EXIT_FAILURE;
	} else {
		idxfilename_orig = basefilename + ".idx";
		idxfilename = idxfilename_orig;
	}
	int val = verify_idxfile(idxfilename_orig.c_str(), idxfilename.c_str(), 
		wordcount, index_file_size, print_info, index);
	if (val==EXIT_FAILURE)
		have_errors = true;

	const std::string synfilename=basefilename + ".syn";
	std::vector<synitem_t> synindex;
	if (synwordcount) {
		val = verify_synfile(synfilename.c_str(), synwordcount, wordcount, print_info, synindex);
		if (val==EXIT_FAILURE)
			have_errors = true;
	} else {
		struct stat stats;
		if (g_stat (synfilename.c_str(), &stats) != -1) {
			print_info("Error: .syn file exists but no \"synwordcount=\" entry in .ifo file\n");
			have_errors = true;
		}
	}
	return have_errors ? EXIT_FAILURE : EXIT_SUCCESS;
}
