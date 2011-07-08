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

#ifndef _LIBSTARDICTVERIFY_H_
#define _LIBSTARDICTVERIFY_H_

#include <vector>
#include <algorithm>
#include "libcommon.h"

#ifdef _WIN32
#  ifdef min
#    undef min
#  endif
#  ifdef max
#    undef max
#  endif
#endif

enum VerifResult {
	VERIF_RESULT_OK, // no error
	VERIF_RESULT_NOTE, // minor issue, save to ignore (for example, trailing spaces in key word)
	VERIF_RESULT_WARNING, // important issue, maybe ignored (double keys in index, referring to the same data)
	VERIF_RESULT_CRITICAL, // may be fixed, but cannot be ignored (for example, index entries are out of order)
	VERIF_RESULT_FATAL // cannot be fixed (for example, .idx file is missing)
};

extern VerifResult stardict_verify(const char *ifofilename);

struct region_t {
	guint32 offset;
	guint32 size;
};

/* combine two verification results = the most serious error */
inline
VerifResult combine_result(VerifResult a, VerifResult b)
{
	return std::max(a, b);
}

template <class item_t>
void verify_data_blocks_overlapping(std::vector<item_t*>& sort_index,
	std::vector<std::pair<size_t, size_t> >& overlapping_blocks)
{
	for(size_t i=0; i<sort_index.size(); ++i) {
		for(size_t j=i+1; j<sort_index.size()
			&& sort_index[i]->offset + sort_index[i]->size > sort_index[j]->offset; ++j) {
			if(sort_index[i]->offset == sort_index[j]->offset
				&& sort_index[i]->size == sort_index[j]->size)
				continue;
			if(sort_index[j]->size == 0)
				continue;
			overlapping_blocks.push_back(std::pair<size_t, size_t>(i, j));
		}
	}
}

template <class item_t>
void verify_unused_regions(std::vector<item_t*>& sort_index,
		std::vector<region_t>& unused_regions, guint32 filesize)
{
	region_t region;
	guint32 low_boundary=0;
	for(size_t i=0; i<sort_index.size(); ++i) {
		const guint32 l_left = sort_index[i]->offset;
		const guint32 l_right = sort_index[i]->offset + sort_index[i]->size;
		if(l_left < low_boundary) {
			if(l_right > low_boundary)
				low_boundary = l_right;
		} if(l_left == low_boundary) {
			low_boundary = l_right;
		} else { // gap found
			region.offset = low_boundary;
			region.size = l_left - low_boundary;
			unused_regions.push_back(region);
			low_boundary = l_right;
		}
	}
	if(low_boundary < filesize) {
		region.offset = low_boundary;
		region.size = filesize - low_boundary;
		unused_regions.push_back(region);
	}
}

#define index_file_truncated_err \
	"Index file is truncated, last record is truncated."
#define incorrect_data_block_size_err \
	"Index item '%s'. Fields do not fit into the data block, incorrect data block size."
#define empty_field_err \
	"Index item '%s'. Empty field in definition data block. Type ID '%c'."
#define invalid_utf8_field_err \
	"Index item '%s'. Invalid field. Type id = '%c'. Invalid utf8 string: '''\n%s\n'''"
#define invalid_utf8_index_item_err \
	"Index item '%s'. Invalid field. Invalid utf8 string: '''\n%s\n'''"
#define invalid_field_content_err \
	"Index item '%s'. Type id '%c'. Invalid field content: '''\n%s\n'''"
#define invalid_chars_in_textual_data_msg \
	"The text contains either invalid Unicode characters " \
	"or Unicode characters not suitable for textual data (mainly control characters). " \
	"The following characters are prohibited: %s."
#define invalid_field_content_chars_err \
	"Index item '%s'. Type id '%c'. Invalid field content: '''\n%s\n'''\n"\
	invalid_chars_in_textual_data_msg
#define syn_file_truncated_err \
	"Synonyms file is truncated, last record is truncated."
#define unknown_type_id_err \
	"Index item '%s'. Unknown type identifier '%c'."
#define empty_word_err \
	"Blank key in index."
#define empty_file_name_err \
	"Blank file name in index."
