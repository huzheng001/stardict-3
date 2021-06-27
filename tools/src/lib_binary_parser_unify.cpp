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

#include <string>
#include <algorithm>
#include <vector>
#include "lib_binary_parser_unify.h"

struct article_t
{
	std::string key;
	std::vector<std::string> synonyms;
	// data in dictionary
	guint32 offset;
	guint32 size;
	int add_key(const std::string& new_key);
	int add_synonym(const std::string& new_synonym);
	void merge(const article_t& other);
	static void merge(std::vector<std::string>& first, const std::vector<std::string>& second);
};

static bool compare_articles_by_offset_size(const article_t& left, const article_t& right)
{
	if(left.offset < right.offset)
		return true;
	if(left.offset > right.offset)
		return false;
	return left.size < right.size;
}

static bool compare_articles_by_word(const article_t& left, const article_t& right)
{
	return 0 > stardict_strcmp(left.key.c_str(), right.key.c_str());
}

static bool compare_articles_by_word_offset_size(const article_t& left, const article_t& right)
{
	const gint res = stardict_strcmp(left.key.c_str(), right.key.c_str());
	if(res < 0)
		return true;
	if(res > 0)
		return false;
	return compare_articles_by_offset_size(left, right);
}

/* Return value:
 * EXIT_FAILURE - key already exists
 * EXIT_SUCCESS - key added
 * */
int article_t::add_key(const std::string& new_key)
{
	if(new_key.empty())
		return EXIT_FAILURE;
	if(key == new_key)
		return EXIT_FAILURE;
	if(std::find(synonyms.begin(), synonyms.end(), new_key) != synonyms.end())
		return EXIT_FAILURE;
	if(key.empty()) {
		key = new_key;
		return EXIT_SUCCESS;
	}
	synonyms.push_back(new_key);
	return EXIT_SUCCESS;
}

/* Return value:
 * EXIT_FAILURE - key or synonym already exists
 * EXIT_SUCCESS - synonym added
 * */
int article_t::add_synonym(const std::string& new_synonym)
{
	if(new_synonym.empty())
		return EXIT_FAILURE;
	if(key == new_synonym)
		return EXIT_FAILURE;
	if(std::find(synonyms.begin(), synonyms.end(), new_synonym) != synonyms.end())
		return EXIT_FAILURE;
	synonyms.push_back(new_synonym);
	return EXIT_SUCCESS;
}

void article_t::merge(const article_t& other)
{
	if(other.key.empty())
		return;
	if(key.empty()) {
		*this = other;
		return;
	}
	merge(synonyms, other.synonyms);
}

/* copy all items from second to first skipping items that are already in first */
void article_t::merge(std::vector<std::string>& first, const std::vector<std::string>& second)
{
	for(size_t i=0; i<second.size(); ++i)
		if(first.end() == std::find(first.begin(), first.end(), second[i]))
			first.push_back(second[i]);
}

static void prepare_articles(std::vector<article_t>& articles, const binary_dict_parser_t& dict)
{
	const binary_dict_parser_t::worditem_vect_t& worditems = dict.get_worditems();
	articles.resize(worditems.size());
	/* Make a copy of worditems. We cannot drop empty words here,
	synitems refers to worditems by index. */
	for(size_t i=0; i<worditems.size(); ++i) {
		if(worditems[i].word.empty())
			continue;
		articles[i].key = worditems[i].word;
		articles[i].offset = worditems[i].offset;
		articles[i].size = worditems[i].size;
	}
	const binary_dict_parser_t::synitem_vect_t& synitems = dict.get_synitems();
	for(size_t i=0; i<synitems.size(); ++i) {
		if(!synitems[i].word.empty()) {
			g_assert(synitems[i].index < articles.size());
			articles[synitems[i].index].add_synonym(synitems[i].word);
		}
	}
	// merge articles referring to the same data
	std::sort(articles.begin(), articles.end(), compare_articles_by_offset_size);
	for(size_t i=0,j; i<articles.size(); i=j) {
		for(j=i+1; j<articles.size()
			&& articles[i].offset == articles[j].offset
			&& articles[i].size == articles[j].size
			; ++j) {
			articles[i].merge(articles[j]);
			articles[j].key.clear();
		}
	}
	// sort articles by key
	std::sort(articles.begin(), articles.end(), compare_articles_by_word_offset_size);
	// remove empty articles
	article_t empty_article;
	typedef std::vector<article_t>::iterator article_iter_t;
	std::pair<article_iter_t, article_iter_t> range
		= std::equal_range(articles.begin(), articles.end(), empty_article, compare_articles_by_word);
	articles.erase(range.first, range.second);
}

static int convert_to_parsed_dict_field(article_data_t& article_data, common_dict_t& parsed_norm_dict,
		const data_field_t& field, const std::string& key)
{
	const char type_id = field.type_id;
	if(g_ascii_islower(type_id)) {
		if(type_id == 'r') {
			dictionary_data_block data_block;
			data_block.set_fix_errors(true);
			data_block.set_word(key.c_str());
			resitem_vect_t items;
			if(VERIF_RESULT_FATAL <= data_block.verify_field_content_r(field.get_data(), field.get_size(), &items))
				return EXIT_FAILURE;
			if(items.empty())
				return EXIT_FAILURE;
			article_def_t article_def;
			article_def.type = 'r';
			for(size_t k=0; k<items.size(); ++k) {
				if(article_def.add_resource(resource_t(items[k].type, items[k].key)))
					return EXIT_FAILURE;
			}
			if(article_data.add_definition(article_def))
				return EXIT_FAILURE;
		} else {
			size_t offset;
			if(parsed_norm_dict.write_data(field.get_data(), field.get_size(), offset))
				return EXIT_FAILURE;
			if(article_data.add_definition(article_def_t(type_id, offset, field.get_size())))
				return EXIT_FAILURE;
		}
	} else {
		g_message("Index item %s, type = '%c'. Binary data types are not supported presently. Skipping.",
			key.c_str(), type_id);
	}
	return EXIT_SUCCESS;
}

int convert_to_parsed_dict(common_dict_t& parsed_norm_dict, const binary_dict_parser_t& norm_dict)
{
	std::vector<article_t> articles;
	prepare_articles(articles, norm_dict);

	parsed_norm_dict.reset();
	parsed_norm_dict.dict_info = norm_dict.get_dict_info();
	for(size_t i=0; i<articles.size(); ++i) {
		const article_t& article = articles[i];
		article_data_t article_data;
		article_data.key = article.key;
		article_data.synonyms = article.synonyms;

		data_field_vect_t fields;
		if(norm_dict.get_data_fields(article.offset, article.size, fields)) {
			g_critical("Failed to load data for field %s.", article.key.c_str());
			return EXIT_FAILURE;
		}
		for(size_t j=0; j<fields.size(); ++j) {
			if(convert_to_parsed_dict_field(article_data, parsed_norm_dict,
					fields[j], article.key))
				return EXIT_FAILURE;
		}
		if(article_data.definitions.empty()) {
			g_critical("Index item %s. Empty definition list.", article.key.c_str());
			return EXIT_FAILURE;
		}
		if(parsed_norm_dict.add_article(article_data))
			return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}

