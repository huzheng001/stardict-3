#ifndef _STARDICT_NETDICT_PLUGIN_H_
#define _STARDICT_NETDICT_PLUGIN_H_

struct StarDictNetDictPlugInObject{
	StarDictNetDictPlugInObject();

	typedef void (*lookup_func_t)(const char *word);
	lookup_func_t lookup_func;
	const char *dict_name;
};

#endif
