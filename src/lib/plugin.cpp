#include "plugin.h"

StarDictPluginSystemInfo oStarDictPluginSystemInfo;
StarDictPluginSystemService oStarDictPluginSystemService;


StarDictPlugInObject::StarDictPlugInObject()
{
	version_str = PLUGIN_SYSTEM_VERSION;
	type = StarDictPlugInType_UNKNOWN;
	info_xml = NULL;
	configure_func = NULL;
	plugin_info = &oStarDictPluginSystemInfo;
	plugin_service = &oStarDictPluginSystemService;
}

StarDictPlugInObject::~StarDictPlugInObject()
{
	g_free(info_xml);
}
