#include "plugin.h"

StarDictPluginSystemInfo oStarDictPluginSystemInfo;
StarDictVirtualDictPlugInSlots oStarDictVirtualDictPlugInSlots;


StarDictPlugInObject::StarDictPlugInObject()
{
	version_str = PLUGIN_SYSTEM_VERSION;
	type = StarDictPlugInType_UNKNOWN;
	info_xml = NULL;
	configure_func = NULL;
	plugin_info = &oStarDictPluginSystemInfo;
	vd_slots = &oStarDictVirtualDictPlugInSlots;
}

StarDictPlugInObject::~StarDictPlugInObject()
{
	g_free(info_xml);
}
