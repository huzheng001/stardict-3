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

#include "stardict_wordnet_parsedata.h"
#include <cstring>
#include <glib/gi18n.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#endif


struct WnUserData {
	const gchar *oword;
	std::string type;
	std::list<std::string> wordlist;
	std::string gloss;
};

static void func_parse_text(GMarkupParseContext *context, const gchar *text, gsize text_len, gpointer user_data, GError **error)
{
	const gchar *element = g_markup_parse_context_get_element(context);
	if (!element)
		return;
	WnUserData *Data = (WnUserData *)user_data;
	if (strcmp(element, "type")==0) {
		Data->type.assign(text, text_len);
	} else if (strcmp(element, "word")==0) {
		std::string word(text, text_len);
		if (word != Data->oword) {
			Data->wordlist.push_back(word);
		}
	} else if (strcmp(element, "gloss")==0) {
		Data->gloss.assign(text, text_len);
	}
}

static void wordnet2result(const char *p, size_t sec_size, ParseResult &result, const char *oword)
{
	WnUserData Data;
	Data.oword = oword;
	GMarkupParser parser;
	parser.start_element = NULL;
	parser.end_element = NULL;
	parser.text = func_parse_text;
	parser.passthrough = NULL;
	parser.error = NULL;
	GMarkupParseContext* context = g_markup_parse_context_new(&parser, (GMarkupParseFlags)0, &Data, NULL);
	g_markup_parse_context_parse(context, p, sec_size, NULL);
	g_markup_parse_context_end_parse(context, NULL);
	g_markup_parse_context_free(context);

	LinksPosList links_list;
	std::string res;
	std::string::size_type cur_pos = 0;
	char *eword;
	if (Data.type == "n") {
		res += "Noun\n";
		cur_pos += sizeof("Noun\n")-1;
	} else if (Data.type == "v") {
		res += "Verb\n";
		cur_pos += sizeof("Verb\n")-1;
	} else if (Data.type == "a") {
		res += "Adjective\n";
		cur_pos += sizeof("Adjective\n")-1;
	} else if (Data.type == "s") {
		res += "Adjective satellite\n";
		cur_pos += sizeof("Adjective satellite\n")-1;
	} else if (Data.type == "r") {
		res += "Adverb\n";
		cur_pos += sizeof("Adverb\n")-1;
	} else {
		eword = g_markup_escape_text(Data.type.c_str(), Data.type.length());
		res += eword;
		g_free(eword);
		cur_pos += g_utf8_strlen(Data.type.c_str(), Data.type.length());
	}
	size_t utf8_len;
	for (std::list<std::string>::iterator i = Data.wordlist.begin(); i != Data.wordlist.end(); ++i) {
		if (i != Data.wordlist.begin()) {
			res += '\t';
			cur_pos++;
		}
		res += "<span foreground=\"blue\" underline=\"single\">";
		utf8_len = g_utf8_strlen(i->c_str(), i->length());
		std::string link;
		link = "query://";
		link += *i;
		links_list.push_back(LinkDesc(cur_pos, utf8_len, link));
		eword = g_markup_escape_text(i->c_str(), i->length());
		res += eword;
		g_free(eword);
		res += "</span>";
		cur_pos += utf8_len;
	}
	if (!Data.wordlist.empty()) {
		res += '\n';
		//cur_pos++;
	}
	eword = g_markup_escape_text(Data.gloss.c_str(), Data.gloss.length());
	res += eword;
	g_free(eword);
	//cur_pos += g_utf8_strlen(Data.gloss.c_str(), Data.gloss.length());
	ParseResultItem item;
	item.type = ParseResultItemType_link;
	item.link = new ParseResultLinkItem;
	item.link->pango = res;
	item.link->links_list = links_list;
	result.item_list.push_back(item);
}

static bool parse(const char *p, unsigned int *parsed_size, ParseResult &result, const char *oword)
{
	if (*p != 'n')
		return false;
	p++;
	size_t len = strlen(p);
	if (len) {
		wordnet2result(p, len, result, oword);
	}
	*parsed_size = 1 + len + 1;
	return true;
}

DLLIMPORT bool stardict_plugin_init(StarDictPlugInObject *obj, IAppDirs* appDirs)
{
	g_debug(_("Loading WordNet data parsing plug-in..."));
	if (strcmp(obj->version_str, PLUGIN_SYSTEM_VERSION)!=0) {
		g_print(_("Error: WordNet data parsing plugin version doesn't match!\n"));
		return true;
	}
	obj->type = StarDictPlugInType_PARSEDATA;
	obj->info_xml = g_strdup_printf("<plugin_info><name>%s</name><version>1.0</version><short_desc>%s</short_desc><long_desc>%s</long_desc><author>Hu Zheng &lt;huzheng_001@163.com&gt;</author><website>http://stardict-4.sourceforge.net</website></plugin_info>", _("WordNet data parsing"), _("WordNet data parsing engine."), _("Parse the WordNet data."));
	obj->configure_func = NULL;
	return false;
}

DLLIMPORT void stardict_plugin_exit(void)
{
}

DLLIMPORT bool stardict_parsedata_plugin_init(StarDictParseDataPlugInObject *obj)
{
	obj->parse_func = parse;
	g_print(_("WordNet data parsing plug-in loaded.\n"));
	return false;
}

#ifdef _WIN32
BOOL APIENTRY DllMain (HINSTANCE hInst     /* Library instance handle. */ ,
                       DWORD reason        /* Reason this function is being called. */ ,
                       LPVOID reserved     /* Not used. */ )
{
    switch (reason)
    {
      case DLL_PROCESS_ATTACH:
        break;

      case DLL_PROCESS_DETACH:
        break;

      case DLL_THREAD_ATTACH:
        break;

      case DLL_THREAD_DETACH:
        break;
    }

    /* Returns TRUE on success, FALSE on failure */
    return TRUE;
}
#endif
