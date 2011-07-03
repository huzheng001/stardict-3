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

#ifndef _FULL_TEXT_TRANS_H_
#define _FULL_TEXT_TRANS_H_

#include <string>
#include <vector>
#include "httpmanager.h"

enum TranslateEngineCode {
	TranslateEngine_Google,
	TranslateEngine_Yahoo,
	// TranslateEngine_Altavista,
	//TranslateEngine_SystranBox,
	TranslateEngine_ExciteJapan,
	TranslateEngine_Size // number of active engines. Must be the last item.
};

class TransLanguage
{
public:
	// language name, translatable
	std::string name;
	// language code for Translation Engine. It may be empty.
	std::string code;
};

class SrcTransLanguage: public TransLanguage
{
public:
	// List of target languages available for the current source language
	// (index in the tgtlangs array).
	size_t tolangind;
};

class FullTextTrans;

class TransEngine
{
	friend class FullTextTrans;
private:
	// Full-Text Translation engine name, translatable.
	std::string name;
	// source languages
	std::vector<SrcTransLanguage> srclangs;
	// target languages store
	std::vector<std::vector<TransLanguage> > tgtlangs;
	std::string website_name;
	std::string website_url;
public:
	std::string get_source_lang(size_t src_lang) const;
	size_t get_source_lang_cnt() const;
	std::string get_target_lang(size_t src_lang, size_t tgt_lang) const;
	size_t get_target_lang_cnt(size_t src_lang) const;
	const std::string& get_name() const
	{
		return name;
	}
	const std::string& get_website_name() const
	{
		return website_name;
	}
	const std::string& get_website_url() const
	{
		return website_url;
	}
	/* Returns true if the target language does not depend on the source language. */
	bool independent_target_lang(void) const
	{
		return tgtlangs.size() == 1;
	}
};

struct TransEngineInt;
struct TransLanguageInt;

class FullTextTrans
{
public:
	sigc::signal<void, const char *> on_error_;
	sigc::signal<void, const char *> on_response_;

	FullTextTrans();
	const TransEngine& get_engine(size_t engine_ind) const;
	void Translate(size_t engine_index, size_t fromlang_index, size_t tolang_index,
		const char *text);
private:
	void build_request(size_t engine_index, size_t fromlang_index, size_t tolang_index,
		const char *text,
		HttpMethod& httpMethod, std::string &host, std::string &file,
		std::string& headers, std::string& body, bool& allow_absolute_URI) const;
	void init_engine(TransEngine& engine, const TransEngineInt& engine_src);
	void sort_engine(TransEngine& engine);
	static bool trans_engine_comp(const TransLanguage& left, const TransLanguage& right);
	void on_http_client_error(HttpClient* http_client, const char *error_msg);
	void on_http_client_response(HttpClient* http_client);
	void parse_response(const char* buffer, size_t buffer_len, glong engine_index);
	// calculate size of a NULL-terminated array
	static size_t calculate_cnt(const char** arr);
	static size_t calculate_cnt(const TransLanguageInt* arr);
	TransEngine engines[TranslateEngine_Size];
	HttpManager oHttpManager;
};

#endif  // _FULL_TEXT_TRANS_H_
