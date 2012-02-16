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
#  ifdef min
#    undef min
#  endif
#  ifdef max
#    undef max
#  endif
#else
#  include <unistd.h>
#endif

#include "lib_res_store.h"
#include "libcommon.h"
#include "ifo_file.h"
#include "lib_binary_dict_parser.h"
#include "lib_dict_verify.h"
#include "lib_chars.h"

/* Limit the initially reserved index size.
 * .ifo file may contain incorrect, unreasonably large value of index size,
 * so we'd be out of memory if we try to allocate such amount. */
const guint32 MAX_RESERVED_INDEX_SIZE = 200*1024;

static bool compare_worditem_by_offset(const worditem_t* left, const worditem_t* right)
{
	return left->offset < right->offset;
}

binary_dict_parser_t::binary_dict_parser_t(void)
:
	dictfilesize(0),
	p_res_storage(NULL),
	fix_errors(false)
{

}

/* p_res_storage may be NULL */
VerifResult binary_dict_parser_t::load(const std::string& ifofilename,
	i_resource_storage* p_res_storage)
{
	this->ifofilename = ifofilename;
	this->p_res_storage = p_res_storage;
	VerifResult result = VERIF_RESULT_OK;
	if(!is_path_end_with(ifofilename, ".ifo")) {
		g_critical(unsupported_file_type_err, ifofilename.c_str());
		return combine_result(result, VERIF_RESULT_FATAL);
	}

	basefilename.assign(ifofilename, 0, ifofilename.length()-4);
	if(load_ifo_file())
		return combine_result(result, VERIF_RESULT_FATAL);
	result = combine_result(result, load_idx_file());
	if((fix_errors ? VERIF_RESULT_FATAL : VERIF_RESULT_CRITICAL) <= result)
		return result;
	result = combine_result(result, load_syn_file());
	if((fix_errors ? VERIF_RESULT_FATAL : VERIF_RESULT_CRITICAL) <= result)
		return result;
	result = combine_result(result, load_dict_file());
	if((fix_errors ? VERIF_RESULT_FATAL : VERIF_RESULT_CRITICAL) <= result)
		return result;
	return result;
}

int binary_dict_parser_t::get_data_fields(guint32 offset, guint32 size, data_field_vect_t& fields) const
{
	if(size == 0)
		return EXIT_FAILURE;
	fields.clear();

	const char* word = "???";
	std::vector<char> buffer(size);

	if(!dictfile) {
		g_critical(dictionary_no_loaded_err);
		return EXIT_FAILURE;
	}
	if(fseek(get_impl(dictfile), offset, SEEK_SET)) {
		std::string error(g_strerror(errno));
		g_critical(read_file_err, dictfilename.c_str(), error.c_str());
		return EXIT_FAILURE;
	}
	if(1 != fread(&buffer[0], size, 1, get_impl(dictfile))) {
		std::string error(g_strerror(errno));
		g_critical(read_file_err, dictfilename.c_str(), error.c_str());
		return EXIT_FAILURE;
	}

	dictionary_data_block data_block;
	data_block.set_resource_storage(p_res_storage);
	data_block.set_fix_errors(fix_errors);
	return VERIF_RESULT_FATAL <= data_block.load(&buffer[0], size, dict_info.get_sametypesequence(), word, &fields)
		? EXIT_FAILURE : EXIT_SUCCESS;
}

VerifResult binary_dict_parser_t::prepare_idx_file(void)
{
	VerifResult result = VERIF_RESULT_OK;
	const std::string index_file_name_gz = basefilename + ".idx.gz";
	const std::string index_file_name_idx = basefilename + ".idx";
	if(g_file_test(index_file_name_gz.c_str(), G_FILE_TEST_EXISTS)
		&& g_file_test(index_file_name_idx.c_str(), G_FILE_TEST_EXISTS)) {
		g_warning(two_index_files_msg, index_file_name_gz.c_str(), index_file_name_idx.c_str());
		result = combine_result(result, VERIF_RESULT_WARNING);
	}
	idxfilename_orig=index_file_name_gz;
	if(g_file_test(idxfilename_orig.c_str(), G_FILE_TEST_EXISTS)) {
		idxfilename = idxtemp.create_temp_file();
		if(idxfilename.empty())
			return combine_result(result, VERIF_RESULT_FATAL);
		if(unpack_zlib(idxfilename_orig.c_str(), idxfilename.c_str()))
			return combine_result(result, VERIF_RESULT_FATAL);
	} else {
		idxfilename_orig = index_file_name_idx;
		idxfilename = idxfilename_orig;
	}
	return result;
}

