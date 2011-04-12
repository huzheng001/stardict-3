#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <glib/gi18n.h>

#include "full_text_trans.h"

struct TransLanguageInt
{
	const char * name;
	const char * code;
};

struct TransEngineInt {
	const char * name;
	const TransLanguageInt* srclangs;
	const TransLanguageInt** tgtlangs;
	const TransLanguageInt* tgtlangs2;
	const char * website_name;
	const char * website_url;
};

static const TransLanguageInt google_srclangs[] = {
	{ N_("Afrikaans"), "af"},
	{ N_("Albanian"), "sq"},
	{ N_("Arabic"), "ar"},
	{ N_("Belarusian"), "be"},
	{ N_("Bulgarian"), "bg"},
	{ N_("Catalan"), "ca"},
	{ N_("Chinese"), "zh-CN"},
	{ N_("Croatian"), "hr"},
	{ N_("Czech"), "cs"},
	{ N_("Danish"), "da"},
	{ N_("Dutch"), "nl"},
	{ N_("English"), "en"},
	{ N_("Estonian"), "et"},
	{ N_("Filipino"), "tl"},
	{ N_("Finnish"), "fi"},
	{ N_("French"), "fr"},
	{ N_("Galician"), "gl"},
	{ N_("German"), "de"},
	{ N_("Greek"), "el"},
	{ N_("Haitian Creole ALPHA"), "ht"},
	{ N_("Hebrew"), "iw"},
	{ N_("Hindi"), "hi"},
	{ N_("Hungarian"), "hu"},
	{ N_("Icelandic"), "is"},
	{ N_("Indonesian"), "id"},
	{ N_("Irish"), "ga"},
	{ N_("Italian"), "it"},
	{ N_("Japanese"), "ja"},
	{ N_("Korean"), "ko"},
	{ N_("Latvian"), "lv"},
	{ N_("Lithuanian"), "lt"},
	{ N_("Macedonian"), "mk"},
	{ N_("Malay"), "ms"},
	{ N_("Maltese"), "mt"},
	{ N_("Norwegian"), "no"},
	{ N_("Persian"), "fa"},
	{ N_("Polish"), "pl"},
	{ N_("Portuguese"), "pt"},
	{ N_("Romanian"), "ro"},
	{ N_("Russian"), "ru"},
	{ N_("Serbian"), "sr"},
	{ N_("Slovak"), "sk"},
	{ N_("Slovenian"), "sl"},
	{ N_("Spanish"), "es"},
	{ N_("Swahili"), "sw"},
	{ N_("Swedish"), "sv"},
	{ N_("Thai"), "th"},
	{ N_("Turkish"), "tr"},
	{ N_("Ukrainian"), "uk"},
	{ N_("Vietnamese"), "vi"},
	{ N_("Welsh"), "cy"},
	{ N_("Yiddish"), "yi"},
	{ NULL, NULL }
};

static const TransLanguageInt google_tgtlangs[] = {
	{ N_("Afrikaans"), "af"},
	{ N_("Albanian"), "sq"},
	{ N_("Arabic"), "ar"},
	{ N_("Belarusian"), "be"},
	{ N_("Bulgarian"), "bg"},
	{ N_("Catalan"), "ca"},
	{ N_("Chinese (Simplified)"), "zh-CN"},
	{ N_("Chinese (Traditional)"), "zh-TW"},
	{ N_("Croatian"), "hr"},
	{ N_("Czech"), "cs"},
	{ N_("Danish"), "da"},
	{ N_("Dutch"), "nl"},
	{ N_("English"), "en"},
	{ N_("Estonian"), "et"},
	{ N_("Filipino"), "tl"},
	{ N_("Finnish"), "fi"},
	{ N_("French"), "fr"},
	{ N_("Galician"), "gl"},
	{ N_("German"), "de"},
	{ N_("Greek"), "el"},
	{ N_("Haitian Creole ALPHA"), "ht"},
	{ N_("Hebrew"), "iw"},
	{ N_("Hindi"), "hi"},
	{ N_("Hungarian"), "hu"},
	{ N_("Icelandic"), "is"},
	{ N_("Indonesian"), "id"},
	{ N_("Irish"), "ga"},
	{ N_("Italian"), "it"},
	{ N_("Japanese"), "ja"},
	{ N_("Korean"), "ko"},
	{ N_("Latvian"), "lv"},
	{ N_("Lithuanian"), "lt"},
	{ N_("Macedonian"), "mk"},
	{ N_("Malay"), "ms"},
	{ N_("Maltese"), "mt"},
	{ N_("Norwegian"), "no"},
	{ N_("Persian"), "fa"},
	{ N_("Polish"), "pl"},
	{ N_("Portuguese"), "pt"},
	{ N_("Romanian"), "ro"},
	{ N_("Russian"), "ru"},
	{ N_("Serbian"), "sr"},
	{ N_("Slovak"), "sk"},
	{ N_("Slovenian"), "sl"},
	{ N_("Spanish"), "es"},
	{ N_("Swahili"), "sw"},
	{ N_("Swedish"), "sv"},
	{ N_("Thai"), "th"},
	{ N_("Turkish"), "tr"},
	{ N_("Ukrainian"), "uk"},
	{ N_("Vietnamese"), "vi"},
	{ N_("Welsh"), "cy"},
	{ N_("Yiddish"), "yi"},
	{ NULL, NULL }
};

