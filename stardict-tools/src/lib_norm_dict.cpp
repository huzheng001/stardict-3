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
#include "lib_norm_dict.h"
#include "libstardictverify.h"

/* Limit the initially reserved index size.
 * .ifo file may contain incorrect, unreasonably large value of index size,
 * so we'd be out of memory if we try to allocate such amount. */
const size_t MAX_RESERVED_INDEX_SIZE = 200*1024;

static bool compare_worditem_by_offset(const worditem_t* left, const worditem_t* right)
{
	return left->offset < right->offset;
}

/* truncate utf8 string on char boundary (string content is not changed,
 * instead desired new length is returned)
 * new string length must be <= max_len
 * beg - first char of the string,
 * str_len - string length in bytes
 * return value: length of the truncated string */
static size_t truncate_utf8_string(const char* const beg, const size_t str_len, const size_t max_len)
{
	if(str_len <= max_len)
		return str_len;
	if(max_len == 0)
		return 0;
	const char* char_end = beg+str_len;
	const char* p = beg+str_len-1;
	while(true) {
		// find the first byte of a utf8 char
		for(; beg <= p && (*p & 0xC0) == 0x80; --p)
			;
		if(p<beg)
			return 0;
		const gunichar guch = g_utf8_get_char_validated(p, char_end-p);
		if(guch != (gunichar)-1 && guch != (gunichar)-2)
			return char_end - beg;
		char_end = p;
		--p;
		if(p<beg)
			return 0;
	}
}


norm_dict::norm_dict(void)
:
	dictfilesize(0),
	print_info(NULL),
	p_res_storage(NULL),
	fix_errors(false)
{

}

