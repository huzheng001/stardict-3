#ifndef _STARDICT_MAN_PLUGIN_H_
#define _STARDICT_MAN_PLUGIN_H_

#include "../../src/lib/plugin.h"
#include "../../src/lib/virtualdictplugin.h"

extern "C" {
	extern bool stardict_plugin_init(StarDictPlugInObject *obj);
	extern void stardict_plugin_exit(void);
	extern bool stardict_virtualdict_plugin_init(StarDictVirtualDictPlugInObject *obj);
}

#endif