static const TransLanguageInt yahoo_srclangs[] = {
	{ N_("Chinese (Simplified)"), NULL},
	{ N_("Chinese (Traditional)"), NULL},
	{ N_("English"), NULL},
	{ N_("Dutch"), NULL},
	{ N_("French"), NULL},
	{ N_("German"), NULL},
	{ N_("Greek"), NULL},
	{ N_("Italian"), NULL},
	{ N_("Japanese"), NULL},
	{ N_("Korean"), NULL},
	{ N_("Portuguese"), NULL},
	{ N_("Russian"), NULL},
	{ N_("Spanish"), NULL},
	{ NULL}
};

static const TransLanguageInt yahoo_chinese_simplified_tgtlangs[] = {
	{ N_("English"), "zh_en"},
	{ N_("Chinese (Traditional)"), "zh_zt"},
	{ NULL }
};
static const TransLanguageInt yahoo_chinese_traditional_tgtlangs[] = {
	{ N_("English"), "zt_en"},
	{ N_("Chinese (Simplified)"), "zt_zh"},
	{ NULL}
};
static const TransLanguageInt yahoo_english_tgtlangs[] = {
	{ N_("Chinese (Simplified)"), "en_zh"},
	{ N_("Chinese (Traditional)"), "en_zt"},
	{ N_("Dutch"), "en_nl"},
	{ N_("French"), "en_fr"},
	{ N_("German"), "en_de"},
	{ N_("Greek"), "en_el"},
	{ N_("Italian"), "en_it"},
	{ N_("Japanese"), "en_ja"},
	{ N_("Korean"), "en_ko"},
	{ N_("Portuguese"), "en_pt"},
	{ N_("Russian"), "en_ru"},
	{ N_("Spanish"), "en_es"},
	{ NULL}
};
static const TransLanguageInt yahoo_dutch_tgtlangs[] = {
	{ N_("English"), "nl_en"},
	{ N_("French"), "nl_fr"},
	{ NULL}
};
static const TransLanguageInt yahoo_french_tgtlangs[] = {
	{ N_("Dutch"), "fr_nl"},
	{ N_("English"), "fr_en"},
	{ N_("German"), "fr_de"},
	{ N_("Greek"), "fr_el"},
	{ N_("Italian"), "fr_it"},
	{ N_("Portuguese"), "fr_pt"},
	{ N_("Spanish"), "fr_es"},
	{ NULL}
};
static const TransLanguageInt yahoo_german_tgtlangs[] = {
	{ N_("English"), "de_en"},
	{ N_("French"), "de_fr"},
	{ NULL}
};
static const TransLanguageInt yahoo_greek_tgtlangs[] = {
	{ N_("English"), "el_en"},
	{ N_("French"), "el_fr"},
	{ NULL}
};
static const TransLanguageInt yahoo_italian_tgtlangs[] = {
	{ N_("English"), "it_en"},
	{ N_("French"), "it_fr"},
	{ NULL}
};
static const TransLanguageInt yahoo_japanese_tgtlangs[] = {
	{ N_("English"), "ja_en"},
	{ NULL}
};
static const TransLanguageInt yahoo_korean_tgtlangs[] = {
	{ N_("English"), "ko_en"},
	{ NULL}
};
static const TransLanguageInt yahoo_portuguese_tgtlangs[] = {
	{ N_("English"), "pt_en"},
	{ N_("French"), "pt_fr"},
	{ NULL}
};
static const TransLanguageInt yahoo_russian_tgtlangs[] = {
	{ N_("English"), "ru_en"},
	{ NULL}
};
static const TransLanguageInt yahoo_spanish_tgtlangs[] = {
	{ N_("English"), "es_en"},
	{ N_("French"), "es_fr"},
	{ NULL}
};
static const TransLanguageInt* yahoo_tgtlangs[] = {
	yahoo_chinese_simplified_tgtlangs,
	yahoo_chinese_traditional_tgtlangs,
	yahoo_english_tgtlangs,
	yahoo_dutch_tgtlangs,
	yahoo_french_tgtlangs,
	yahoo_german_tgtlangs,
	yahoo_greek_tgtlangs,
	yahoo_italian_tgtlangs,
	yahoo_japanese_tgtlangs,
	yahoo_korean_tgtlangs,
	yahoo_portuguese_tgtlangs,
	yahoo_russian_tgtlangs,
	yahoo_spanish_tgtlangs,
	NULL
};

