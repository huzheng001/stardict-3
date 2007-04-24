#ifndef _STARDICT_PLUG_MANAGER_H_
#define _STARDICT_PLUG_MANAGER_H_

#include "plugin.h"
#include "virtualdictplugin.h"
#include <glib.h>
#include <gmodule.h>
#include <vector>
#include "dictmask.h"

class StarDictVirtualDictPlugin {
public:
	StarDictVirtualDictPlugin(GModule *module, StarDictVirtualDictPlugInObject *virtualdict_plugin_obj);
	~StarDictVirtualDictPlugin();
	void lookup(const char *word);
	bool is_instant();
private:
	GModule *module;
	StarDictVirtualDictPlugInObject *obj;
};

class StarDictVirtualDictPlugins {
public:
	StarDictVirtualDictPlugins();
	~StarDictVirtualDictPlugins();
	void add(GModule *module, StarDictVirtualDictPlugInObject *virtualdict_plugin_obj);
	void lookup(const gchar *word, size_t iPlugin);
	size_t ndicts() { return oPlugins.size(); }
	void SetDictMask(std::vector<InstantDictIndex> &dictmask);
private:
	std::vector<StarDictVirtualDictPlugin *> oPlugins;
};

class StarDictPlugins {
public:
	StarDictPlugins(const char *dirpath);
	~StarDictPlugins();
	StarDictVirtualDictPlugins VirtualDictPlugins;
private:
	void load(const char *dirpath);
	void load_plugin(const char *filename);
};

#endif
