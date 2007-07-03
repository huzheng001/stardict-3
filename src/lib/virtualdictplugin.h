#ifndef _STARDICT_VIRTUALDICT_PLUGIN_H_
#define _STARDICT_VIRTUALDICT_PLUGIN_H_

struct StarDictVirtualDictPlugInObject{
	StarDictVirtualDictPlugInObject();

	typedef void (*lookup_func_t)(const char *text, char ***pppWord, char ****ppppWordData);
	lookup_func_t lookup_func;
	const char *dict_name;
};

#endif
