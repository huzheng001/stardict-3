#ifndef _STARDICT_NETDICT_PLUGIN_H_
#define _STARDICT_NETDICT_PLUGIN_H_

struct NetDictResponse {
	~NetDictResponse();
	const char *bookname;
	char *word;
	char *data;
};

struct StarDictNetDictPlugInObject{
	StarDictNetDictPlugInObject();

	typedef void (*lookup_func_t)(const char *word, bool ismainwin);
	lookup_func_t lookup_func;
	const char *dict_name;
	const char *dict_cacheid;
};

#endif
