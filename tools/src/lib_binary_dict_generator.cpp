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

#include <cstring>
#include <vector>
#include <algorithm>
#include "lib_binary_dict_generator.h"
#include "lib_dict_verify.h"

struct synitem_t {
	synitem_t(const std::string& synonym, size_t index)
	:
		synonym(synonym),
		index(index)
	{

	}
	std::string synonym;
	size_t index;
};

static bool compare_synitems_by_synonym(const synitem_t& left, const synitem_t& right)
{
	return 0 > stardict_strcmp(left.synonym.c_str(), right.synonym.c_str());
}

binary_dict_gen_t::binary_dict_gen_t(void)
:
	norm_dict(NULL),
	use_same_type_sequence(true),
	compress_dict(true)
{

}

int binary_dict_gen_t::generate(const std::string& ifofilename, common_dict_t *norm_dict)
{
	clear();
	this->ifofilename = ifofilename;
	this->norm_dict = norm_dict;
	if(!is_path_end_with(ifofilename, ".ifo")) {
		g_critical(unsupported_file_type_err, ifofilename.c_str());
		return EXIT_FAILURE;
	}
	basefilename.assign(ifofilename, 0, ifofilename.length() - (sizeof(".ifo")-1));
	decide_on_same_type_sequence();
	if(generate_dict_and_idx())
		return EXIT_FAILURE;
#ifndef _WIN32
	if(compress_dict) {
		std::string command(std::string("dictzip ") + dictfilename);
		int result;
		result = system(command.c_str());
		if (result == -1) {
			g_print("system() error!\n");
		}
	}
#endif
	if(generate_syn())
		return EXIT_FAILURE;
	norm_dict->dict_info.ifo_file_name = ifofilename;
	norm_dict->dict_info.set_infotype(DictInfoType_NormDict);
	if(!norm_dict->dict_info.save_ifo_file())
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}

void binary_dict_gen_t::clear(void)
{
	norm_dict = NULL;
	basefilename.clear();
	ifofilename.clear();
	dictfilename.clear();
	idxfilename.clear();
	synfilename.clear();
	dictfile.reset(NULL);
	idxfile.reset(NULL);
	synfile.reset(NULL);
}

int binary_dict_gen_t::generate_dict_and_idx(void)
{
	if(prepare_dict())
		return EXIT_FAILURE;
	if(prepare_idx())
		return EXIT_FAILURE;
	for(size_t i=0; i<norm_dict->articles.size(); ++i) {
		const article_data_t& article = norm_dict->articles[i];
		const guint32 offset = ftell(get_impl(dictfile));
		for(size_t j=0; j<article.definitions.size(); ++j) {
			if(same_type_sequence.empty()) {
				if(generate_dict_definition(article.definitions[j], article.key))
					return EXIT_FAILURE;
			} else {
				if(generate_dict_definition_sts(article.definitions[j], article.key,
						j+1 == article.definitions.size()))
					return EXIT_FAILURE;
			}
		}
		const guint32 size = ftell(get_impl(dictfile)) - offset;
		if(generate_index_item(article.key, offset, size))
			return EXIT_FAILURE;
	}
	norm_dict->dict_info.set_wordcount(norm_dict->articles.size());
	norm_dict->dict_info.set_index_file_size(ftell(get_impl(idxfile)));
	dictfile.reset(NULL);
	idxfile.reset(NULL);
	return EXIT_SUCCESS;
}