/*
static const char *altavista_fromlangs[] = {N_("Chinese (Simplified)"), N_("Chinese (Traditional)"), N_("Dutch"), N_("English"), N_("French"), N_("German"), N_("Greek"), N_("Italian"), N_("Japanese"), N_("Korean"), N_("Portuguese"), N_("Russian"), NULL};
static const char *altavista_chinese_simplified_tolangs[] = {N_("English"), NULL};
static const char *altavista_chinese_simplified_code[] = { "zh_en" };
static const char *altavista_chinese_traditional_tolangs[] = {N_("English"), NULL};
static const char *altavista_chinese_traditional_code[] = { "zt_en" };
static const char *altavista_dutch_tolangs[] = {N_("English"), N_("French"), NULL};
static const char *altavista_dutch_code[] = { "nl_en" , "nl_fr"};
static const char *altavista_english_tolangs[] = {N_("Chinese (Simplified)"), N_("Chinese (Traditional)"), N_("Dutch"), N_("French"), N_("German"), N_("Greek"), N_("Italian"), N_("Japanese"), N_("Korean"), N_("Portuguese"), N_("Russian"), N_("Spanish"), NULL};
static const char *altavista_english_code[] = { "en_zh", "en_zt", "en_nl", "en_fr", "en_de", "en_el", "en_it", "en_ja", "en_ko", "en_pt", "en_ru", "en_es" };
static const char *altavista_french_tolangs[] = {N_("Dutch"), N_("English"), N_("German"), N_("Greek"), N_("Italian"), N_("Portuguese"), N_("Spanish"), NULL };
static const char *altavista_french_code[] = { "fr_nl", "fr_en", "fr_de", "fr_el", "fr_it", "fr_pt", "fr_es"};
static const char *altavista_german_tolangs[] = {N_("English"), N_("French"), NULL};
static const char *altavista_german_code[] = { "de_en", "de_fr" };
static const char *altavista_greek_tolangs[] = {N_("English"), N_("French"), NULL};
static const char *altavista_greek_code[] = { "el_en", "el_fr" };
static const char *altavista_italian_tolangs[] = {N_("English"), N_("French"), NULL};
static const char *altavista_italian_code[] = { "it_en", "it_fr" };
static const char *altavista_japanese_tolangs[] = {N_("English"), NULL};
static const char *altavista_japanese_code[] = { "ja_en" };
static const char *altavista_korean_tolangs[] = {N_("English"), NULL};
static const char *altavista_korean_code[] = { "ko_en" };
static const char *altavista_portuguese_tolangs[] = {N_("English"), N_("French"), NULL};
static const char *altavista_portuguese_code[] = { "pt_en", "pt_fr" };
static const char *altavista_spanish_tolangs[] = {N_("English"), N_("French"), NULL};
static const char *altavista_spanish_code[] = { "es_en", "es_fr" };
static const char **altavista_tolangs[] = {altavista_chinese_simplified_tolangs, altavista_chinese_traditional_tolangs, altavista_dutch_tolangs, altavista_english_tolangs, altavista_french_tolangs, altavista_german_tolangs, altavista_greek_tolangs, altavista_italian_tolangs, altavista_japanese_tolangs, altavista_korean_tolangs, altavista_portuguese_tolangs, altavista_spanish_tolangs};
static const char **altavista_code[] = {altavista_chinese_simplified_code,altavista_chinese_traditional_code,altavista_dutch_code,altavista_english_code, altavista_french_code, altavista_german_code, altavista_greek_code, altavista_italian_code, altavista_japanese_code, altavista_korean_code, altavista_portuguese_code, altavista_spanish_code};
*/
/*
static const char *systranbox_fromlangs[] = {N_("Chinese (Simplified)"), N_("Chinese (Traditional)"), N_("Dutch"), N_("English"), N_("French"), N_("German"), N_("Swedish"), N_("Italian"), N_("Japanese"), N_("Korean"), N_("Portuguese"), N_("Russian"), NULL};
static const char *systranbox_chinese_simplified_tolangs[] = {N_("English"), NULL};
static const char *systranbox_chinese_simplified_code[] = { "zh_en" };
static const char *systranbox_chinese_traditional_tolangs[] = {N_("English"), NULL};
static const char *systranbox_chinese_traditional_code[] = { "zt_en" };
static const char *systranbox_dutch_tolangs[] = {N_("English"), N_("French"), NULL};
static const char *systranbox_dutch_code[] = { "nl_en" , "nl_fr"};
static const char *systranbox_english_tolangs[] = {N_("Chinese (Simplified)"), N_("Chinese (Traditional)"), N_("Dutch"), N_("French"), N_("German"), N_("Swedish"), N_("Italian"), N_("Japanese"), N_("Korean"), N_("Portuguese"), N_("Russian"), N_("Spanish"), NULL};
static const char *systranbox_english_code[] = { "en_zh", "en_zt", "en_nl", "en_fr", "en_de", "en_sv", "en_it", "en_ja", "en_ko", "en_pt", "en_ru", "en_es" };
static const char *systranbox_french_tolangs[] = {N_("Dutch"), N_("English"), N_("German"), N_("Greek"), N_("Italian"), N_("Portuguese"), N_("Spanish"), NULL };
static const char *systranbox_french_code[] = { "fr_nl", "fr_en", "fr_de", "fr_el", "fr_it", "fr_pt", "fr_es"};
static const char *systranbox_german_tolangs[] = {N_("English"), N_("French"), NULL};
static const char *systranbox_german_code[] = { "de_en", "de_fr" };
static const char *systranbox_swedish_tolangs[] = {N_("English"), NULL};
static const char *systranbox_swedish_code[] = { "sv_en" };
static const char *systranbox_italian_tolangs[] = {N_("English"), N_("French"), NULL};
static const char *systranbox_italian_code[] = { "it_en", "it_fr" };
static const char *systranbox_japanese_tolangs[] = {N_("English"), NULL};
static const char *systranbox_japanese_code[] = { "ja_en" };
static const char *systranbox_korean_tolangs[] = {N_("English"), NULL};
static const char *systranbox_korean_code[] = { "ko_en" };
static const char *systranbox_portuguese_tolangs[] = {N_("English"), N_("French"), NULL};
static const char *systranbox_portuguese_code[] = { "pt_en", "pt_fr" };
static const char *systranbox_spanish_tolangs[] = {N_("English"), N_("French"), NULL};
static const char *systranbox_spanish_code[] = { "es_en", "es_fr" };
static const char **systranbox_tolangs[] = {systranbox_chinese_simplified_tolangs, systranbox_chinese_traditional_tolangs, systranbox_dutch_tolangs, systranbox_english_tolangs, systranbox_french_tolangs, systranbox_german_tolangs, systranbox_swedish_tolangs, systranbox_italian_tolangs, systranbox_japanese_tolangs, systranbox_korean_tolangs, systranbox_portuguese_tolangs, systranbox_spanish_tolangs};
static const char **systranbox_code[] = {systranbox_chinese_simplified_code,systranbox_chinese_traditional_code,systranbox_dutch_code,systranbox_english_code, systranbox_french_code, systranbox_german_code, systranbox_swedish_code, systranbox_italian_code, systranbox_japanese_code, systranbox_korean_code, systranbox_portuguese_code, systranbox_spanish_code};
*/

