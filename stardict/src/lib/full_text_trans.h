#ifndef _FULL_TEXT_TRANS_H_
#define _FULL_TEXT_TRANS_H_

#include <string>

enum TranslateEngineCode {
	TranslateEngine_Google,
	TranslateEngine_Yahoo,
	// TranslateEngine_Altavista,
	//TranslateEngine_SystranBox,
	TranslateEngine_ExciteJapan,
	TranslateEngine_Size // number of active engines. Must be the last item.
};

struct TranslateEngine {
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

extern TranslateEngine trans_engines[];

void GetHostFile(gint engine_index, gint fromlang_index, gint tolang_index,
	std::string &host, std::string &file, const char *text);

#endif  // _FULL_TEXT_TRANS_H_
