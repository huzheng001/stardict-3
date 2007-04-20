#include "plugin.h"

StarDictVirtualDictPlugInSlots oStarDictVirtualDictPlugInSlots;
StarDictTtsPlugInSlots oStarDictTtsPlugInSlots;


StarDictPlugInObject::StarDictPlugInObject()
{
	version_str = PLUGIN_SYSTEM_VERSION;
	type = StarDictPlugInType_UNKNOWN;
	vd_slots = &oStarDictVirtualDictPlugInSlots;
	tts_slots = &oStarDictTtsPlugInSlots;
}