#define long_word_err \
	"Index item '%s'. Key is too long. Maximum allowed length: %d, key length: %d."
#define word_begin_space_err \
	"Index item '%s'. Key begins with a space character."
#define word_end_space_err \
	"Index item '%s'. Key ends with a space character."
#define word_forbidden_chars_err \
	"Index item '''%s'''\nKey contains forbidden characters."
#define word_invalid_utf8_err \
	"Index item '%s'. Invalid utf8 string."
#define word_invalid_char_value_err \
	"Index item '%s'. Invalid item name.\n" \
	invalid_chars_in_textual_data_msg
#define wrong_word_order_err \
	"Wrong key order, first key = '%s', second key = '%s'."
#define wrong_file_order_err \
	"Wrong file order, first file name = '%s', second file name = '%s'."
#define fields_extraction_faild_err \
	"Index item '%s'. Extraction of the fields failed."
#define unsupported_file_type_err \
	"Unsupported file type. File must have 'ifo' extension. File: '%s'."
#define dictionary_no_loaded_err \
	"Dictionary is not loaded."
#define file_not_found_idx_err \
	"Unable to find index file: '%s'. Error: %s."
#define loading_idx_file_msg \
	"Loading index file: '%s'..."
#define incorrect_idx_file_size_err \
	"Incorrect size of the index file: in .ifo file, idxfilesize=%u, real file size is %u."
#define incorrect_ridx_file_size_err \
	"Incorrect size of the index file: in .rifo file, ridxfilesize=%d, real file size is %ld."
#define empty_block_err \
	"Index item '%s'. Data block size = 0."
#define incorrect_word_cnt_err \
	"Incorrect number of words: in .ifo file, wordcount=%d, while the real word count is %d."
#define incorrect_syn_word_cnt_err \
	"Incorrect number of words: in .ifo file, synwordcount=%d, while the real synwordcount is %d."
#define duplicate_index_item_err \
	"Multiple index items have the same key = '%s', offset = %d, size = %d."
#define duplicate_syn_item_err \
	"Multiple synonym items with the same key = '%s', index = %d."
#define syn_file_exist_msg \
	".syn file exists but there is no \"synwordcount=\" entry in .ifo file."
#define syn_file_no_found_msg \
	"Unable to find synonyms file '%s'. Error: %s."
#define loading_syn_file_msg \
	"Loading synonyms file: '%s'..."
#define wrong_index_err \
	"Index item '%s'. Wrong index of entry in the index file: %d."
#define load_syn_file_failed_err \
	"Loading synonyms file failed: '%s'."
#define dict_file_not_found_err \
	"Dictionary file does not exist: '%s'. Error: %s."
#define loading_dict_file_err \
	"Loading dictionary file: '%s'..."
#define open_dict_file_failed_err \
	"Unable open dictionary file '%s'. Error: %s."
#define record_out_of_file_err \
	"Index item '%s'. Incorrect size, offset parameters. Referenced data block is outside dictionary file."
#define overlapping_data_blocks_msg \
	"Index item '%s' and index item '%s' refer to overlapping but not equal regions (offset, size): " \
	"(%u, %u) and (%u, %u)."
#define unreferenced_data_blocks_msg \
	"Dictionary contains unreferenced data blocks (offset, size):"
#define rdb_unreferenced_data_blocks_msg \
	"Resource database contains unreferenced data blocks (offset, size):"
#define data_block_no_fields_err \
	"Index item '%s'. No fields were extracted."
#define resource_not_found_msg \
	"Index item '%s'. Type id '%c'. The field refers to resource '%s', that is not found in resource storage."
#define resource_invalid_format_empty_line_msg \
	"Index item '%s'. Type id '%c'. Invalid field format. Empty resource line."
#define resource_invalid_format_colon_msg \
	"Index item '%s'. Type id '%c'. Invalid field format. Line: '%s'. ':' is not found."
#define resource_invalid_format_type_blank_msg \
	"Index item '%s'. Type id '%c'. Invalid field format. Line: '%s'. Type is blank."
#define resource_invalid_format_key_blank_msg \
	"Index item '%s'. Type id '%c'. Invalid field format. Line: '%s'. Key is blank."
