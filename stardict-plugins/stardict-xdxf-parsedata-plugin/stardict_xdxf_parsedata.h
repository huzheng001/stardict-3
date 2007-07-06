#ifndef _STARDICT_XDXF_PARSEDATA_PLUGIN_H_
#define _STARDICT_XDXF_PARSEDATA_PLUGIN_H_

#include "../../src/lib/plugin.h"
#include "../../src/lib/parsedata_plugin.h"

extern "C" {
	extern bool stardict_plugin_init(StarDictPlugInObject *obj);
	extern void stardict_plugin_exit(void);
	extern bool stardict_parsedata_plugin_init(StarDictParseDataPlugInObject *obj);
}

#endif