VerifResult binary_dict_parser_t::prepare_dict_file(void)
{
	VerifResult result = VERIF_RESULT_OK;
	const std::string dict_file_name_dz = basefilename + ".dict.dz";
	const std::string dict_file_name_dict = basefilename + ".dict";
	if(g_file_test(dict_file_name_dz.c_str(), G_FILE_TEST_EXISTS)
		&& g_file_test(dict_file_name_dict.c_str(), G_FILE_TEST_EXISTS)) {
		g_warning(two_dict_files_msg, dict_file_name_dz.c_str(), dict_file_name_dict.c_str());
		result = combine_result(result, VERIF_RESULT_WARNING);
	}
	dictfilename_orig=dict_file_name_dz;
	if(g_file_test(dictfilename_orig.c_str(), G_FILE_TEST_EXISTS)) {
		dictfilename = dicttemp.create_temp_file();
		if(dictfilename.empty())
			return combine_result(result, VERIF_RESULT_FATAL);
		if(unpack_zlib(dictfilename_orig.c_str(), dictfilename.c_str()))
			return combine_result(result, VERIF_RESULT_FATAL);
	} else {
		dictfilename_orig = dict_file_name_dict;
		dictfilename = dictfilename_orig;
	}
	return result;
}

int binary_dict_parser_t::load_ifo_file(void)
{
	if(!dict_info.load_from_ifo_file(ifofilename, DictInfoType_NormDict))
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}