#define resource_invalid_format_unknown_type_msg \
	"Index item '%s'. Type id '%c'. Invalid field format. Line: '%s'. Unknown type."
#define resource_invalid_format_back_spash_msg \
	"Index item '%s'. Type id '%c'. Invalid field format. Line: '%s'. Key contains '\\' char."
#define resource_resource_nof_found_msg \
	"Index item '%s'. Type id '%c'. Line '%s'. The field refers to resource '%s', that is not found in resource storage."
#define resource_empty_list_msg \
	"Index item '%s'. Type id '%c'. Empty resource list."
#define two_index_files_msg \
	"Two index files were found: compressed '%s' and uncompressed '%s'. We will use the compressed version."
#define two_dict_files_msg \
	"Two dictionary files were found: compressed '%s' and uncompressed '%s'. We will use the compressed version."
#define rdb_filecnt_zero_err \
	"Resource database '%s'. No files. filecount = 0."
#define rdb_ridxfilesize_zero_err \
	"Resource database '%s'. Empty index file size. ridxfilesize = 0."
#define rdb_invalid_file_name_format_back_spash_err \
	"Index item '%s'. Found '\\' character. '/' must be used as directory separator."
#define rdb_invalid_file_name_format_abs_path_err \
	"Index item '%s'. File name must not start with directory separator '/'."
#define rdb_invalid_file_name_format_empty_dir_err \
	"Index item '%s'. Empty directory in file path: '//'."
#define rdb_incorrect_file_cnt \
	"Incorrect number of files: in .rifo file, filecount=%d, while the real file count is %d."
#define rdb_dict_file_not_found_err \
	"Unable to find resource dictionary file: '%s'. Error: %s."
#define rdb_loading_ridx_file_msg \
	"Loading resource index file: '%s'..."
#define rdb_loading_dict_file_msg \
	"Loading resource dictionary file: '%s'..."
#define rdb_loaded_db_msg \
	"Resource storage loaded. Type - database."
#define rdb_load_db_failed_msg \
	"Resource storage load failed. Type - database."
#define rdb_loaded_files_msg \
	"Resource storage loaded. Type - files."
#define rdb_load_files_failed_msg \
	"Resource storage load failed. Type - files."
#define rdb_two_index_files_msg \
	"Two resource index files were found: compressed '%s' and uncompressed '%s'. We will use the compressed version."
#define rdb_two_dict_files_msg \
	"Two resource dictionary files were found: compressed '%s' and uncompressed '%s'. We will use the compressed version."

#define fixed_ignore_field_msg \
	"The problem was fixed. Ignore the field."
#define duplicate_file_name \
	"Multiple index items with the same file name: '%s'."
#define fixed_accept_unknown_field_msg \
	"The problem was fixed. Accept unknown field type."
#define fixed_ignore_resource_line_msg \
	"The problem was fixed. Ignore the resource line."
#define fixed_ignore_file_tail_msg \
	"The problem was fixed. Ignore the tail of the file."
#define fixed_ignore_syn_file_msg \
	"The problem was fixed. Ignore the .syn file."
#define fixed_ignore_word_msg \
	"The problem was fixed. Ignore the key."
#define fixed_drop_invalid_char_msg \
	"The problem was fixed. Dropping invalid chars."
#define fixed_word_truncated_msg \
	"The problem was fixed. The key is truncated."
#define fixed_words_reordered_msg \
	"The problem was fixed. Key will be reordered."
#define fixed_process_syn_file_msg \
	"The problem was fixed. Process the .syn file."
#define fixed_data_block_size_change_msg \
	"The problem was fixed. Changed size of the data block."
#define fixed_change_field_size_msg \
	"The problem was fixed. Change field size."
#define fixed_field_take_longest_str_msg \
	"The problem was fixed. Take the longest string."
#define fixed_field_take_zero_term_str_msg \
	"The problem was fixed. Take a zero-terminated string."
#define fixed_trim_spaces \
	"The problem was fixed. Leading and trailing spaces trimmed."
#define fixed_utf8_drop_invalid_char_msg \
	"The problem was fixed. Dropping invalid UTF-8 characters."

#endif

