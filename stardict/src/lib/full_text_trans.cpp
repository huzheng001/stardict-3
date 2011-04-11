#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <glib/gi18n.h>

#include "full_text_trans.h"

struct TransEngineInt {
	const char * name;
	const char ** fromlangs;
	const char *** tolangs;
	const char ** tolangs2;
	const char *** code;
	const char ** fromcode;
	const char ** tocode;
	const char * website_name;
	const char * website;
};

static const char *google_fromlangs[] = {N_("Afrikaans"), N_("Albanian"), N_("Arabic"), N_("Belarusian"), N_("Bulgarian"), N_("Catalan"), N_("Chinese"), N_("Croatian"), N_("Czech"), N_("Danish"), N_("Dutch"), N_("English"), N_("Estonian"), N_("Filipino"), N_("Finnish"), N_("French"), N_("Galician"), N_("German"), N_("Greek"), N_("Haitian Creole ALPHA"), N_("Hebrew"), N_("Hindi"), N_("Hungarian"), N_("Icelandic"), N_("Indonesian"), N_("Irish"), N_("Italian"), N_("Japanese"), N_("Korean"), N_("Latvian"), N_("Lithuanian"), N_("Macedonian"), N_("Malay"), N_("Maltese"), N_("Norwegian"), N_("Persian"), N_("Polish"), N_("Portuguese"), N_("Romanian"), N_("Russian"), N_("Serbian"), N_("Slovak"), N_("Slovenian"), N_("Spanish"), N_("Swahili"), N_("Swedish"), N_("Thai"), N_("Turkish"), N_("Ukrainian"), N_("Vietnamese"), N_("Welsh"), N_("Yiddish"), NULL};
static const char *google_intolangs[] = {N_("Afrikaans"), N_("Albanian"), N_("Arabic"), N_("Belarusian"), N_("Bulgarian"), N_("Catalan"), N_("Chinese (Simplified)"), N_("Chinese (Traditional)"), N_("Croatian"), N_("Czech"), N_("Danish"), N_("Dutch"), N_("English"), N_("Estonian"), N_("Filipino"), N_("Finnish"), N_("French"), N_("Galician"), N_("German"), N_("Greek"), N_("Haitian Creole ALPHA"), N_("Hebrew"), N_("Hindi"), N_("Hungarian"), N_("Icelandic"), N_("Indonesian"), N_("Irish"), N_("Italian"), N_("Japanese"), N_("Korean"), N_("Latvian"), N_("Lithuanian"), N_("Macedonian"), N_("Malay"), N_("Maltese"), N_("Norwegian"), N_("Persian"), N_("Polish"), N_("Portuguese"), N_("Romanian"), N_("Russian"), N_("Serbian"), N_("Slovak"), N_("Slovenian"), N_("Spanish"), N_("Swahili"), N_("Swedish"), N_("Thai"), N_("Turkish"), N_("Ukrainian"), N_("Vietnamese"), N_("Welsh"), N_("Yiddish"), NULL};
static const char *google_fromlangs_code[] = {"af", "sq", "ar", "be", "bg", "ca", "zh-CN", "hr", "cs", "da", "nl", "en", "et", "tl", "fi", "fr", "gl", "de", "el", "ht", "iw", "hi", "hu", "is", "id", "ga", "it", "ja", "ko", "lv", "lt", "mk", "ms", "mt", "no", "fa", "pl", "pt", "ro", "ru", "sr", "sk", "sl", "es", "sw", "sv", "th", "tr", "uk", "vi", "cy", "yi", NULL};
static const char *google_intolangs_code[] = {"af", "sq", "ar", "be", "bg", "ca", "zh-CN", "zh-TW", "hr", "cs", "da", "nl", "en", "et", "tl", "fi", "fr", "gl", "de", "el", "ht", "iw", "hi", "hu", "is", "id", "ga", "it", "ja", "ko", "lv", "lt", "mk", "ms", "mt", "no", "fa", "pl", "pt", "ro", "ru", "sr", "sk", "sl", "es", "sw", "sv", "th", "tr", "uk", "vi", "cy", "yi", NULL};

