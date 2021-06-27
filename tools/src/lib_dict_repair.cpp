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
#include <vector>
#include <algorithm>
#include "lib_dict_repair.h"
#include "lib_chars.h"

static bool compare_article_data_by_key(const article_data_t& left, const article_data_t& right)
{
	return 0 > stardict_strcmp(left.key.c_str(), right.key.c_str());
}

static void repair_text_data(std::string& text)
{
	if(!g_utf8_validate(text.c_str(), -1, NULL)) {
		text = fix_utf8_str(text, 0);
	}
	typedef std::list<const char*> str_list_t;
	str_list_t invalid_chars;
	if(check_xml_string_chars(text.c_str(), invalid_chars)) {
		std::string tmp;
		fix_xml_string_chars(text.c_str(), tmp);
		text = tmp;
	}
}

static void repair_key(std::string& key)
{
	if(key.empty())
		return;
	repair_text_data(key);
	if(key.length()>=(size_t)MAX_INDEX_KEY_SIZE) {
		size_t wordlen = truncate_utf8_string(key.c_str(), key.length(), MAX_INDEX_KEY_SIZE-1);
		key.resize(wordlen);
	}
	if(!key.empty()) {
		if(g_ascii_isspace(key[0]) || g_ascii_isspace(key[key.length()-1])) {
			const char* new_beg = NULL;
			size_t new_len;
			trim_spaces(key.c_str(), new_beg, new_len);
			std::string new_key(new_beg, new_len);
			key = new_key;
		}
	}
	if (check_stardict_key_chars(key.c_str())) {
		std::string tmp;
		fix_stardict_key_chars(key.c_str(), tmp);
		key = tmp;
	}
}

/* return value:
 * EXIT_FAILURE - unrecoverable error occurred, for example file read error.
 * Errors related to article contents are do not lead to EXIT_FAILURE.
 * In case the article contents is broken and cannot be recovered
 * we clear the article key, that in practise mean that this article will ignored. */
static int repair_article(article_data_t& article, common_dict_t& norm_dict)
{
	repair_key(article.key);
	// We check that the key is blank after processing synonyms
	// synonyms
	{
		std::vector<std::string> synonyms2;
		synonyms2.reserve(article.synonyms.size());
		for(std::vector<std::string>::iterator it=article.synonyms.begin(); it!=article.synonyms.end(); ++it) {
			repair_key(*it);
			if(it->empty())
				continue;
			if(*it == article.key)
				continue;
			// ignore duplicates
			if(std::find(article.synonyms.begin(), it, *it) != it)
				continue;
			synonyms2.push_back(*it);
		}
		std::swap(article.synonyms, synonyms2);
	}
	if(article.key.empty()) {
		if(article.synonyms.empty())
			return EXIT_SUCCESS;
		// if the key is empty, replace it with the first synonym
		article.key = article.synonyms[0];
		article.synonyms.erase(article.synonyms.begin());
	}
	// definitions
	{
		std::vector<article_def_t> defs2;
		std::vector<char> buf;
		defs2.reserve(article.definitions.size());
		for(std::vector<article_def_t>::iterator it=article.definitions.begin(); it!=article.definitions.end(); ++it) {
			if(it->type == 'r') {
				if(it->resources.empty())
					continue;
				defs2.push_back(*it);
				continue;
			}
			if(it->size == 0)
				continue;
			if(g_ascii_isupper(it->type)) {
				defs2.push_back(*it);
				continue;
			}
			if(g_ascii_islower(it->type)) {
				buf.resize(it->size);
				if(norm_dict.read_data(&buf[0], it->size, it->offset))
					return EXIT_FAILURE;
				std::string def(&buf[0], buf.size());
				const std::string def_orig(def);
				repair_text_data(def);
				if(def.empty())
					continue;
				if(def != def_orig) {
					size_t offset;
					if(norm_dict.write_data(def.c_str(), def.length(), offset))
						return EXIT_FAILURE;
					it->size = def.length();
					it->offset = offset;
				}
				defs2.push_back(*it);
				continue;
			}
			// unknown type
		}
		std::swap(article.definitions, defs2);
		if(article.definitions.empty()) {
			article.key.clear();
			return EXIT_SUCCESS;
		}
	}
	return EXIT_SUCCESS;
}

int repair_dict(common_dict_t& norm_dict)
{
	for(std::vector<article_data_t>::iterator it=norm_dict.articles.begin(); it!=norm_dict.articles.end(); ++it)
		if(repair_article(*it, norm_dict))
			return EXIT_FAILURE;
	std::sort(norm_dict.articles.begin(), norm_dict.articles.end(), compare_article_data_by_key);
	// remove empty articles
	article_data_t empty_article;
	typedef std::vector<article_data_t>::iterator article_iter_t;
	std::pair<article_iter_t, article_iter_t> range
		= std::equal_range(norm_dict.articles.begin(), norm_dict.articles.end(), empty_article,
			compare_article_data_by_key);
	norm_dict.articles.erase(range.first, range.second);
	if(norm_dict.articles.empty()) {
		g_critical("Dictionary contains no articles");
		return EXIT_FAILURE;
	}
	norm_dict.dict_info.set_wordcount(norm_dict.articles.size());
	return EXIT_SUCCESS;
}
