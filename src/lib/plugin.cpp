#include "plugin.h"

StarDictPluginSystemInfo oStarDictPluginSystemInfo;
StarDictVirtualDictPlugInSlots oStarDictVirtualDictPlugInSlots;
StarDictTtsPlugInSlots oStarDictTtsPlugInSlots;


StarDictPlugInObject::StarDictPlugInObject()
{
	version_str = PLUGIN_SYSTEM_VERSION;
	type = StarDictPlugInType_UNKNOWN;
	info_xml = NULL;
	plugin_info = &oStarDictPluginSystemInfo;
	vd_slots = &oStarDictVirtualDictPlugInSlots;
	tts_slots = &oStarDictTtsPlugInSlots;
}

StarDictPlugInObject::~StarDictPlugInObject()
{
	g_free(info_xml);
}
