#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <cstring>
#include <iostream>
#include <vector>
#include <algorithm>
#include "lib_stardict_text2bin.h"
#include "ifo_file.hpp"
#include "resourcewrap.hpp"
#include "libcommon.h"
#include "lib_chars.h"
#include "lib_common_dict.h"
#include "lib_textual_dict_parser.h"

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

static bool compare_article_data_by_key(const article_data_t& left, const article_data_t& right)
{
	return 0 > stardict_strcmp(left.key.c_str(), right.key.c_str());
}

/* generate binary normal dictionary */
class binary_dict_gen_t
{
public:
	binary_dict_gen_t(void);
	int generate(const std::string& ifofilename, common_dict_t *norm_dict);
	void clear(void);
	void set_print_info(print_info_t print_info)
	{
		this->print_info = print_info;
	}
	void set_use_same_type_sequence(bool b)
	{
		use_same_type_sequence = b;
	}
	void set_compress_dict(bool b)
	{
		compress_dict = b;
	}
private:
	int generate_dict_and_idx(void);
	int generate_syn(void);
	int prepare_dict(void);
	int prepare_idx(void);
	int prepare_syn(void);
	int generate_dict_definition(const article_def_t& def, const std::string& key);
	int generate_dict_definition_sts(const article_def_t& def, const std::string& key, bool last);
	int generate_dict_definition_r(const resource_vect_t& resources, const std::string& key);
	int generate_dict_definition_r_sts(const resource_vect_t& resources, const std::string& key, bool last);
	int generate_index_item(const std::string& key, guint32 offset, guint32 size);
	void decide_on_same_type_sequence(void);
	std::string build_type_sequence(const article_data_t& article) const;
	print_info_t print_info;
	common_dict_t *norm_dict;
	std::string basefilename;
	std::string ifofilename;
	std::string dictfilename;
	std::string idxfilename;
	std::string synfilename;
	clib::File dictfile;
	clib::File idxfile;
	clib::File synfile;
	/* use same_type_sequence if possible */
	bool use_same_type_sequence;
	/* If use_same_type_sequence == true and all articles have the same sequence of types,
	 * this string contains the sequence of types to use.
	 * Otherwise this is an empty string. */
	std::string same_type_sequence;
	/* run dictzip on the generated file if enabled */
	bool compress_dict;
};

binary_dict_gen_t::binary_dict_gen_t(void)
:
	print_info(NULL),
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
	if(!g_str_has_suffix(ifofilename.c_str(), ".ifo")) {
		print_info("Unsupported file type. File must have 'ifo' extension. file = %s.\n",
			ifofilename.c_str());
		return EXIT_FAILURE;
	}
	basefilename.assign(ifofilename, 0, ifofilename.length() - (sizeof(".ifo")-1));
	decide_on_same_type_sequence();
	if(generate_dict_and_idx())
		return EXIT_FAILURE;
#ifndef _WIN32
	if(compress_dict) {
		std::string command(std::string("dictzip ") + dictfilename);
		system(command.c_str());
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
			print_info(write_file_err, synfilename.c_str());
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
		print_info(open_write_file_err, dictfilename.c_str());
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}

int binary_dict_gen_t::prepare_idx(void)
{
	idxfilename = basefilename + ".idx";
	idxfile.reset(g_fopen(idxfilename.c_str(), "wb"));
	if(!idxfile) {
		print_info(open_write_file_err, idxfilename.c_str());
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}

int binary_dict_gen_t::prepare_syn(void)
{
	synfilename = basefilename + ".syn";
	synfile.reset(g_fopen(synfilename.c_str(), "wb"));
	if(!synfile) {
		print_info(open_write_file_err, synfilename.c_str());
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
				print_info(write_file_err, dictfilename.c_str());
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
			print_info(write_file_err, dictfilename.c_str());
			return EXIT_FAILURE;
		}
	} else {
		print_info(unknown_type_id_err, key.c_str(), type_id);
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
				print_info(write_file_err, dictfilename.c_str());
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
			print_info(write_file_err, dictfilename.c_str());
			return EXIT_FAILURE;
		}
	} else {
		print_info(unknown_type_id_err, key.c_str(), type_id);
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
		print_info(write_file_err, dictfilename.c_str());
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
		print_info(write_file_err, dictfilename.c_str());
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
		print_info(write_file_err, idxfilename.c_str());
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
			print_info("Can not use same type sequence. "
				"Article '%s' needs type sequence '%s', "
				"while article '%s' needs type sequence '%s'.\n",
				norm_dict->articles[0].key.c_str(), seq1.c_str(),
				norm_dict->articles[i].key.c_str(), seq2.c_str());
			return;
		}
	}
	same_type_sequence = seq1;
	norm_dict->dict_info.set_sametypesequence(seq1);
	print_info("Using same type sequence '%s'.\n", same_type_sequence.c_str());
}

std::string binary_dict_gen_t::build_type_sequence(const article_data_t& article) const
{
	std::string seq;
	for(size_t i=0; i<article.definitions.size(); ++i)
		seq += article.definitions[i].type;
	return seq;
}

int stardict_text2bin(const std::string& xmlfilename, const std::string& ifofilename,
		print_info_t print_info, bool show_xincludes, bool use_same_type_sequence)
{
	common_dict_t norm_dict;
	norm_dict.set_print_info(print_info);
	if(parse_textual_dict(xmlfilename, &norm_dict,
			print_info, show_xincludes))
		return EXIT_FAILURE;
	std::sort(norm_dict.articles.begin(), norm_dict.articles.end(), compare_article_data_by_key);
	binary_dict_gen_t generator;
	generator.set_print_info(print_info);
	generator.set_use_same_type_sequence(use_same_type_sequence);
	generator.set_compress_dict(true);
	print_info("Generating dictionary %s...\n", ifofilename.c_str());
	if(generator.generate(ifofilename, &norm_dict)) {
		print_info("Generation failed.\n");
		return EXIT_FAILURE;
	}
	print_info("Generation done.\n");
	return EXIT_SUCCESS;
}