static const char *yahoo_fromlangs[] = {N_("Chinese (Simplified)"), N_("Chinese (Traditional)"), N_("English"), N_("Dutch"), N_("French"), N_("German"), N_("Greek"), N_("Italian"), N_("Japanese"), N_("Korean"), N_("Portuguese"), N_("Russian"), N_("Spanish"), NULL};
static const char *yahoo_chinese_simplified_tolangs[] = {N_("English"), N_("Chinese (Traditional)"), NULL};
static const char *yahoo_chinese_simplified_code[] = { "zh_en", "zh_zt" };
static const char *yahoo_chinese_traditional_tolangs[] = {N_("English"), N_("Chinese (Simplified)"), NULL};
static const char *yahoo_chinese_traditional_code[] = { "zt_en", "zt_zh" };
static const char *yahoo_english_tolangs[] = {N_("Chinese (Simplified)"), N_("Chinese (Traditional)"), N_("Dutch"), N_("French"), N_("German"), N_("Greek"), N_("Italian"), N_("Japanese"), N_("Korean"), N_("Portuguese"), N_("Russian"), N_("Spanish"), NULL};
static const char *yahoo_english_code[] = { "en_zh", "en_zt", "en_nl", "en_fr", "en_de", "en_el", "en_it", "en_ja", "en_ko", "en_pt", "en_ru", "en_es" };
static const char *yahoo_dutch_tolangs[] = {N_("English"), N_("French"), NULL};
static const char *yahoo_dutch_code[] = { "nl_en" , "nl_fr"};
static const char *yahoo_french_tolangs[] = {N_("Dutch"), N_("English"), N_("German"), N_("Greek"), N_("Italian"), N_("Portuguese"), N_("Spanish"), NULL };
static const char *yahoo_french_code[] = { "fr_nl", "fr_en", "fr_de", "fr_el", "fr_it", "fr_pt", "fr_es"};
static const char *yahoo_german_tolangs[] = {N_("English"), N_("French"), NULL};
static const char *yahoo_german_code[] = { "de_en", "de_fr" };
static const char *yahoo_greek_tolangs[] = {N_("English"), N_("French"), NULL};
static const char *yahoo_greek_code[] = { "el_en", "el_fr" };
static const char *yahoo_italian_tolangs[] = {N_("English"), N_("French"), NULL};
static const char *yahoo_italian_code[] = { "it_en", "it_fr" };
static const char *yahoo_japanese_tolangs[] = {N_("English"), NULL};
static const char *yahoo_japanese_code[] = { "ja_en" };
static const char *yahoo_korean_tolangs[] = {N_("English"), NULL};
static const char *yahoo_korean_code[] = { "ko_en" };
static const char *yahoo_portuguese_tolangs[] = {N_("English"), N_("French"), NULL};
static const char *yahoo_portuguese_code[] = { "pt_en", "pt_fr" };
static const char *yahoo_russian_tolangs[] = {N_("English"), NULL};
static const char *yahoo_russian_code[] = { "ru_en" };
static const char *yahoo_spanish_tolangs[] = {N_("English"), N_("French"), NULL};
static const char *yahoo_spanish_code[] = { "es_en", "es_fr" };
static const char **yahoo_tolangs[] = {yahoo_chinese_simplified_tolangs, yahoo_chinese_traditional_tolangs, yahoo_english_tolangs, yahoo_dutch_tolangs, yahoo_french_tolangs, yahoo_german_tolangs, yahoo_greek_tolangs, yahoo_italian_tolangs, yahoo_japanese_tolangs, yahoo_korean_tolangs, yahoo_portuguese_tolangs, yahoo_russian_tolangs, yahoo_spanish_tolangs};
static const char **yahoo_code[] = {yahoo_chinese_simplified_code, yahoo_chinese_traditional_code, yahoo_english_code, yahoo_dutch_code,yahoo_french_code, yahoo_german_code, yahoo_greek_code, yahoo_italian_code, yahoo_japanese_code, yahoo_korean_code, yahoo_portuguese_code, yahoo_russian_code, yahoo_spanish_code};

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

