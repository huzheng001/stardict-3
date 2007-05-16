#ifndef _STARDICT_PLUGIN_H_
#define _STARDICT_PLUGIN_H_

#include <gtk/gtk.h>
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

struct StarDictTtsPlugInSlots {
};

struct StarDictPluginInfo {
	const char *datadir;
	GtkWidget *mainwin;
};

// Notice: You need to init these structs' members before creating a StarDictPlugins object.
extern StarDictPluginInfo oStarDictPluginInfo;
extern StarDictVirtualDictPlugInSlots oStarDictVirtualDictPlugInSlots;
extern StarDictTtsPlugInSlots oStarDictTtsPlugInSlots;

struct StarDictPlugInObject {
	StarDictPlugInObject();

	const char* version_str;
	StarDictPlugInType type;

	const StarDictPluginInfo *plugin_info;
	const StarDictVirtualDictPlugInSlots *vd_slots;
	const StarDictTtsPlugInSlots *tts_slots;
};

#endif