int binary_dict_gen_t::generate_syn(void)
{
	norm_dict->dict_info.unset_synwordcount();
	std::vector<synitem_t> synonyms;
	for(size_t i=0; i<norm_dict->articles.size(); ++i) {
		const article_data_t& article = norm_dict->articles[i];
		for(size_t j=0; j<article.synonyms.size(); ++j) {
			synonyms.push_back(synitem_t(article.synonyms[j], i));
		}
	}
	if(synonyms.empty())
		return EXIT_SUCCESS;
	std::sort(synonyms.begin(), synonyms.end(), compare_synitems_by_synonym);
	if(prepare_syn())
		return EXIT_SUCCESS;
	std::vector<char> buf;
	for(size_t i=0; i<synonyms.size(); ++i) {
		const size_t len = synonyms[i].synonym.length();
		buf.resize(len + 1 + sizeof(guint32));
		memcpy(&buf[0], synonyms[i].synonym.c_str(), len+1);
		*reinterpret_cast<guint32*>(&buf[len+1]) = g_htonl(static_cast<guint32>(synonyms[i].index));
		if(1 != fwrite(&buf[0], buf.size(), 1, get_impl(synfile))) {
			g_critical(write_file_err, synfilename.c_str());
			return EXIT_FAILURE;
		}
	}
	norm_dict->dict_info.set_synwordcount(synonyms.size());
	synfile.reset(NULL);
	return EXIT_SUCCESS;
}

