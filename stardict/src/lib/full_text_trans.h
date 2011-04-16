#ifndef _FULL_TEXT_TRANS_H_
#define _FULL_TEXT_TRANS_H_

#include <string>
#include <vector>

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

class TransEngineInt;
class TransLanguageInt;

class FullTextTrans
{
public:
	FullTextTrans();
	const TransEngine& get_engine(size_t engine_ind) const;
	void GetHostFile(size_t engine_index, size_t fromlang_index, size_t tolang_index,
		std::string &host, std::string &file, const char *text) const;
private:
	void init_engine(TransEngine& engine, const TransEngineInt& engine_src);
	void sort_engine(TransEngine& engine);
	static bool trans_engine_comp(const TransLanguage& left, const TransLanguage& right);
	// calculate size of a NULL-terminated array
	static size_t calculate_cnt(const char** arr);
	static size_t calculate_cnt(const TransLanguageInt* arr);
	TransEngine engines[TranslateEngine_Size];
};

#endif  // _FULL_TEXT_TRANS_H_
