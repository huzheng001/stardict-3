/*
 * Copyright 2016 Hu Zheng <huzheng001@gmail.com>
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


#include "stardict_youdaodict.h"
#include <glib/gi18n.h>
#include <cstring>
#include <string>
#include <list>
#include <string.h>


#define YOUDAODICT "youdao.com"

static const StarDictPluginSystemInfo *plugin_info = NULL;
static const StarDictPluginSystemService *plugin_service;
static IAppDirs* gpAppDirs = NULL;

struct QueryInfo {
	bool ismainwin;
	char *word;
};

static std::list<QueryInfo *> keyword_list;


struct dict_ParseUserData {
	std::string phonetic_symbol;
	std::list<std::string> custom_translation;
	std::list< std::pair <std::string, std::list <std::string> > > web_translation;

	std::string tmp_key;
	std::list <std::string> tmp_value;
};

static void dict_parse_text(GMarkupParseContext *context, const gchar *text, gsize text_len, gpointer user_data, GError **error)
{
	const gchar *element = g_markup_parse_context_get_element(context);
        if (!element) {
                return;
	}

	dict_ParseUserData *Data = (dict_ParseUserData *)user_data;
	if (strcmp(element, "phonetic-symbol")==0) {
		Data->phonetic_symbol.assign(text, text_len);
	}
}

static void dict_parse_passthrough(GMarkupParseContext *context, const gchar *passthrough_text, gsize text_len, gpointer user_data, GError **error)
{
	const gchar *element = g_markup_parse_context_get_element(context);
        if (!element) {
                return;
	}
	gchar *text = g_strndup(passthrough_text, text_len);
	if (!(g_str_has_prefix(text, "<![CDATA[") && (g_str_has_suffix(text, "]]>")))) {
		g_free(text);
                return;
	}

	gchar *p = text + 9;
	gchar *p2 = strstr(p, "]]>");
	if (p2) {
		*p2 = '\0';
	}

	dict_ParseUserData *Data = (dict_ParseUserData *)user_data;
	if (strcmp(element, "content")==0) {
		Data->custom_translation.push_back(std::string(p));
	} else if (strcmp(element, "key")==0) {
		Data->tmp_key.assign(p);
	} else if (strcmp(element, "value")==0) {
		Data->tmp_value.push_back(std::string(p));
	}
	g_free(text);
}

static void dict_parse_start_element(GMarkupParseContext *context, const gchar *element_name, const gchar **attribute_names, const gchar **attribute_values, gpointer user_data, GError **error)
{
	if (strcmp(element_name, "web-translation")==0) {
		dict_ParseUserData *Data = (dict_ParseUserData *)user_data;
		Data->tmp_value.clear();
	}
}

static void dict_parse_end_element(GMarkupParseContext *context, const gchar *element_name, gpointer user_data, GError **error)
{
	if (strcmp(element_name, "web-translation")==0) {
		dict_ParseUserData *Data = (dict_ParseUserData *)user_data;
		Data->web_translation.push_back(std::pair<std::string, std::list < std::string> >(Data->tmp_key, Data->tmp_value));
	}
}

static void process_xml_response(const char *data, size_t data_len, NetDictResponse *resp)
{
	const gchar *xml = g_strstr_len(data, data_len, "<yodaodict>");
	if (!xml) {
		return;
	}

	const gchar *xml_end = g_strstr_len(xml+11, data_len - (xml+11 - data), "</yodaodict>");
	if (!xml_end) {
		return;
	}
	xml_end += 12;

	data = xml;
	data_len = xml_end - xml;

	dict_ParseUserData Data;
	GMarkupParser parser;
	parser.start_element = dict_parse_start_element;
	parser.end_element = dict_parse_end_element;
	parser.text = dict_parse_text;
	parser.passthrough = dict_parse_passthrough;
	parser.error = NULL;
	GError *err = NULL;
	GMarkupParseContext* context = g_markup_parse_context_new(&parser, (GMarkupParseFlags)0, &Data, NULL);
	if(!g_markup_parse_context_parse(context, data, data_len, &err)) {
		g_warning(_("YouDao.com plugin: context parse failed: %s"), err ? err->message : "");
		g_error_free(err);
		g_markup_parse_context_free(context);
		return;
	}
	if(!g_markup_parse_context_end_parse(context, &err)) {
		g_warning(_("YouDao.com plugin: context parse failed: %s"), err ? err->message : "");
		g_error_free(err);
		g_markup_parse_context_free(context);
		return;
	}
	g_markup_parse_context_free(context);
	if (Data.phonetic_symbol.empty() && Data.custom_translation.empty() && Data.web_translation.empty()) {
		return;
	}

	std::string definition;
	if(!Data.phonetic_symbol.empty()) {
		definition += "&lt;";
		definition += Data.phonetic_symbol;
		definition += "&gt;";
	}
	if (!Data.custom_translation.empty()) {
		if (!definition.empty()) {
			definition += "\n\n";
		}
		definition += "<b>基本翻译：</b>\n";
		for (std::list< std::string >::const_iterator it=Data.custom_translation.begin(); it != Data.custom_translation.end(); ++it) {
			definition += "\n";
			definition += *it;
		}
	}
	if (!Data.web_translation.empty()) {
		if (!definition.empty()) {
			definition += "\n\n";
		}
		definition += "<b>网络释义：</b>\n";
		for (std::list<std::pair<std::string, std::list <std::string> > >::const_iterator i = Data.web_translation.begin(); i != Data.web_translation.end(); ++i) {
			definition += "\n<kref>";
			definition += i->first;
			definition += "</kref>：";
			for (std::list<std::string>::const_iterator it=i->second.begin(); it != i->second.end(); ++it) {
				definition += " ";
				definition += *it;
			}
		}
	}
	resp->data = plugin_service->build_dictdata('x', definition.c_str());
}

static void on_get_http_response(const char *buffer, size_t buffer_len, gpointer userdata)
{
	if (!buffer) {
		return;
	}
	const char *p = g_strstr_len(buffer, buffer_len, "\r\n\r\n");
	if (!p) {
		return;
	}
	p += 4;

	QueryInfo *qi = (QueryInfo *)userdata;
	NetDictResponse *resp = new NetDictResponse;
	resp->bookname = _("www.YouDao.com");
	resp->booklink = "http://www.youdao.com";
	resp->word = qi->word; // So neen't free qi->word;

	process_xml_response(p, buffer_len - (p - buffer), resp);

	plugin_service->netdict_save_cache_resp(YOUDAODICT, qi->word, resp);
	plugin_service->show_netdict_resp(YOUDAODICT, resp, qi->ismainwin);
	delete qi;
	keyword_list.remove(qi);
}

static void lookup(const char *word, bool ismainwin)
{
	std::string file = "/fsearch?q=";

	char *eword = plugin_service->encode_uri_string(word);
	file += eword;
	g_free(eword);
	gchar *keyword = g_strdup(word);
	QueryInfo *qi = new QueryInfo;
	qi->ismainwin = ismainwin;
	qi->word = keyword;
	keyword_list.push_back(qi);
	plugin_service->send_http_request("dict.youdao.com", file.c_str(), on_get_http_response, qi);
}

DLLIMPORT bool stardict_plugin_init(StarDictPlugInObject *obj, IAppDirs* appDirs)
{
	g_debug(_("Loading YouDao.com plug-in..."));
	if (strcmp(obj->version_str, PLUGIN_SYSTEM_VERSION)!=0) {
		g_print(_("Error: YouDao.com plugin version doesn't match!\n"));
		return true;
	}
	obj->type = StarDictPlugInType_NETDICT;
	obj->info_xml = g_strdup_printf("<plugin_info><name>%s</name><version>1.0</version><short_desc>%s</short_desc><long_desc>%s</long_desc><author>Hu Zheng &lt;huzheng001@gmail.com&gt;</author><website>http://stardict-4.sourceforge.net</website></plugin_info>", _("YouDao.com"), _("YouDao.com network dictionary."), _("Query result from YouDao.com website."));
	obj->configure_func = NULL;
	plugin_info = obj->plugin_info;
	plugin_service = obj->plugin_service;
	gpAppDirs = appDirs;
	return false;
}

DLLIMPORT void stardict_plugin_exit(void)
{
	for (std::list<QueryInfo *>::iterator i = keyword_list.begin(); i != keyword_list.end(); ++i) {
		g_free((*i)->word);
		delete *i;
	}
	gpAppDirs = NULL;
}

DLLIMPORT bool stardict_netdict_plugin_init(StarDictNetDictPlugInObject *obj)
{
	obj->lookup_func = lookup;
	obj->dict_name = _("www.YouDao.com");
	obj->dict_link = "http://www.youdao.com";
	obj->dict_cacheid = YOUDAODICT;
	g_print(_("YouDao.com plug-in loaded.\n"));
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
