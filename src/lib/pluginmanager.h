#ifndef _STARDICT_PLUG_MANAGER_H_
#define _STARDICT_PLUG_MANAGER_H_

#include "plugin.h"
#include "virtualdictplugin.h"
#include "ttsplugin.h"
#include <glib.h>
#include <gmodule.h>
#include <vector>
#include <list>
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

class StarDictTtsPlugin {
public:
	StarDictTtsPlugin(GModule *module, StarDictTtsPlugInObject *tts_plugin_obj);
	~StarDictTtsPlugin();
	void saytext(const gchar *text);
private:
	GModule *module;
	StarDictTtsPlugInObject *obj;
};

class StarDictTtsPlugins {
public:
	StarDictTtsPlugins();
	~StarDictTtsPlugins();
	void add(GModule *module, StarDictTtsPlugInObject *tts_plugin_obj);
	void saytext(size_t iPlugin, const gchar *text);
	size_t nplugins() { return oPlugins.size(); }
private:
	std::vector<StarDictTtsPlugin *> oPlugins;
};

struct StarDictPluginInfo {
	std::string filename;
	std::string info_xml;
};

class StarDictPlugins {
public:
	StarDictPlugins(const char *dirpath, const std::list<std::string>& disable_list);
	~StarDictPlugins();
	void get_plugin_list(std::list<std::pair<StarDictPlugInType, std::list<StarDictPluginInfo> > > &plugin_list);
	bool get_loaded(const char *filename);
	void load_plugin(const char *filename);
	void unload_plugin(const char *filename);
	StarDictVirtualDictPlugins VirtualDictPlugins;
	StarDictTtsPlugins TtsPlugins;
private:
	std::string plugindirpath;
	std::list<std::string> loaded_plugin_list;
	void load(const char *dirpath, const std::list<std::string>& disable_list);
	void get_plugin_info(const char *filename, StarDictPlugInType &plugin_type, std::string &info_xml);
};

#endif
