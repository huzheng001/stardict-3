#include "plugin.h"

StarDictPluginInfo oStarDictPluginInfo;
StarDictVirtualDictPlugInSlots oStarDictVirtualDictPlugInSlots;
StarDictTtsPlugInSlots oStarDictTtsPlugInSlots;


StarDictPlugInObject::StarDictPlugInObject()
{
	version_str = PLUGIN_SYSTEM_VERSION;
	type = StarDictPlugInType_UNKNOWN;
	plugin_info = &oStarDictPluginInfo;
	vd_slots = &oStarDictVirtualDictPlugInSlots;
	tts_slots = &oStarDictTtsPlugInSlots;
}
