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
	void lookup(const char *word, char ***pppWord, char ****ppppWordData);
	bool is_instant();
	const char *dict_name();
private:
	GModule *module;
	StarDictVirtualDictPlugInObject *obj;
};

class StarDictVirtualDictPlugins {
public:
	StarDictVirtualDictPlugins();
	~StarDictVirtualDictPlugins();
	void add(GModule *module, StarDictVirtualDictPlugInObject *virtualdict_plugin_obj);
	void lookup(size_t iPlugin, const gchar *word, char ***pppWord, char ****ppppWordData);
	size_t ndicts() { return oPlugins.size(); }
	const char *dict_name(size_t iPlugin);
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
