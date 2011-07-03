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

#include <glib/gi18n.h>
#include <algorithm>

#include "collation.h"
#include "full_text_trans.h"
#include "utils.h"
#include "libcommon.h"

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
	{ N_("Haitian Creole"), "ht"},
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
	{ N_("Haitian Creole"), "ht"},
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
	{ N_("Chinese"), NULL},
	{ N_("English"), NULL},
	{ N_("Japanese"), NULL},
	{ N_("Korean"), NULL},
	{ N_("French"), NULL},
	{ N_("German"), NULL},
	{ N_("Italian"), NULL},
	{ N_("Spanish"), NULL},
	{ N_("Portuguese"), NULL},
	{ NULL}
};
static const TransLanguageInt excite_chinese_tgtlangs[] = {
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
	{ N_("Chinese"), "JACH"},
	{ N_("French"), "JAFR"},
	{ N_("German"), "JADE"},
	{ N_("Italian"), "JAIT"},
	{ N_("Spanish"), "JAES"},
	{ N_("Portuguese"), "JAPT"},
	{ NULL}
};
static const TransLanguageInt excite_korean_tgtlangs[] = {
	{ N_("Japanese"), "KOJA"},
	{ NULL}
};
static const TransLanguageInt excite_french_tgtlangs[] = {
	{ N_("Japanese"), "FRJA"},
	{ N_("English"), "FREN"},
	{ NULL}
};
static const TransLanguageInt excite_german_tgtlangs[] = {
	{ N_("Japanese"), "DEJA"},
	{ N_("English"), "DEEN"},
	{ NULL}
};
static const TransLanguageInt excite_intalian_tgtlangs[] = {
	{ N_("Japanese"), "ITJA"},
	{ N_("English"), "ITEN"},
	{ NULL}
};
static const TransLanguageInt excite_spanish_tgtlangs[] = {
	{ N_("Japanese"), "ESJA"},
	{ N_("English"), "ESEN"},
	{ NULL}
};
static const TransLanguageInt excite_portuguese_tgtlangs[] = {
	{ N_("Japanese"), "PTJA"},
	{ N_("English"), "PTEN"},
	{ NULL}
};
static const TransLanguageInt* excite_tgtlangs[] = {
	excite_chinese_tgtlangs,
	excite_english_tgtlangs,
	excite_japanese_tgtlangs,
	excite_korean_tgtlangs,
	excite_french_tgtlangs,
	excite_german_tgtlangs,
	excite_intalian_tgtlangs,
	excite_spanish_tgtlangs,
	excite_portuguese_tgtlangs,
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

const CollateFunctions FullTextTransCollation = UTF8_UNICODE_CI;

FullTextTrans::FullTextTrans()
{
	utf8_collate_init(FullTextTransCollation);
	for(size_t engine=0; engine<TranslateEngine_Size; ++engine) {
		engines[engine].name = gettext(trans_engines[engine].name);
		engines[engine].website_name = trans_engines[engine].website_name;
		engines[engine].website_url = trans_engines[engine].website_url;
		init_engine(engines[engine], trans_engines[engine]);
		sort_engine(engines[engine]);
	}
	utf8_collate_end(FullTextTransCollation);
}

const TransEngine& FullTextTrans::get_engine(size_t engine_ind) const
{
	return engines[engine_ind];
}

void FullTextTrans::Translate(size_t engine_index, size_t fromlang_index, size_t tolang_index,
	const char *text)
{
	HttpMethod httpMethod;
	std::string host, file, headers, body;
	bool allow_absolute_URI;
	gchar *etext = common_encode_uri_string(text);
	build_request(engine_index, fromlang_index, tolang_index, etext,
		httpMethod, host, file, headers, body, allow_absolute_URI);
	g_free(etext);
	HttpClient *client = new HttpClient();
	client->on_error_.connect(sigc::mem_fun(this, &FullTextTrans::on_http_client_error));
	client->on_response_.connect(sigc::mem_fun(this, &FullTextTrans::on_http_client_response));
	oHttpManager.Add(client);
	client->SetMethod(httpMethod);
	client->SetHeaders(headers.c_str());
	client->SetBody(body.c_str());
	client->SetAllowAbsoluteURI(allow_absolute_URI);
	client->SendHttpRequest(host.c_str(), file.c_str(), (gpointer)engine_index);
}

void FullTextTrans::build_request(
	size_t engine_index, size_t fromlang_index, size_t tolang_index,
	const char *text,
	HttpMethod& httpMethod, std::string &host, std::string &file,
	std::string& headers, std::string& body, bool& allow_absolute_URI) const
{
	headers.clear();
	body.clear();
	if(engine_index==TranslateEngine_Google){
		httpMethod = HTTP_METHOD_GET;
		host = "translate.google.com";
		file = "/translate_t?ie=UTF-8";
		allow_absolute_URI = true;
	} else if(engine_index==TranslateEngine_Yahoo){
		httpMethod = HTTP_METHOD_POST;
		host = "babelfish.yahoo.com";
		file = "/translate_txt";
		body = "ei=UTF-8";
		allow_absolute_URI = true;
		headers += "Content-Type: application/x-www-form-urlencoded\r\n";
	/*}else if(engine_index==TranslateEngine_SystranBox){
		host = "www.systranbox.com";
		file = "/systran/box?systran_id=SystranSoft-en&systran_charset=UTF-8&systran_lp="; */
	} else if(engine_index==TranslateEngine_ExciteJapan){
		httpMethod = HTTP_METHOD_POST;
		host = "www.excite.co.jp";
		file = "/world";
		allow_absolute_URI = false;
		headers += "Content-Type: application/x-www-form-urlencoded\r\n";
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
	} else if(engine_index==TranslateEngine_Yahoo || engine_index==TranslateEngine_ExciteJapan) {
		const size_t tolangind = engines[engine_index].srclangs[fromlang_index].tolangind;
		std::string lang_code = engines[engine_index].tgtlangs[tolangind][tolang_index].code;
		if(engine_index==TranslateEngine_ExciteJapan) {
			if(strcmp(lang_code.c_str(),"KOJA")==0 || strcmp(lang_code.c_str(),"JAKO")==0){
				file += "/korean/";
			}else if(strcmp(lang_code.c_str(),"ENJA")==0 || strcmp(lang_code.c_str(),"JAEN")==0){
				file += "/english/";
			}else if(strcmp(lang_code.c_str(),"CHJA")==0 || strcmp(lang_code.c_str(),"JACH")==0){
				file += "/chinese/";
			}else if(strcmp(lang_code.c_str(),"FRJA")==0 || strcmp(lang_code.c_str(),"JAFR")==0
				|| strcmp(lang_code.c_str(),"FREN")==0 || strcmp(lang_code.c_str(),"ENFR")==0){
				file += "/french/";
			}else if(strcmp(lang_code.c_str(),"DEJA")==0 || strcmp(lang_code.c_str(),"JADE")==0
				|| strcmp(lang_code.c_str(),"DEEN")==0 || strcmp(lang_code.c_str(),"ENDE")==0){
				file += "/german/";
			}else if(strcmp(lang_code.c_str(),"ITJA")==0 || strcmp(lang_code.c_str(),"JAIT")==0
				|| strcmp(lang_code.c_str(),"ITEN")==0 || strcmp(lang_code.c_str(),"ENIT")==0){
				file += "/italian/";
			}else if(strcmp(lang_code.c_str(),"ESJA")==0 || strcmp(lang_code.c_str(),"JAES")==0
				|| strcmp(lang_code.c_str(),"ESEN")==0 || strcmp(lang_code.c_str(),"ENES")==0){
				file += "/spanish/";
			}else if(strcmp(lang_code.c_str(),"PTJA")==0 || strcmp(lang_code.c_str(),"JAPT")==0
				|| strcmp(lang_code.c_str(),"PTEN")==0 || strcmp(lang_code.c_str(),"ENPT")==0){
				file += "/portuguese/";
			}
			body += "wb_lp=";
			body += lang_code;
		} else if(engine_index==TranslateEngine_Yahoo) {
			body += "&lp=";
			body += lang_code;
		}
	}
	if (engine_index == TranslateEngine_Google) {
		file += "&text=";
		file += text;
	} else if(engine_index == TranslateEngine_Yahoo) {
		body += "&trtext=";
		body += text;
	/*}else if(engine_index == TranslateEngine_SystranBox) {
		file += "&systran_text=";
		file += text; */
	} else if(engine_index == TranslateEngine_ExciteJapan) {
		body += "&before=";
		body += text;
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

void FullTextTrans::sort_engine(TransEngine& engine)
{
	std::sort(engine.srclangs.begin(), engine.srclangs.end(), trans_engine_comp);
	for(size_t i=0; i < engine.tgtlangs.size(); ++i)
		std::sort(engine.tgtlangs[i].begin(), engine.tgtlangs[i].end(), trans_engine_comp);
}

bool FullTextTrans::trans_engine_comp(const TransLanguage& left, const TransLanguage& right)
{
	return utf8_collate(left.name.c_str(), right.name.c_str(), FullTextTransCollation) < 0;
}

void FullTextTrans::on_http_client_error(HttpClient* http_client, const char *error_msg)
{
	on_error_.emit(error_msg);
	oHttpManager.Remove(http_client);
}

void FullTextTrans::on_http_client_response(HttpClient* http_client)
{
	if (http_client->buffer == NULL) {
		on_error_.emit(_("Unable to interpret translation engine response!"));
		oHttpManager.Remove(http_client);
		return;
	}
	const char *buffer = http_client->buffer;
	size_t buffer_len = http_client->buffer_len;
	glong engine_index = (glong)(http_client->userdata);
	parse_response(buffer, buffer_len, engine_index);
	oHttpManager.Remove(http_client);
}

void FullTextTrans::parse_response(const char* buffer, size_t buffer_len, glong engine_index)
{
	bool found = false;
	std::string result_text;
	if (engine_index == TranslateEngine_ExciteJapan) {
		#define ExicuteTranslateStartMark "name=\"after\""
		char *p_E = g_strstr_len(buffer, buffer_len, ExicuteTranslateStartMark);
		if(p_E){
			p_E = strchr(p_E, '>');
			if (p_E) {
				p_E++;
				char *p2_E = g_strstr_len(p_E, buffer_len - (p_E - buffer), "</textarea>");
				if(p2_E){
					result_text.assign(p_E, p2_E-p_E);
					found = true;
				}
			}
		}
	/*} else if (engine_index == TranslateEngine_SystranBox) {
		#define SystranBoxTranslateStartMark "<textarea name=\"translation\" rows=\"10\" cols=\"3\" style=\"float:left; clear:none; background-color:#FFFFFF; color:#000000; border-color:#FFFFFF;\">"
		char *p_S = g_strstr_len(buffer, buffer_len, SystranBoxTranslateStartMark);
		if(p_S){
			p_S += sizeof(SystranBoxTranslateStartMark) -1;
			char *p2_S = g_strstr_len(p_S, buffer_len - (p_S - buffer), "</textarea>");
			if(p2_S){
				result_text.assign(p_S, p2_S-p_S);
				found = true;
			}
		} */
	} else if (engine_index == TranslateEngine_Yahoo) {
		#define YahooTranslateStartMark "<div id=\"result\">"
		const char *p_y = g_strstr_len(buffer, buffer_len, YahooTranslateStartMark);
		if(p_y) {
			p_y += sizeof(YahooTranslateStartMark) -1;
			const char *p2_y = g_strstr_len(p_y, buffer_len - (p_y - buffer), "</div>");
			const char *p3_y = g_strstr_len(p_y, buffer_len - (p_y - buffer), ">");
			if(p2_y && p3_y) {
				p3_y += 1;
				result_text.assign(p3_y, p2_y-p3_y);
				found = true;
			}
		}
	} else if (engine_index == TranslateEngine_Google) {
		static const char * const GoogleTranslateStartMark = "<span id=result_box ";
		static const char * const GoogleTranslateEndMark = "</div>";

		do {
			char *p = g_strstr_len(buffer, buffer_len, GoogleTranslateStartMark);
			if(!p)
				break;
			char *p1 = g_strstr_len(p, buffer_len - (p - buffer) , ">");
			if(!p1)
				break;
			p = p1 + 1;
			p1 = g_strstr_len(p, buffer_len - (p - buffer) , GoogleTranslateEndMark);
			if(!p1)
				break;
			result_text.assign(p, p1-p);
			found = true;
		} while(false);
		// remove spans
		if(found) {
			std::string temp;
			temp.reserve(result_text.length());
			size_t pos1, pos2, pos3;
			pos1 = 0;
			while(true) {
				pos2 = result_text.find('<', pos1);
				if(pos2 == std::string::npos) {
					temp.append(result_text, pos1, std::string::npos);
					break;
				}
				if(0 == result_text.compare(pos2, sizeof("<span")-1, "<span")) {
					temp.append(result_text, pos1, pos2-pos1);
					pos3 = result_text.find('>', pos2);
					if(pos3 == std::string::npos) {
						break;
					} else {
						pos1 = pos3 + 1;
					}
				} else if(0 == result_text.compare(pos2, sizeof("</span>")-1, "</span>")) {
					temp.append(result_text, pos1, pos2-pos1);
					pos1 = pos2 + sizeof("</span>") - 1;
				} else {
					pos3 = result_text.find('>', pos2);
					if(pos3 == std::string::npos) {
						temp.append(result_text, pos1, std::string::npos);
						break;
					} else {
						pos3 += 1;
						temp.append(result_text, pos1, pos3-pos1);
						pos1 = pos3;
					}
				}
			}
			result_text.swap(temp);
		}
	}

	if (found) {
		std::string charset;
		char *p3 = g_strstr_len(buffer, buffer_len, "charset=");
		if (p3) {
			p3 += sizeof("charset=") -1;
			char *p4 = p3;
			int len = buffer_len - (p3 - buffer);
			while (true) {
				if (p4 - p3 > len) {
					p4 = NULL;
					break;
				}
				if (*p4 == '"' || *p4 == '\r')
					break;
				p4++;
			}
			if (p4) {
				charset.assign(p3, p4-p3);
			}
		}
		if (charset.empty()) {
			on_response_.emit(result_text.c_str());
		} else {
			glib::CharStr text(g_convert(result_text.c_str(), result_text.length(), "UTF-8", charset.c_str(), NULL, NULL, NULL));
			if (text) {
				html_decode(get_impl(text), result_text);
				on_response_.emit(result_text.c_str());
			} else {
				std::string msg(_("Unable to interpret translation engine response!"));
				msg += "\n";
				msg += _("Conversion error.");
				on_error_.emit(msg.c_str());
			}
		}
	} else {
		on_error_.emit(_("Unable to interpret translation engine response!"));
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