VerifResult binary_dict_parser_t::load_idx_file(void)
{
	VerifResult result = VERIF_RESULT_OK;
	{
		VerifResult res = prepare_idx_file();
		result = combine_result(result, res);
		if((fix_errors ? VERIF_RESULT_FATAL : VERIF_RESULT_CRITICAL) <= res)
			return result;
	}

	guint32 idxfilesize;
	{
		stardict_stat_t stats;
		if (g_stat (idxfilename.c_str(), &stats) == -1) {
			std::string error(g_strerror(errno));
			g_critical(file_not_found_idx_err, idxfilename.c_str(), error.c_str());
			return combine_result(result, VERIF_RESULT_FATAL);
		}
		idxfilesize = (guint32)stats.st_size;
	}
	g_message(loading_idx_file_msg, idxfilename_orig.c_str());

	if (dict_info.get_index_file_size() != idxfilesize) {
		g_warning(incorrect_idx_file_size_err,
			dict_info.get_index_file_size(), idxfilesize);
		result = combine_result(result, VERIF_RESULT_CRITICAL);
		if(fix_errors) {
			dict_info.set_index_file_size(idxfilesize);
			g_message(fixed_msg);
		} else
			return result;
	}

	index.clear();
	index.reserve(std::min(MAX_RESERVED_INDEX_SIZE, dict_info.get_wordcount()));

	std::vector<gchar> buf(idxfilesize+1);
	gchar * const buffer_beg = &buf[0];
	gchar * const buffer_end = buffer_beg+idxfilesize;
	{
		FILE *idxfile = g_fopen(idxfilename.c_str(),"rb");
		if(!idxfile) {
			std::string error(g_strerror(errno));
			g_critical(open_read_file_err, idxfilename.c_str(), error.c_str());
			return combine_result(result, VERIF_RESULT_FATAL);
		}
		if(idxfilesize != fread(buffer_beg, 1, idxfilesize, idxfile)) {
			std::string error(g_strerror(errno));
			g_critical(open_read_file_err, idxfilename.c_str(), error.c_str());
			fclose(idxfile);
			return combine_result(result, VERIF_RESULT_FATAL);
		}
		fclose(idxfile);
	}

	const char *p=buffer_beg;
	int wordlen;
	gint cmpvalue;
	guint wordcount=0;
	worditem_t worditem, preworditem;
	size_t size_remain; // to the end of the index file

	while (p < buffer_end) {
		size_remain = buffer_end - p;
		const char* const word_end = reinterpret_cast<const char*>(memchr(p, '\0', size_remain));
		if(!word_end) {
			g_warning(index_file_truncated_err);
			result = combine_result(result, VERIF_RESULT_CRITICAL);
			if(fix_errors)
				g_message(fixed_ignore_file_tail_msg);
			break;
		}
		worditem.word = p;
		wordlen = worditem.word.length();
		if (!g_utf8_validate(worditem.word.c_str(), wordlen, NULL)) {
			g_warning(word_invalid_utf8_err, worditem.word.c_str());
			result = combine_result(result, VERIF_RESULT_CRITICAL);
			if(fix_errors) {
				worditem.word = fix_utf8_str(worditem.word, 0);
				wordlen = worditem.word.length();
				g_message(fixed_utf8_drop_invalid_char_msg);
			}
		}
		{	// check for invalid chars
			typedef std::list<const char*> str_list_t;
			str_list_t invalid_chars;
			const char* const word = worditem.word.c_str();
			if(check_xml_string_chars(word, invalid_chars)) {
				result = combine_result(result, VERIF_RESULT_WARNING);
				g_message(word_invalid_char_value_err,
					word, print_char_codes(invalid_chars).c_str());
				if(fix_errors) {
					g_message(fixed_drop_invalid_char_msg);
					fix_xml_string_chars(word, worditem.word);
					wordlen = worditem.word.length();
				}
			}
		}
		if (wordlen > 0) {
			if (wordlen>=MAX_INDEX_KEY_SIZE) {
				g_warning(long_word_err, worditem.word.c_str(), MAX_INDEX_KEY_SIZE, wordlen);
				result = combine_result(result, VERIF_RESULT_CRITICAL);
				if(fix_errors) {
					wordlen = truncate_utf8_string(worditem.word.c_str(), wordlen, MAX_INDEX_KEY_SIZE-1);
					worditem.word.resize(wordlen);
					g_message(fixed_word_truncated_msg);
				}
			}
			bool have_spaces = false;
			if (g_ascii_isspace(worditem.word[0])) {
				g_message(word_begin_space_err, worditem.word.c_str());
				result = combine_result(result, VERIF_RESULT_NOTE);
				have_spaces = true;
			}
			if (g_ascii_isspace(worditem.word[wordlen-1])) {
				g_message(word_end_space_err, worditem.word.c_str());
				result = combine_result(result, VERIF_RESULT_NOTE);
				have_spaces = true;
			}
			if(have_spaces && fix_errors) {
				g_message(fixed_trim_spaces);
				const char* new_beg;
				size_t new_len;
				trim_spaces(worditem.word.c_str(), new_beg, new_len);
				if(new_len == 0)
					worditem.word.clear();
				else {
					std::string tmp(new_beg, new_len);
					worditem.word = tmp;
				}
			}
		}
		if(check_stardict_key_chars(worditem.word.c_str())) {
			g_message(word_forbidden_chars_err, worditem.word.c_str());
			result = combine_result(result, VERIF_RESULT_NOTE);
			if(fix_errors) {
				g_message(fixed_drop_invalid_char_msg);
				std::string tmp;
				fix_stardict_key_chars(worditem.word.c_str(), tmp);
				worditem.word = tmp;
				wordlen = worditem.word.length();
			}
		}
		if (wordlen==0) {
			g_warning(empty_word_err);
			result = combine_result(result, VERIF_RESULT_WARNING);
			if(fix_errors)
				g_message(fixed_ignore_word_msg);
		}
		if (!preworditem.word.empty() && !worditem.word.empty()) {
			cmpvalue=stardict_strcmp(preworditem.word.c_str(), worditem.word.c_str());
			if (cmpvalue>0) {
				g_warning(wrong_word_order_err, preworditem.word.c_str(), worditem.word.c_str());
				result = combine_result(result, VERIF_RESULT_WARNING);
				if(fix_errors)
					g_message(fixed_words_reordered_msg);
			}
		}
		p = word_end + 1;
		size_remain = buffer_end - p;
		if(size_remain < 2 * sizeof(guint32)) {
			g_warning(index_file_truncated_err);
			result = combine_result(result, VERIF_RESULT_CRITICAL);
			if(fix_errors)
				g_message(fixed_ignore_file_tail_msg);
			break;
		}
		worditem.offset = g_ntohl(*reinterpret_cast<const guint32 *>(p));
		p += sizeof(guint32);
		worditem.size = g_ntohl(*reinterpret_cast<const guint32 *>(p));
		p += sizeof(guint32);
		if (worditem.size==0) {
			g_warning(empty_block_err, worditem.word.c_str());
			result = combine_result(result, VERIF_RESULT_WARNING);
			if(fix_errors) {
				worditem.word.clear();
				g_message(fixed_ignore_word_msg);
			}
		}
		preworditem = worditem;
		wordcount++;
		index.push_back(worditem);
	} // while

	g_assert(p <= buffer_end);

	if (dict_info.get_wordcount() != wordcount) {
		g_warning(incorrect_word_cnt_err, dict_info.get_wordcount(), wordcount);
		result = combine_result(result, VERIF_RESULT_CRITICAL);
		if(fix_errors) {
			dict_info.set_wordcount(wordcount);
			g_message(fixed_msg);
		}
	}

	for(size_t i=0; i < index.size(); ++i) {
		if(index[i].word.empty())
			continue;
		for(size_t j=i+1; j < index.size() && index[i].word == index[j].word; ++j) {
			if(index[i].offset == index[j].offset && index[i].size == index[j].size) {
				g_warning(duplicate_index_item_err,
					index[i].word.c_str(), index[i].offset, index[i].size);
				result = combine_result(result, VERIF_RESULT_NOTE);
				break;
			}
		}
	}

	return result;
}

