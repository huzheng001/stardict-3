#include "stardict_dictdotcn.h"
#include <glib/gi18n.h>
#include <string>
#include <list>

#ifdef _WIN32
#include <windows.h>
#endif

#define DICTDOTCN "dict.cn"

const StarDictPluginSystemService *plugin_service;

struct QueryInfo {
	bool ismainwin;
	char *word;
};

static std::list<QueryInfo *> keyword_list;

static char *build_dictdata(char type, const char *definition)
{
	size_t len = strlen(definition);
	guint32 size;
	size = sizeof(char) + len + 1;
	char *data = (char *)g_malloc(sizeof(guint32) + size);
	char *p = data;
	*((guint32 *)p)= size;
	p += sizeof(guint32);
	*p = type;
	p++;
	memcpy(p, definition, len+1);
	return data;
}

struct dict_ParseUserData {
	std::string pron;
	std::string def;
	std::string rel;
};

static void dict_parse_text(GMarkupParseContext *context, const gchar *text, gsize text_len, gpointer user_data, GError **error)
{
	const gchar *element = g_markup_parse_context_get_element(context);
	if (!element)
		return;
	dict_ParseUserData *Data = (dict_ParseUserData *)user_data;
	if (strcmp(element, "pron")==0) {
		Data->pron.assign(text, text_len);
	} else if (strcmp(element, "def")==0) {
		Data->def.assign(text, text_len);
	} else if (strcmp(element, "rel")==0) {
		Data->rel.assign(text, text_len);
	}
}

static void on_get_http_response(char *buffer, size_t buffer_len, gpointer userdata)
{
	if (!buffer) {
		return;
	}
	const char *p = g_strstr_len(buffer, buffer_len, "\r\n\r\n");
	if (!p) {
		return;
	}
	p += 4;
	const char *xml = g_strstr_len(p, buffer_len - (p - buffer), "<dict>");
	if (!xml) {
		return;
	}
	const char *xml_end = g_strstr_len(xml+6, buffer_len - (xml+6 - buffer), "</dict>");
	if (!xml_end) {
		return;
	}
	xml_end += 7;
	dict_ParseUserData Data;
	GMarkupParser parser;
	parser.start_element = NULL;
	parser.end_element = NULL;
	parser.text = dict_parse_text;
	parser.passthrough = NULL;
	parser.error = NULL;
	GMarkupParseContext* context = g_markup_parse_context_new(&parser, (GMarkupParseFlags)0, &Data, NULL);
	g_markup_parse_context_parse(context, xml, xml_end - xml, NULL);
	g_markup_parse_context_end_parse(context, NULL);
	g_markup_parse_context_free(context);
	QueryInfo *qi = (QueryInfo *)userdata;
	NetDictResponse *resp = new NetDictResponse;
	resp->bookname = _("Dict.cn");
	resp->word = qi->word; // So neen't free qi->word;
	if (Data.def == "Not Found") {
		resp->data = NULL;
	} else {
		std::string definition;
		if (!Data.pron.empty()) {
			definition += Data.pron;
			definition += "\n";
		}
		definition += Data.def;
		if (!Data.rel.empty()) {
			definition += "\n";
			definition += Data.rel;
		}
		resp->data = build_dictdata('m', definition.c_str());
	}
	plugin_service->netdict_save_cache_resp(DICTDOTCN, qi->word, resp);
	plugin_service->show_netdict_resp(resp, qi->ismainwin);
	delete qi;
	keyword_list.remove(qi);
}

static void lookup(const char *word, bool ismainwin)
{
	std::string file = "/ws.php?utf8=true&q=";
	file += word;
	gchar *keyword = g_strdup(word);
	QueryInfo *qi = new QueryInfo;
	qi->ismainwin = ismainwin;
	qi->word = keyword;
	keyword_list.push_back(qi);
	plugin_service->send_http_request("dict.cn", file.c_str(), on_get_http_response, qi);
}

static void configure()
{
}

DLLIMPORT bool stardict_plugin_init(StarDictPlugInObject *obj)
{
	if (strcmp(obj->version_str, PLUGIN_SYSTEM_VERSION)!=0) {
		g_print("Error: Dict.cn plugin version doesn't match!\n");
		return true;
	}
	obj->type = StarDictPlugInType_NETDICT;
	obj->info_xml = g_strdup_printf("<plugin_info><name>%s</name><version>1.0</version><short_desc>%s</short_desc><long_desc>%s</long_desc><author>Hu Zheng &lt;huzheng_001@163.com&gt;</author><website>http://stardict.sourceforge.net</website></plugin_info>", _("Dict.cn"), _("Dict.cn network dictionary."), _("Query result from Dict.cn website."));
	obj->configure_func = configure;
	plugin_service = obj->plugin_service;
	return false;
}

DLLIMPORT void stardict_plugin_exit(void)
{
	for (std::list<QueryInfo *>::iterator i = keyword_list.begin(); i != keyword_list.end(); ++i) {
		g_free((*i)->word);
		delete *i;
	}
}

DLLIMPORT bool stardict_netdict_plugin_init(StarDictNetDictPlugInObject *obj)
{
	obj->lookup_func = lookup;
	obj->dict_name = _("Dict.cn");
	obj->dict_cacheid = DICTDOTCN;
	g_print(_("Dict.cn plug-in loaded.\n"));
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
