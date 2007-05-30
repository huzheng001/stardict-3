#ifndef _STARDICT_PLUGIN_H_
#define _STARDICT_PLUGIN_H_

#include <gtk/gtk.h>
#include <string>
#include <list>


#define PLUGIN_SYSTEM_VERSION "3.0.0"

enum StarDictPlugInType {
	StarDictPlugInType_UNKNOWN,
	StarDictPlugInType_VIRTUALDICT,
	StarDictPlugInType_TTS,
};

/*struct VirtualDictLookupResponse {
	const char *bookname;
	const char *word;
	std::list<char *> datalist;
};*/

struct StarDictVirtualDictPlugInSlots {
	//typedef void (*on_stardict_virtual_dict_plugin_lookup_end_func_t)(const struct VirtualDictLookupResponse *);
	//on_stardict_virtual_dict_plugin_lookup_end_func_t on_lookup_end;
};

struct StarDictPluginSystemInfo {
	const char *datadir;
	GtkWidget *mainwin;
};

// Notice: You need to init these structs' members before creating a StarDictPlugins object.
extern StarDictPluginSystemInfo oStarDictPluginSystemInfo;
extern StarDictVirtualDictPlugInSlots oStarDictVirtualDictPlugInSlots;

typedef void (*plugin_configure_func_t)();

struct StarDictPlugInObject {
	StarDictPlugInObject();
	~StarDictPlugInObject();

	const char* version_str;
	StarDictPlugInType type;
	char* info_xml;
	plugin_configure_func_t configure_func;

	const StarDictPluginSystemInfo *plugin_info;
	const StarDictVirtualDictPlugInSlots *vd_slots;
};

#endif