static const char *excite_fromlangs[] = {N_("Chinese (Simplified)"), N_("Chinese (Traditional)"), N_("English"), N_("Japanese"), N_("Korean"), NULL};
static const char *excite_chinese_simplified_tolangs[] = {N_("Japanese"), NULL};
static const char *excite_chinese_simplified_code[] = { "CHJA" };
static const char *excite_chinese_traditional_tolangs[] = {N_("Japanese"), NULL};
static const char *excite_chinese_traditional_code[] = { "CHJA" };
static const char *excite_english_tolangs[] = { N_("Japanese"), NULL};
static const char *excite_english_code[] = { "ENJA" };
static const char *excite_japanese_tolangs[] = {N_("English"), N_("Korean"), N_("Chinese (Simplified)"), N_("Chinese (Traditional)"), NULL};
static const char *excite_japanese_code[] = { "JAEN", "JAKO", "JACH", "JACH" };
static const char *excite_korean_tolangs[] = {N_("Japanese"), NULL};
static const char *excite_korean_code[] = { "KOJA" };
static const char **excite_tolangs[] = {excite_chinese_simplified_tolangs, excite_chinese_traditional_tolangs, excite_english_tolangs,  excite_japanese_tolangs, excite_korean_tolangs};
static const char **excite_code[] = {excite_chinese_simplified_code,excite_chinese_traditional_code,excite_english_code, excite_japanese_code, excite_korean_code};

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
	// name,                        fromlangs,            tolangs,            tolangs2,         code,            fromcode,              tocode,                website_name,   website
	{N_("Google Translate"),        google_fromlangs,     NULL,               google_intolangs, NULL,            google_fromlangs_code, google_intolangs_code, "Google",       "http://translate.google.com"},
	{ N_("Yahoo Translate"),        yahoo_fromlangs,      yahoo_tolangs,      NULL,             yahoo_code,      NULL,                  NULL,                  "Yahoo",        "http://babelfish.yahoo.com"},
	//{ N_("Altavista Translate"),    altavista_fromlangs,  altavista_tolangs,  NULL,             altavista_code,  NULL,                  NULL,                  "Altavista",    "http://babelfish.altavista.com"},
	//{ N_("SystranBox Translate"),   systranbox_fromlangs, systranbox_tolangs, NULL,             systranbox_code, NULL,                  NULL,                  "SystranBox",   "http://www.systranbox.com"},
	{ N_("Excite Japan Translate"), excite_fromlangs,     excite_tolangs,     NULL,             excite_code,     NULL,                  NULL,                  "Excite Japan", "http://www.excite.co.jp"},
	//{ N_("KingSoft Translate"), kingsoft_fromlangs, kingsoft_tolangs, NULL}
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
		engines[engine].website_url = trans_engines[engine].website;
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
	engine.srclangs.resize(calculate_cnt(engine_src.fromlangs));
	for(size_t i = 0; engine_src.fromlangs[i]; ++i) {
		engine.srclangs[i].name = gettext(engine_src.fromlangs[i]);
		if(engine_src.fromcode)
			engine.srclangs[i].code = engine_src.fromcode[i];
		if(engine_src.tolangs2) {
			if(engine.tgtlangs.empty()) {
				engine.tgtlangs.resize(1);
				std::vector<TransLanguage>& langs = engine.tgtlangs[0];
				langs.resize(calculate_cnt(engine_src.tolangs2));
				for(size_t j = 0; engine_src.tolangs2[j]; ++j) {
					langs[j].name = gettext(engine_src.tolangs2[j]);
					langs[j].code = engine_src.tocode[j];
				}
			}
			engine.srclangs[i].tolangind = 0;
		} else if(engine_src.tolangs) {
			if(engine.tgtlangs.empty())
				engine.tgtlangs.resize(engine.srclangs.size());
			std::vector<TransLanguage>& langs = engine.tgtlangs[i];
			langs.resize(calculate_cnt(engine_src.tolangs[i]));
			for(size_t j=0; engine_src.tolangs[i][j]; ++j) {
				langs[j].name = gettext(engine_src.tolangs[i][j]);
				langs[j].code = engine_src.code[i][j];
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