VerifResult binary_dict_parser_t::load_syn_file(void)
{
	synfilename = basefilename + ".syn";
	VerifResult result = VERIF_RESULT_OK;

	if (dict_info.get_synwordcount() == 0) {
		if (g_file_test(synfilename.c_str(), G_FILE_TEST_EXISTS)) {
			g_warning(syn_file_exist_msg);
			result = combine_result(result, VERIF_RESULT_WARNING);
			if(fix_errors) {
				g_message(fixed_process_syn_file_msg);
			} else
				return result;
		} else
			return result;
	}

	guint32 synfilesize;
	{
		stardict_stat_t stats;
		if (g_stat (synfilename.c_str(), &stats) == -1) {
			std::string error(g_strerror(errno));
			g_warning(syn_file_no_found_msg, synfilename.c_str(), error.c_str());
			result = VERIF_RESULT_CRITICAL;
			if(fix_errors) {
				dict_info.set_synwordcount(0);
				g_message(fixed_ignore_syn_file_msg);
				return result;
			} else
				return result;
		}
		synfilesize = stats.st_size;
	}
	g_message(loading_syn_file_msg, synfilename.c_str());

	synindex.clear();
	synindex.reserve(std::min(MAX_RESERVED_INDEX_SIZE, dict_info.get_synwordcount()));

	std::vector<gchar> buf(synfilesize+1);
	gchar *buffer_begin = &buf[0];
	gchar *buffer_end = buffer_begin+synfilesize;
	{
		FILE *synfile = g_fopen(synfilename.c_str(),"rb");
		if(!synfile) {
			std::string error(g_strerror(errno));
			g_warning(open_read_file_err, synfilename.c_str(), error.c_str());
			result = VERIF_RESULT_CRITICAL;
			if(fix_errors) {
				dict_info.set_synwordcount(0);
				g_message(fixed_ignore_syn_file_msg);
				return result;
			} else
				return result;
		}
		if(synfilesize != fread (buffer_begin, 1, synfilesize, synfile)) {
			std::string error(g_strerror(errno));
			g_warning(open_read_file_err, synfilename.c_str(), error.c_str());
			result = VERIF_RESULT_CRITICAL;
			fclose (synfile);
			if(fix_errors) {
				dict_info.set_synwordcount(0);
				g_message(fixed_ignore_syn_file_msg);
				return result;
			} else
				return result;
		}
		fclose (synfile);
	}

	const char *p=buffer_begin;
	int wordlen;
	gint cmpvalue;
	guint wordcount=0;
	synitem_t synitem, presynitem;
	size_t size_remain; // to the end of the synonyms file

	while (p < buffer_end) {
		size_remain = buffer_end - p;
		const char* const word_end = reinterpret_cast<const char*>(memchr(p, '\0', size_remain));
		if(!word_end) {
			g_warning(syn_file_truncated_err);
			result = combine_result(result, VERIF_RESULT_CRITICAL);
			if(fix_errors)
				g_message(fixed_ignore_file_tail_msg);
			break;
		}
		synitem.word = p;
		wordlen = synitem.word.length();
		if (!g_utf8_validate(synitem.word.c_str(), wordlen, NULL)) {
			g_warning(word_invalid_utf8_err, synitem.word.c_str());
			result = combine_result(result, VERIF_RESULT_CRITICAL);
			if(fix_errors) {
				synitem.word = fix_utf8_str(synitem.word);
				wordlen = synitem.word.length();
				g_message(fixed_utf8_drop_invalid_char_msg);
			}
		}
		{	// check for invalid chars
			typedef std::list<const char*> str_list_t;
			str_list_t invalid_chars;
			const char* const word = synitem.word.c_str();
			if(check_xml_string_chars(word, invalid_chars)) {
				result = combine_result(result, VERIF_RESULT_WARNING);
				g_message(word_invalid_char_value_err,
					word, print_char_codes(invalid_chars).c_str());
				if(fix_errors) {
					g_message(fixed_drop_invalid_char_msg);
					fix_xml_string_chars(word, synitem.word);
					wordlen = synitem.word.length();
				}
			}
		}
		if (wordlen > 0) {
			if (wordlen>=MAX_INDEX_KEY_SIZE) {
				g_warning(long_word_err, synitem.word.c_str(), MAX_INDEX_KEY_SIZE, wordlen);
				result = combine_result(result, VERIF_RESULT_CRITICAL);
				if(fix_errors) {
					wordlen = truncate_utf8_string(synitem.word.c_str(), wordlen, MAX_INDEX_KEY_SIZE-1);
					synitem.word.resize(wordlen);
					g_message(fixed_word_truncated_msg);
				}
			}
			bool have_spaces = false;
			if (g_ascii_isspace(synitem.word[0])) {
				g_message(word_begin_space_err, synitem.word.c_str());
				result = combine_result(result, VERIF_RESULT_NOTE);
				have_spaces = true;
			}
			if (g_ascii_isspace(synitem.word[wordlen-1])) {
				g_message(word_end_space_err, synitem.word.c_str());
				result = combine_result(result, VERIF_RESULT_NOTE);
				have_spaces = true;
			}
			if(have_spaces && fix_errors) {
				g_message(fixed_trim_spaces);
				const char* new_beg;
				size_t new_len;
				trim_spaces(synitem.word.c_str(), new_beg, new_len);
				if(new_len == 0)
					synitem.word.clear();
				else {
					std::string tmp(new_beg, new_len);
					synitem.word = tmp;
				}
			}
		}
		if (check_stardict_key_chars(synitem.word.c_str())) {
			g_message(word_forbidden_chars_err, synitem.word.c_str());
			result = combine_result(result, VERIF_RESULT_NOTE);
			if(fix_errors) {
				g_message(fixed_drop_invalid_char_msg);
				std::string tmp;
				fix_stardict_key_chars(synitem.word.c_str(), tmp);
				synitem.word = tmp;
				wordlen = synitem.word.length();
			}
		}
		if (wordlen==0) {
			g_warning(empty_word_err);
			result = combine_result(result, VERIF_RESULT_WARNING);
			if(fix_errors)
				g_message(fixed_ignore_word_msg);
		}
		if (!presynitem.word.empty() && !synitem.word.empty()) {
			cmpvalue=stardict_strcmp(presynitem.word.c_str(), synitem.word.c_str());
			if (cmpvalue>0) {
				g_warning(wrong_word_order_err, presynitem.word.c_str(), synitem.word.c_str());
				result = combine_result(result, VERIF_RESULT_WARNING);
				if(fix_errors)
					g_message(fixed_words_reordered_msg);
			}
		}
		p = word_end +1;
		size_remain = buffer_end - p;
		if(size_remain < sizeof(guint32)) {
			g_warning(syn_file_truncated_err);
			result = combine_result(result, VERIF_RESULT_CRITICAL);
			if(fix_errors)
				g_message(fixed_ignore_file_tail_msg);
			break;
		}
		synitem.index = g_ntohl(*reinterpret_cast<const guint32 *>(p));
		if (synitem.index>=dict_info.get_wordcount()) {
			g_warning(wrong_index_err, synitem.word.c_str(), synitem.index);
			result = combine_result(result, VERIF_RESULT_CRITICAL);
			if(fix_errors) {
				synitem.word.clear();
				g_message(fixed_ignore_word_msg);
			}
		}
		p+=sizeof(guint32);
		presynitem = synitem;
		wordcount++;
		synindex.push_back(synitem);
	} // while

	g_assert(p <= buffer_end);

	if (wordcount != dict_info.get_synwordcount()) {
		g_warning(incorrect_syn_word_cnt_err,
			dict_info.get_synwordcount(), wordcount);
		result = combine_result(result, VERIF_RESULT_CRITICAL);
		if(fix_errors) {
			dict_info.set_synwordcount(wordcount);
			g_message(fixed_msg);
		}
	}

	for(size_t i=0; i < synindex.size(); ++i) {
		for(size_t j=i+1; j < synindex.size() && synindex[i].word == synindex[j].word; ++j) {
			if(synindex[i].index == synindex[j].index) {
				g_warning(duplicate_syn_item_err,
					synindex[i].word.c_str(), synindex[i].index);
				result = combine_result(result, VERIF_RESULT_NOTE);
				break;
			}
		}
	}

	if((fix_errors ? VERIF_RESULT_FATAL : VERIF_RESULT_CRITICAL) <= result) {
		g_warning(load_syn_file_failed_err, synfilename.c_str());
		if(fix_errors) {
			dict_info.set_synwordcount(0);
			synindex.clear();
			g_message(fixed_ignore_syn_file_msg);
			result = VERIF_RESULT_CRITICAL;
		}
	}
	return result;
}

