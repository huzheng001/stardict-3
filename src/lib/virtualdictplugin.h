#ifndef _STARDICT_VIRTUALDICT_PLUGIN_H_
#define _STARDICT_VIRTUALDICT_PLUGIN_H_

struct StarDictVirtualDictPlugInObject{
	StarDictVirtualDictPlugInObject();

	typedef void (*lookup_func_t)(char *word);
	lookup_func_t lookup_func;
};

#endif