static const TransLanguageInt excite_srclangs[] = {
	{ N_("Chinese (Simplified)"), NULL},
	{ N_("Chinese (Traditional)"), NULL},
	{ N_("English"), NULL},
	{ N_("Japanese"), NULL},
	{ N_("Korean"), NULL},
	{ NULL}
};
static const TransLanguageInt excite_chinese_simplified_tgtlangs[] = {
	{ N_("Japanese"), "CHJA"},
	{ NULL}
};
static const TransLanguageInt excite_chinese_traditional_tgtlangs[] = {
	{ N_("Japanese"), "CHJA"},
	{ NULL}
};
static const TransLanguageInt excite_english_tgtlangs[] = {
	{ N_("Japanese"), "ENJA"},
	{ NULL}
};
static const TransLanguageInt excite_japanese_tgtlangs[] = {
	{ N_("English"), "JAEN"},
	{ N_("Korean"), "JAKO"},
	{ N_("Chinese (Simplified)"), "JACH"},
	{ N_("Chinese (Traditional)"), "JACH"},
	{ NULL}
};
static const TransLanguageInt excite_korean_tgtlangs[] = {
	{ N_("Japanese"), "KOJA"},
	{ NULL}
};
static const TransLanguageInt* excite_tgtlangs[] = {
	excite_chinese_simplified_tgtlangs,
	excite_chinese_traditional_tgtlangs,
	excite_english_tgtlangs,
	excite_japanese_tgtlangs,
	excite_korean_tgtlangs,
	NULL
};

