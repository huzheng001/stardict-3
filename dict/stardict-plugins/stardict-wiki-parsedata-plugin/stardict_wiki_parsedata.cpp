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

DLLIMPORT bool stardict_plugin_init(StarDictPlugInObject *obj, IAppDirs* appDirs)
{
	g_debug(_("Loading Wiki data parsing plug-in..."));
	if (strcmp(obj->version_str, PLUGIN_SYSTEM_VERSION)!=0) {
		g_print(_("Error: Wiki data parsing plugin version doesn't match!\n"));
		return true;
	}
	obj->type = StarDictPlugInType_PARSEDATA;
	obj->info_xml = g_strdup_printf("<plugin_info><name>%s</name><version>1.0</version><short_desc>%s</short_desc><long_desc>%s</long_desc><author>Hu Zheng &lt;huzheng001@gmail.com&gt;</author><website>http://stardict-4.sourceforge.net</website></plugin_info>", _("Wiki data parsing"), _("Wiki data parsing engine."), _("Parse the wiki data."));
	obj->configure_func = NULL;
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
