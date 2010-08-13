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

static const char* const key_forbidden_chars = "\t\n\r";
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
static const char* const known_type_ids = "mtygxkwhnr";

static bool compare_worditem_by_offset(const worditem_t* left, const worditem_t* right)
{
	return left->offset < right->offset;
}

class dictionary_data_block {
public:
	/* p_res_storage may be NULL */
	int verify(const char* const data, size_t data_size,
		const std::string& sametypesequence, print_info_t print_info,
		const char* word,
		i_resource_storage* p_res_storage = NULL)
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
		if(p_res_storage && !p_res_storage->have_file(key)) {
			print_info("Warning: Index item %s. Type id '%c'. The field refers to resource \"%s\", "
				"that is not found in resource storage.\n",
				word, type_id, key.c_str());
			have_errors = true;
		}
		p = tag + sizeof("</rref>") - 1;
	}
	return have_errors ? EXIT_FAILURE : EXIT_SUCCESS;
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