/*
static const char *kingsoft_fromlangs[] = {N_("English"), N_("Chinese (Simplified)"), N_("Chinese (Traditional)"), N_("Japanese"), NULL };
static const char *kingsoft_english_tolangs[] = {N_("Chinese (Simplified)"), N_("Chinese (Traditional)"),NULL};
static const char *kingsoft_chinese_simplified_tolangs[] = {N_("English"), NULL};
static const char *kingsoft_chinese_traditional_tolangs[] = {N_("English"), NULL};
static const char *kingsoft_japanese_tolangs[] = {N_("Chinese (Simplified)"), N_("Chinese (Traditional)"),NULL};
static const char **kingsoft_tolangs[] = {kingsoft_english_tolangs, kingsoft_chinese_simplified_tolangs, kingsoft_chinese_traditional_tolangs, kingsoft_japanese_tolangs};
*/


/* keep in sync with enum TranslateEngineCode! */
TransEngineInt trans_engines[] = {
	// name,                         srclangs,            tgtlangs,            tgtlangs2,        website_name,   website_url
	{ N_("Google Translate"),        google_srclangs,     NULL,                google_tgtlangs,  "Google",       "http://translate.google.com"},
	{ N_("Yahoo Translate"),         yahoo_srclangs,      yahoo_tgtlangs,      NULL,             "Yahoo",        "http://babelfish.yahoo.com"},
	{ N_("Excite Japan Translate"),  excite_srclangs,     excite_tgtlangs,     NULL,             "Excite Japan", "http://www.excite.co.jp"},
	{ NULL }
};

std::string TransEngine::get_source_lang(size_t src_lang) const
{
	if(src_lang >= srclangs.size())
		return "";
	return srclangs[src_lang].name;
}

size_t TransEngine::get_source_lang_cnt() const
{
	return srclangs.size();
}

std::string TransEngine::get_target_lang(size_t src_lang, size_t tgt_lang) const
{
	if(src_lang >= srclangs.size())
		return "";
	const size_t tolangind = srclangs[src_lang].tolangind;
	if(tgt_lang >= tgtlangs[tolangind].size())
		return "";
	return tgtlangs[tolangind][tgt_lang].name;
}

size_t TransEngine::get_target_lang_cnt(size_t src_lang) const
{
	if(src_lang >= srclangs.size())
		return 0;
	const size_t tolangind = srclangs[src_lang].tolangind;
	return tgtlangs[tolangind].size();
}

FullTextTrans::FullTextTrans()
{
	for(size_t engine=0; engine<TranslateEngine_Size; ++engine) {
		engines[engine].name = gettext(trans_engines[engine].name);
		engines[engine].website_name = trans_engines[engine].website_name;
		engines[engine].website_url = trans_engines[engine].website_url;
		init_engine(engines[engine], trans_engines[engine]);
	}
}

const TransEngine& FullTextTrans::get_engine(size_t engine_ind) const
{
	return engines[engine_ind];
}

