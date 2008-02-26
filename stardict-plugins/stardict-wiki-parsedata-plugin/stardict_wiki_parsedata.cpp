#include "stardict_wiki_parsedata.h"
#include "stardict_wiki2xml.h"
#include <cstring>
#include <glib/gi18n.h>

#ifdef _WIN32
#include <windows.h>
#endif

static bool parse(const char *p, unsigned int *parsed_size, ParseResult &result, const char *oword)
{
	if (*p != 'w')
		return false;
	p++;
	size_t len = strlen(p);
	if (len) {
		ParseResultItem item;
		item.type = ParseResultItemType_mark;
		item.mark = new ParseResultMarkItem;
		std::string res(p, len);
		std::string xml = wiki2xml(res);
		item.mark->pango = wikixml2pango(xml);
		result.item_list.push_back(item);
	}
	*parsed_size = 1 + len + 1;
	return true;
}

static void configure()
{
}

DLLIMPORT bool stardict_plugin_init(StarDictPlugInObject *obj)
{
	if (strcmp(obj->version_str, PLUGIN_SYSTEM_VERSION)!=0) {
		g_print("Error: Wiki data parsing plugin version doesn't match!\n");
		return true;
	}
	obj->type = StarDictPlugInType_PARSEDATA;
	obj->info_xml = g_strdup_printf("<plugin_info><name>%s</name><version>1.0</version><short_desc>%s</short_desc><long_desc>%s</long_desc><author>Hu Zheng &lt;huzheng_001@163.com&gt;</author><website>http://stardict.sourceforge.net</website></plugin_info>", _("Wiki data parsing"), _("Wiki data parsing engine."), _("Parse the wiki data."));
	obj->configure_func = configure;
	return false;
}

DLLIMPORT void stardict_plugin_exit(void)
{
}

DLLIMPORT bool stardict_parsedata_plugin_init(StarDictParseDataPlugInObject *obj)
{
	obj->parse_func = parse;
	g_print(_("Wiki data parsing plug-in loaded.\n"));
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