/* p_res_storage may be NULL */
int norm_dict::load(const std::string& ifofilename, print_info_t print_info,
	i_resource_storage* p_res_storage)
{
	this->ifofilename = ifofilename;
	this->print_info = print_info;
	this->p_res_storage = p_res_storage;
	if(!g_str_has_suffix(ifofilename.c_str(), ".ifo")) {
		print_info("Unsupported file type. File must have 'ifo' extension. file = %s.\n",
			ifofilename.c_str());
		return EXIT_FAILURE;
	}

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

int norm_dict::get_data_fields(guint32 offset, guint32 size, data_field_vect_t& fields) const
{
	if(size == 0)
		return EXIT_FAILURE;
	fields.clear();

	const char* word = "???";
	std::vector<char> buffer(size);

	if(!dictfile) {
		print_info("Dictionary is not loaded.\n");
		return EXIT_FAILURE;
	}
	if(fseek(get_impl(dictfile), offset, SEEK_SET)) {
		print_info(read_file_err, dictfilename.c_str());
		return EXIT_FAILURE;
	}
	if(1 != fread(&buffer[0], size, 1, get_impl(dictfile))) {
		print_info(read_file_err, dictfilename.c_str());
		return EXIT_FAILURE;
	}

	dictionary_data_block data_block;
	data_block.set_resource_storage(p_res_storage);
	data_block.set_print_info(print_info);
	data_block.set_fix_errors(fix_errors);
	return data_block.load(&buffer[0], size, dict_info.sametypesequence, word, &fields);
}

int norm_dict::prepare_idx_file(void)
{
	idxfilename_orig=basefilename + ".idx.gz";
	if(g_file_test(idxfilename_orig.c_str(), G_FILE_TEST_EXISTS)) {
		idxfilename = idxtemp.create_temp_file();
		if(idxfilename.empty())
			return EXIT_FAILURE;
		if(EXIT_FAILURE == unpack_zlib(idxfilename_orig.c_str(), idxfilename.c_str(), print_info))
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
		if(EXIT_FAILURE == unpack_zlib(dictfilename_orig.c_str(), dictfilename.c_str(), print_info))
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

	guint32 idxfilesize;
	{
		struct stat stats;
		if (g_stat (idxfilename.c_str(), &stats) == -1) {
			print_info("Unable to find index file %s\n", idxfilename.c_str());
			return EXIT_FAILURE;
		}
		idxfilesize = (guint32)stats.st_size;
	}
	print_info("Loading index file: %s\n", idxfilename_orig.c_str());

	if (dict_info.index_file_size != idxfilesize) {
		print_info("Incorrect size of the index file: in .ifo file, idxfilesize=%u, "
			"real file size is %u\n",
			dict_info.index_file_size, idxfilesize);
		if(fix_errors) {
			dict_info.index_file_size = idxfilesize;
			print_info(fixed_msg);
		} else
			return EXIT_FAILURE;
	}

	index.clear();
	index.reserve(std::min(MAX_RESERVED_INDEX_SIZE, dict_info.wordcount));

	std::vector<gchar> buf(idxfilesize+1);
	gchar * const buffer_beg = &buf[0];
	gchar * const buffer_end = buffer_beg+idxfilesize;
	{
		FILE *idxfile = g_fopen(idxfilename.c_str(),"rb");
		if(!idxfile) {
			print_info(open_read_file_err, idxfilename.c_str());
			return EXIT_FAILURE;
		}
		fread(buffer_beg, 1, idxfilesize, idxfile);
		fclose(idxfile);
	}

	const char *p=buffer_beg;
	int wordlen;
	gint cmpvalue;
	guint wordcount=0;
	bool have_errors=false;
	worditem_t worditem, preworditem;
	size_t size_remain; // to the end of the index file

	while (p < buffer_end) {
		size_remain = buffer_end - p;
		const char* const word_end = reinterpret_cast<const char*>(memchr(p, '\0', size_remain));
		if(!word_end) {
			print_info(index_file_truncated_err);
			if(fix_errors)
				print_info(fixed_ignore_file_tail_msg);
			else
				have_errors=true;
			break;
		}
		worditem.word = p;
		wordlen = worditem.word.length();
		if (!g_utf8_validate(worditem.word.c_str(), wordlen, NULL)) {
			print_info(word_invalid_utf8_err, worditem.word.c_str());
			if(fix_errors) {
				worditem.word.clear();
				wordlen = 0;
				print_info(fixed_ignore_word_msg);
			} else
				have_errors=true;
		}
		if (wordlen==0) {
			print_info(empty_word_err);
			if(fix_errors)
				print_info(fixed_ignore_word_msg);
			else
				have_errors=true;
		} else {
			if (wordlen>=MAX_INDEX_KEY_SIZE) {
				print_info(long_word_err, worditem.word.c_str(), MAX_INDEX_KEY_SIZE, wordlen);
				if(fix_errors) {
					wordlen = truncate_utf8_string(worditem.word.c_str(), wordlen, MAX_INDEX_KEY_SIZE-1);
					worditem.word.resize(wordlen);
					print_info("fixed. the word is truncated.\n");
				} else
					have_errors=true;
			}
			if (g_ascii_isspace(worditem.word[0])) {
				print_info(word_begin_space_err, worditem.word.c_str());
			}
			if (g_ascii_isspace(worditem.word[wordlen-1])) {
				print_info(word_end_space_err, worditem.word.c_str());
			}
		}
		if (strpbrk(worditem.word.c_str(), key_forbidden_chars)) {
			print_info(word_forbidden_chars_err, worditem.word.c_str());
		}
		if (!preworditem.word.empty() && !worditem.word.empty()) {
			cmpvalue=stardict_strcmp(preworditem.word.c_str(), worditem.word.c_str());
			if (cmpvalue>0) {
				print_info(wrong_word_order_err, preworditem.word.c_str(), worditem.word.c_str());
				if(fix_errors)
					print_info("fixed. Will be reordered.\n");
				else
					have_errors=true;
			}
		}
		p = word_end + 1;
		size_remain = buffer_end - p;
		if(size_remain < 2 * sizeof(guint32)) {
			print_info(index_file_truncated_err);
			if(fix_errors)
				print_info(fixed_ignore_file_tail_msg);
			else
				have_errors=true;
			break;
		}
		worditem.offset = g_ntohl(*reinterpret_cast<const guint32 *>(p));
		p += sizeof(guint32);
		worditem.size = g_ntohl(*reinterpret_cast<const guint32 *>(p));
		p += sizeof(guint32);
		if (worditem.size==0) {
			print_info("Index item %s. Data block size = 0.\n", worditem.word.c_str());
			if(fix_errors) {
				worditem.word.clear();
				print_info(fixed_ignore_word_msg);
			}
		}
		preworditem = worditem;
		wordcount++;
		index.push_back(worditem);
	} // while

	g_assert(p <= buffer_end);

	if (dict_info.wordcount != wordcount) {
		print_info("Incorrect number of words: in .ifo file, wordcount=%d, "
			"while the real word count is %d\n", dict_info.wordcount, wordcount);
		if(fix_errors) {
			dict_info.wordcount = wordcount;
			print_info(fixed_msg);
		} else
			have_errors=true;
	}

	for(size_t i=0; i < index.size(); ++i) {
		for(size_t j=i+1; j < index.size() && index[i].word == index[j].word; ++j) {
			if(index[i].offset == index[j].offset && index[i].size == index[j].size) {
				print_info("Warning. Multiple index items have the same word = %s, offset = %d, size = %d\n",
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

	if (dict_info.synwordcount == 0) {
		struct stat stats;
		if (g_stat (synfilename.c_str(), &stats) != -1) {
			print_info(".syn file exists but no \"synwordcount=\" entry in .ifo file\n");
			if(fix_errors) {
				print_info("fixed. process the .syn file.\n");
			} else
				return EXIT_FAILURE;
		} else
			return EXIT_SUCCESS;
	}

	guint32 synfilesize;
	{
		struct stat stats;
		if (g_stat (synfilename.c_str(), &stats) == -1) {
			print_info("Unable to find synonyms file %s\n", synfilename.c_str());
			if(fix_errors) {
				dict_info.synwordcount = 0;
				print_info(fixed_ignore_syn_file_msg);
				return EXIT_SUCCESS;
			} else
				return EXIT_FAILURE;
		}
		synfilesize = stats.st_size;
	}
	print_info("Loading synonyms file: %s\n", synfilename.c_str());

	synindex.clear();
	synindex.reserve(std::min(MAX_RESERVED_INDEX_SIZE, dict_info.synwordcount));

	std::vector<gchar> buf(synfilesize+1);
	gchar *buffer_begin = &buf[0];
	gchar *buffer_end = buffer_begin+synfilesize;
	{
		FILE *synfile = g_fopen(synfilename.c_str(),"rb");
		if(!synfile) {
			print_info(open_read_file_err, synfilename.c_str());
			if(fix_errors) {
				dict_info.synwordcount = 0;
				print_info(fixed_ignore_syn_file_msg);
				return EXIT_SUCCESS;
			} else
				return EXIT_FAILURE;
		}
		fread (buffer_begin, 1, synfilesize, synfile);
		fclose (synfile);
	}

	const char *p=buffer_begin;
	int wordlen;
	gint cmpvalue;
	guint wordcount=0;
	bool have_errors=false;
	synitem_t synitem, presynitem;
	size_t size_remain; // to the end of the synonyms file

	while (p < buffer_end) {
		size_remain = buffer_end - p;
		const char* const word_end = reinterpret_cast<const char*>(memchr(p, '\0', size_remain));
		if(!word_end) {
			print_info(syn_file_truncated_err);
			if(fix_errors)
				print_info(fixed_ignore_file_tail_msg);
			else
				have_errors=true;
			break;
		}
		synitem.word = p;
		wordlen = synitem.word.length();
		if (!g_utf8_validate(synitem.word.c_str(), wordlen, NULL)) {
			print_info(word_invalid_utf8_err, synitem.word.c_str());
			if(fix_errors) {
				synitem.word.clear();
				wordlen = 0;
				print_info(fixed_ignore_word_msg);
			} else
				have_errors=true;
		}
		if (wordlen==0) {
			print_info(empty_word_err);
			if(fix_errors)
				print_info(fixed_ignore_word_msg);
			else
				have_errors=true;
		} else {
			if (wordlen>=MAX_INDEX_KEY_SIZE) {
				print_info(long_word_err, synitem.word.c_str(), MAX_INDEX_KEY_SIZE, wordlen);
				if(fix_errors) {
					wordlen = truncate_utf8_string(synitem.word.c_str(), wordlen, MAX_INDEX_KEY_SIZE-1);
					synitem.word.resize(wordlen);
					print_info("fixed. the word is truncated.\n");
				} else
					have_errors=true;
			}
			if (g_ascii_isspace(synitem.word[0])) {
				print_info(word_begin_space_err, synitem.word.c_str());
			}
			if (g_ascii_isspace(synitem.word[wordlen-1])) {
				print_info(word_end_space_err, synitem.word.c_str());
			}
		}
		if (strpbrk(synitem.word.c_str(), key_forbidden_chars)) {
			print_info(word_forbidden_chars_err, synitem.word.c_str());
		}
		if (!presynitem.word.empty() && !synitem.word.empty()) {
			cmpvalue=stardict_strcmp(presynitem.word.c_str(), synitem.word.c_str());
			if (cmpvalue>0) {
				print_info(wrong_word_order_err, presynitem.word.c_str(), synitem.word.c_str());
				if(fix_errors)
					print_info("fixed. Will be reordered.\n");
				else
					have_errors=true;
			}
		}
		p = word_end +1;
		size_remain = buffer_end - p;
		if(size_remain < sizeof(guint32)) {
			print_info(syn_file_truncated_err);
			if(fix_errors)
				print_info(fixed_ignore_file_tail_msg);
			else
				have_errors=true;
			break;
		}
		synitem.index = g_ntohl(*reinterpret_cast<const guint32 *>(p));
		if (synitem.index>=dict_info.wordcount) {
			print_info("Index item %s. wrong index %d.\n", synitem.word.c_str(), synitem.index);
			if(fix_errors) {
				synitem.word.clear();
				print_info(fixed_ignore_word_msg);
			} else
				have_errors=true;
		}
		p+=sizeof(guint32);
		presynitem = synitem;
		wordcount++;
		synindex.push_back(synitem);
	} // while

	g_assert(p <= buffer_end);

	if (wordcount != dict_info.synwordcount) {
		print_info("Incorrect number of words: in .ifo file, synwordcount=%d, "
			"while the real synwordcount is %d\n",
			dict_info.synwordcount, wordcount);
		if(fix_errors) {
			dict_info.synwordcount = wordcount;
			print_info(fixed_msg);
		} else
			have_errors=true;
	}

	for(size_t i=0; i < synindex.size(); ++i) {
		for(size_t j=i+1; j < synindex.size() && synindex[i].word == synindex[j].word; ++j) {
			if(synindex[i].index == synindex[j].index) {
				print_info("Warning. Multiple synonym items with the same word = %s, index =%d\n",
					synindex[i].word.c_str(), synindex[i].index);
				break;
			}
		}
	}

	if(have_errors) {
		print_info("Loading synonyms file failed.\n");
		if(fix_errors) {
			dict_info.synwordcount = 0;
			synindex.clear();
			print_info(fixed_ignore_syn_file_msg);
			return EXIT_SUCCESS;
		} else
			return EXIT_FAILURE;
	} else
		return EXIT_SUCCESS;
}

int norm_dict::load_dict_file(void)
{
	if(prepare_dict_file())
		return EXIT_FAILURE;

	{
		struct stat stats;
		if (g_stat (dictfilename.c_str(), &stats) == -1) {
			print_info("Dictionary file does not exist: %s\n", dictfilename.c_str());
			return EXIT_FAILURE;
		}
		dictfilesize = stats.st_size;
	}

	print_info("Loading dictionary file: %s\n", dictfilename_orig.c_str());
	dictfile.reset(g_fopen(dictfilename.c_str(), "rb"));
	if(!dictfile) {
		print_info("Unable open dictionary file %s.\n", dictfilename.c_str());
		return EXIT_FAILURE;
	}

	bool have_errors = false;
	std::vector<char> buffer;
	dictionary_data_block block_verifier;
	block_verifier.set_resource_storage(p_res_storage);
	block_verifier.set_print_info(print_info);
	block_verifier.set_fix_errors(fix_errors);
	for(size_t i=0; i<index.size(); ++i) {
		if(index[i].word.empty())
			continue;
		if(index[i].offset + index[i].size > dictfilesize) {
			print_info("Index item %s. Incorrect size, offset parameters. "
				"Referenced data block is outside dictionary file.\n", index[i].word.c_str());
			if(fix_errors) {
				if(index[i].offset >= dictfilesize) {
					index[i].word.clear();
					print_info(fixed_ignore_word_msg);
					continue;
				} else {
					index[i].size = dictfilesize - index[i].offset;
					print_info("fixed. changed size of the data block.\n");
				}
			} else {
				have_errors = true;
				continue;
			}
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
		if(block_verifier.load(&buffer[0], index[i].size,
			dict_info.sametypesequence, index[i].word.c_str())) {
			if(fix_errors) {
				index[i].word.clear();
				print_info(fixed_ignore_word_msg);
				continue;
			} else
				have_errors = true;
		}
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
	std::vector<std::pair<size_t, size_t> > overlapping_blocks;
	::verify_data_blocks_overlapping(sort_index, overlapping_blocks);
	for(size_t i=0; i<overlapping_blocks.size(); ++i) {
		const worditem_t& first = *sort_index[overlapping_blocks[i].first];
		const worditem_t& second = *sort_index[overlapping_blocks[i].second];
		print_info("Warning: Index item %s and index item %s refer to overlapping "
			"but not equal regions (offset, size): "
			"(%u, %u) and (%u, %u)\n",
			first.word.c_str(), second.word.c_str(),
			first.offset, first.size, second.offset, second.size);
	}
	// find not used regions
	std::vector<region_t> unused_regions;
	verify_unused_regions(sort_index, unused_regions, dictfilesize);
	if(!unused_regions.empty()) {
		print_info("Warning: Dictionary contains unreferenced blocks (offset, size):\n");
		for(size_t i = 0; i<unused_regions.size(); ++i)
			print_info("\t(%u, %u)\n", unused_regions[i].offset, unused_regions[i].size);
	}
}

