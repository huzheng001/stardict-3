#ifndef _STARDICT_UPDATE_INFO_PLUGIN_H_
#define _STARDICT_UPDATE_INFO_PLUGIN_H_

#include "../../src/lib/plugin.h"

extern "C" {
	extern bool stardict_plugin_init(StarDictPlugInObject *obj);
	extern void stardict_plugin_exit(void);
	extern bool stardict_misc_plugin_init(void);
}

#endif
