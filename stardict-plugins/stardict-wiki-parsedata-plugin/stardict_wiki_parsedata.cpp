#include "stardict_wiki_parsedata.h"
#include "stardict_wiki2xml.h"
#include <glib/gi18n.h>

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
		item.type = ParseResultItemType_unknown; // So item.mark is not deleted.
	}
	*parsed_size = 1 + len + 1;
	return true;
}

static void configure()
{
}

bool stardict_plugin_init(StarDictPlugInObject *obj)
{
	if (strcmp(obj->version_str, PLUGIN_SYSTEM_VERSION)!=0) {
		g_print("Error: Wiki data parse plugin version doesn't match!\n");
		return true;
	}
	obj->type = StarDictPlugInType_PARSEDATA;
	obj->info_xml = g_strdup_printf("<plugin_info><name>%s</name><version>1.0</version><short_desc>%s</short_desc><long_desc>%s</long_desc><author>Hu Zheng &lt;huzheng_001@163.com&gt;</author><website>http://stardict.sourceforge.net</website></plugin_info>", _("Wiki data parse"), _("Wiki data parse engine."), _("Parse the wiki data."));
	obj->configure_func = configure;
	return false;
}

void stardict_plugin_exit(void)
{
}

bool stardict_parsedata_plugin_init(StarDictParseDataPlugInObject *obj)
{
	obj->parse_func = parse;
	g_print(_("Wiki data parse plug-in loaded.\n"));
	return false;
}