VerifResult binary_dict_parser_t::load_dict_file(void)
{
	VerifResult result = VERIF_RESULT_OK;
	{
		VerifResult res = prepare_dict_file();
		result = combine_result(result, res);
		if((fix_errors ? VERIF_RESULT_FATAL : VERIF_RESULT_CRITICAL) <= res)
			return result;
	}

	{
		stardict_stat_t stats;
		if (g_stat (dictfilename.c_str(), &stats) == -1) {
			std::string error(g_strerror(errno));
			g_critical(dict_file_not_found_err, dictfilename.c_str(), error.c_str());
			return combine_result(result, VERIF_RESULT_FATAL);
		}
		dictfilesize = stats.st_size;
	}

	g_message(loading_dict_file_err, dictfilename_orig.c_str());
	dictfile.reset(g_fopen(dictfilename.c_str(), "rb"));
	if(!dictfile) {
		std::string error(g_strerror(errno));
		g_critical(open_dict_file_failed_err, dictfilename.c_str(), error.c_str());
		return combine_result(result, VERIF_RESULT_FATAL);
	}

	std::vector<char> buffer;
	dictionary_data_block block_verifier;
	block_verifier.set_resource_storage(p_res_storage);
	block_verifier.set_fix_errors(fix_errors);
	for(size_t i=0; i<index.size(); ++i) {
		if(index[i].word.empty())
			continue;
		if(index[i].size == 0)
			continue;
		if(index[i].offset + index[i].size > dictfilesize) {
			g_warning(record_out_of_file_err, index[i].word.c_str());
			result = combine_result(result, VERIF_RESULT_CRITICAL);
			if(fix_errors) {
				if(index[i].offset >= dictfilesize) {
					index[i].word.clear();
					g_message(fixed_ignore_word_msg);
					continue;
				} else {
					index[i].size = dictfilesize - index[i].offset;
					g_message(fixed_data_block_size_change_msg);
				}
			} else {
				continue;
			}
		}
		buffer.resize(index[i].size);
		if(fseek(get_impl(dictfile), index[i].offset, SEEK_SET)) {
			std::string error(g_strerror(errno));
			g_critical(read_file_err, dictfilename.c_str(), error.c_str());
			return combine_result(result, VERIF_RESULT_FATAL);
		}
		if(1 != fread(&buffer[0], index[i].size, 1, get_impl(dictfile))) {
			std::string error(g_strerror(errno));
			g_critical(read_file_err, dictfilename.c_str(), error.c_str());
			return combine_result(result, VERIF_RESULT_FATAL);
		}
		VerifResult result2 = block_verifier.load(&buffer[0], index[i].size,
				dict_info.get_sametypesequence(), index[i].word.c_str());
		if(VERIF_RESULT_FATAL <= result2) {
			result = combine_result(result, VERIF_RESULT_CRITICAL);
			if(fix_errors) {
				index[i].word.clear();
				g_message(fixed_ignore_word_msg);
				continue;
			}
		} else
			result = combine_result(result, result2);
	}
	result = combine_result(result, verify_data_blocks_overlapping());
	return result;
}

VerifResult binary_dict_parser_t::verify_data_blocks_overlapping(void)
{
	VerifResult result = VERIF_RESULT_OK;
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
		g_warning(overlapping_data_blocks_msg,
			first.word.c_str(), second.word.c_str(),
			first.offset, first.size, second.offset, second.size);
		result = combine_result(result, VERIF_RESULT_WARNING);
	}
	// find not used regions
	std::vector<region_t> unused_regions;
	verify_unused_regions(sort_index, unused_regions, dictfilesize);
	if(!unused_regions.empty()) {
		g_warning(unreferenced_data_blocks_msg);
		result = combine_result(result, VERIF_RESULT_NOTE);
		for(size_t i = 0; i<unused_regions.size(); ++i)
			g_warning("\t(%u, %u)", unused_regions[i].offset, unused_regions[i].size);
	}
	return result;
}