int binary_dict_gen_t::prepare_dict(void)
{
	dictfilename = basefilename + ".dict";
	dictfile.reset(g_fopen(dictfilename.c_str(), "wb"));
	if(!dictfile) {
		g_critical(open_write_file_err, dictfilename.c_str());
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}

int binary_dict_gen_t::prepare_idx(void)
{
	idxfilename = basefilename + ".idx";
	idxfile.reset(g_fopen(idxfilename.c_str(), "wb"));
	if(!idxfile) {
		g_critical(open_write_file_err, idxfilename.c_str());
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}

int binary_dict_gen_t::prepare_syn(void)
{
	synfilename = basefilename + ".syn";
	synfile.reset(g_fopen(synfilename.c_str(), "wb"));
	if(!synfile) {
		g_critical(open_write_file_err, synfilename.c_str());
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}

int binary_dict_gen_t::generate_dict_definition(const article_def_t& def, const std::string& key)
{
	const char type_id = def.type;
	if(g_ascii_islower(type_id)) {
		if(type_id == 'r') {
			if(generate_dict_definition_r(def.resources, key))
				return EXIT_FAILURE;
		} else {
			std::vector<char> buf;
			buf.resize(1 + def.size + 1);
			buf[0] = type_id;
			if(norm_dict->read_data(&buf[1], def.size, def.offset)) {
				return EXIT_FAILURE;
			}
			buf.back() = '\0';
			if(1 != fwrite(&buf[0], buf.size(), 1, get_impl(dictfile))) {
				g_critical(write_file_err, dictfilename.c_str());
				return EXIT_FAILURE;
			}
		}
	} else if(g_ascii_isupper(type_id)) {
		std::vector<char> buf;
		buf.resize(1 + sizeof(guint32) + def.size);
		buf[0] = type_id;
		*reinterpret_cast<guint32*>(&buf[1]) = g_htonl(static_cast<guint32>(def.size));
		if(norm_dict->read_data(&buf[1 + sizeof(guint32)], def.size, def.offset)) {
			return EXIT_FAILURE;
		}
		if(1 != fwrite(&buf[0], buf.size(), 1, get_impl(dictfile))) {
			g_critical(write_file_err, dictfilename.c_str());
			return EXIT_FAILURE;
		}
	} else {
		g_critical(unknown_type_id_err, key.c_str(), type_id);
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}

/* same as generate_dict_definition, but same type sequence is in effect
 * last is true if this is the last definition */
int binary_dict_gen_t::generate_dict_definition_sts(const article_def_t& def,
		const std::string& key, bool last)
{
	const char type_id = def.type;
	if(g_ascii_islower(type_id)) {
		if(type_id == 'r') {
			if(generate_dict_definition_r_sts(def.resources, key, last))
				return EXIT_FAILURE;
		} else {
			std::vector<char> buf;
			buf.resize(def.size + (last ? 0 : 1));
			if(norm_dict->read_data(&buf[0], def.size, def.offset)) {
				return EXIT_FAILURE;
			}
			if(!last)
				buf.back() = '\0';
			if(1 != fwrite(&buf[0], buf.size(), 1, get_impl(dictfile))) {
				g_critical(write_file_err, dictfilename.c_str());
				return EXIT_FAILURE;
			}
		}
	} else if(g_ascii_isupper(type_id)) {
		std::vector<char> buf;
		buf.resize((last ? 0 : sizeof(guint32)) + def.size);
		if(!last)
			*reinterpret_cast<guint32*>(&buf[0]) = g_htonl(static_cast<guint32>(def.size));
		if(norm_dict->read_data(&buf[(last ? 0 : sizeof(guint32))], def.size, def.offset)) {
			return EXIT_FAILURE;
		}
		if(1 != fwrite(&buf[0], buf.size(), 1, get_impl(dictfile))) {
			g_critical(write_file_err, dictfilename.c_str());
			return EXIT_FAILURE;
		}
	} else {
		g_critical(unknown_type_id_err, key.c_str(), type_id);
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}

int binary_dict_gen_t::generate_dict_definition_r(const resource_vect_t& resources, const std::string& key)
{
	std::string str;
	str += 'r';
	for(size_t i=0; i<resources.size(); ++i) {
		if(i>0) {
			str += '\n';
		}
		str += resources[i].type;
		str += ':';
		str += resources[i].key;
	}
	if(1 != fwrite(str.c_str(), str.length() + 1, 1, get_impl(dictfile))) {
		g_critical(write_file_err, dictfilename.c_str());
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}

/* same as generate_dict_definition_r, but same type sequence is in effect
 * last is true if this is the last definition */
int binary_dict_gen_t::generate_dict_definition_r_sts(const resource_vect_t& resources,
		const std::string& key, bool last)
{
	std::string str;
	for(size_t i=0; i<resources.size(); ++i) {
		if(i>0) {
			str += '\n';
		}
		str += resources[i].type;
		str += ':';
		str += resources[i].key;
	}
	if(1 != fwrite(str.c_str(), str.length() + (last ? 0 : 1), 1, get_impl(dictfile))) {
		g_critical(write_file_err, dictfilename.c_str());
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}

int binary_dict_gen_t::generate_index_item(const std::string& key, guint32 offset, guint32 size)
{
	std::vector<char> buf;
	const size_t len = key.length();
	buf.resize(len + 1 + sizeof(guint32)*2);
	memcpy(&buf[0], key.c_str(), len+1);
	*reinterpret_cast<guint32*>(&buf[len+1]) = g_htonl(offset);
	*reinterpret_cast<guint32*>(&buf[len+1+sizeof(guint32)]) = g_htonl(size);
	if(1 != fwrite(&buf[0], buf.size(), 1, get_impl(idxfile))) {
		g_critical(write_file_err, idxfilename.c_str());
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}

/* Decide on whether the same type sequence can be used.
 * Set same_type_sequence and norm_dict->dict_info.*_sametypesequence options. */
void binary_dict_gen_t::decide_on_same_type_sequence(void)
{
	norm_dict->dict_info.unset_sametypesequence();
	same_type_sequence.clear();
	if(!use_same_type_sequence)
		return;
	if(norm_dict->articles.empty())
		return;
	const std::string seq1 = build_type_sequence(norm_dict->articles[0]);
	std::string seq2;
	for(size_t i=1; i<norm_dict->articles.size(); ++i) {
		seq2 = build_type_sequence(norm_dict->articles[i]);
		if(seq1 != seq2) {
			g_message("Can not use same type sequence. "
				"Article '%s' needs type sequence '%s', "
				"while article '%s' needs type sequence '%s'.",
				norm_dict->articles[0].key.c_str(), seq1.c_str(),
				norm_dict->articles[i].key.c_str(), seq2.c_str());
			return;
		}
	}
	same_type_sequence = seq1;
	norm_dict->dict_info.set_sametypesequence(seq1);
	g_message("Using same type sequence '%s'.", same_type_sequence.c_str());
}

std::string binary_dict_gen_t::build_type_sequence(const article_data_t& article) const
{
	std::string seq(article.definitions.size(), ' ');
	for(size_t i=0; i<article.definitions.size(); ++i)
		seq[i] = article.definitions[i].type;
	return seq;
}

