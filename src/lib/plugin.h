#ifndef _STARDICT_PLUGIN_H_
#define _STARDICT_PLUGIN_H_

#include "sigc++/sigc++.h"

#define PLUGIN_SYSTEM_VERSION "3.0.0"

enum StarDictPlugInType {
	StarDictPlugInType_UNKNOWN,
	StarDictPlugInType_VIRTUALDICT,
	StarDictPlugInType_TTS,
};


struct VirtualDictLookupResponse {
	char *bookname;
	char *word;
	std::list<char *> datalist;
};

struct StarDictVirtualDictPlugInSlots {
	sigc::signal<void, const struct VirtualDictLookupResponse *> on_lookup_end;
};

struct StarDictTtsPlugInSlots {
};

// Notice: You need to init these structs' members before creating a StarDictPlugins object.
extern StarDictVirtualDictPlugInSlots oStarDictVirtualDictPlugInSlots;
extern StarDictTtsPlugInSlots oStarDictTtsPlugInSlots;

struct StarDictPlugInObject {
	StarDictPlugInObject();

	const char* version_str;
	StarDictPlugInType type;

	const StarDictVirtualDictPlugInSlots *vd_slots;
	const StarDictTtsPlugInSlots *tts_slots;
};

#endif