void FullTextTrans::GetHostFile(
	size_t engine_index, size_t fromlang_index, size_t tolang_index,
	std::string &host, std::string &file, const char *text) const
{
	if(engine_index==TranslateEngine_Google){
		host = "translate.google.com";
		file = "/translate_t?ie=UTF-8";
	} else if(engine_index==TranslateEngine_Yahoo){
		host = "babelfish.yahoo.com";
		file = "/translate_txt?ei=UTF-8&lp=";
	/*}else if(engine_index==TranslateEngine_SystranBox){
		host = "www.systranbox.com";
		file = "/systran/box?systran_id=SystranSoft-en&systran_charset=UTF-8&systran_lp="; */
	} else if(engine_index==TranslateEngine_ExciteJapan){
		host = "www.excite.co.jp";
		file = "/world";
	} /*else if(engine_index==5){
		host = "fy.iciba.com";
		file = "/?langpair=";
	} */

	if(engine_index==TranslateEngine_Google) {
		file += "&sl=";
		file += engines[engine_index].srclangs[fromlang_index].code;
		file += "&tl=";
		const size_t tolangind = engines[engine_index].srclangs[fromlang_index].tolangind;
		file += engines[engine_index].tgtlangs[tolangind][tolang_index].code;
	} else {
		const size_t tolangind = engines[engine_index].srclangs[fromlang_index].tolangind;
		std::string lang_code = engines[engine_index].tgtlangs[tolangind][tolang_index].code;
		if(engine_index==TranslateEngine_ExciteJapan) {
			if(strcmp(lang_code.c_str(),"KOJA")==0 || strcmp(lang_code.c_str(),"JAKO")==0){
				file += "/korean?wb_lp=";
			}else if(strcmp(lang_code.c_str(),"ENJA")==0 || strcmp(lang_code.c_str(),"JAEN")==0){
				file += "/english?wb_lp=";
			}else if(strcmp(lang_code.c_str(),"CHJA")==0 || strcmp(lang_code.c_str(),"JACH")==0){
				file += "/chinese?wb_lp=";
			}else{
				file += "/english?wb_lp=";
			}
		}
		file += lang_code;
	}
	if (engine_index == TranslateEngine_Google || engine_index == TranslateEngine_Yahoo) {
		file += "&text=";
		file += text;
	/*}else if(engine_index == TranslateEngine_SystranBox) {
		file += "&systran_text=";
		file += text; */
	} else if(engine_index == TranslateEngine_ExciteJapan) {
		file += "&before=";
		file += text;
	}
}

void FullTextTrans::init_engine(TransEngine& engine, const TransEngineInt& engine_src)
{
	engine.srclangs.resize(calculate_cnt(engine_src.srclangs));
	for(size_t i = 0; i < engine.srclangs.size(); ++i) {
		engine.srclangs[i].name = gettext(engine_src.srclangs[i].name);
		if(engine_src.srclangs[i].code)
			engine.srclangs[i].code = engine_src.srclangs[i].code;
		if(engine_src.tgtlangs2) {
			if(engine.tgtlangs.empty()) {
				engine.tgtlangs.resize(1);
				std::vector<TransLanguage>& langs = engine.tgtlangs[0];
				langs.resize(calculate_cnt(engine_src.tgtlangs2));
				for(size_t j = 0; engine_src.tgtlangs2[j].name; ++j) {
					langs[j].name = gettext(engine_src.tgtlangs2[j].name);
					langs[j].code = engine_src.tgtlangs2[j].code;
				}
			}
			engine.srclangs[i].tolangind = 0;
		} else if(engine_src.tgtlangs) {
			if(engine.tgtlangs.empty())
				engine.tgtlangs.resize(engine.srclangs.size());
			std::vector<TransLanguage>& langs = engine.tgtlangs[i];
			langs.resize(calculate_cnt(engine_src.tgtlangs[i]));
			for(size_t j=0; engine_src.tgtlangs[i][j].name; ++j) {
				langs[j].name = gettext(engine_src.tgtlangs[i][j].name);
				langs[j].code = engine_src.tgtlangs[i][j].code;
			}
			engine.srclangs[i].tolangind = i;
		}
	}
}

size_t FullTextTrans::calculate_cnt(const char** arr)
{
	size_t cnt = 0;
	while(arr[cnt])
		++cnt;
	return cnt;
}

size_t FullTextTrans::calculate_cnt(const TransLanguageInt* arr)
{
	size_t cnt = 0;
	while(arr[cnt].name)
		++cnt;
	return cnt;
}
