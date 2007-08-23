#include "stardict_dictdotcn.h"
#include <glib/gi18n.h>
#include <string>

#ifdef _WIN32
#include <windows.h>
#endif

const StarDictPluginSystemService *plugin_service;

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

static void on_get_http_response(char *buffer, size_t buffer_len, int userdata)
{
	if (!buffer)
		return;
	const char *p = g_strstr_len(buffer, buffer_len, "\r\n\r\n");
	if (!p) {
		return;
	}
	p += 4;
	//g_print("%s\n", p);
}

static void lookup(const char *word)
{
	std::string file = "/ws.php?utf8=true&q=";
	file += word;
	plugin_service->send_http_request("dict.cn", file.c_str(), on_get_http_response, 0);
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
}

DLLIMPORT bool stardict_netdict_plugin_init(StarDictNetDictPlugInObject *obj)
{
	obj->lookup_func = lookup;
	obj->dict_name = _("Dict.cn");
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
