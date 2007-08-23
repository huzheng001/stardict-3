#ifndef _STARDICT_PLUG_MANAGER_H_
#define _STARDICT_PLUG_MANAGER_H_

#include "plugin.h"
#include "virtualdictplugin.h"
#include "netdictplugin.h"
#include "ttsplugin.h"
#include "parsedata_plugin.h"
#include <glib.h>
#include <gmodule.h>
#include <string>
#include <vector>
#include <list>
#include "dictmask.h"

struct StarDictPluginBaseObject {
	StarDictPluginBaseObject(const char *filename, GModule *module_, plugin_configure_func_t configure_func_);
	std::string plugin_filename;
	GModule *module;
	plugin_configure_func_t configure_func;
};

class StarDictPluginBase {
public:
	StarDictPluginBase(StarDictPluginBaseObject *baseobj_);
	~StarDictPluginBase();
	const char *get_filename();
	void configure();
protected:
	StarDictPluginBaseObject *baseobj;
};

class StarDictVirtualDictPlugin : public StarDictPluginBase {
public:
	StarDictVirtualDictPlugin(StarDictPluginBaseObject *baseobj, StarDictVirtualDictPlugInObject *virtualdict_plugin_obj);
	~StarDictVirtualDictPlugin();
	void lookup(const char *word, char ***pppWord, char ****ppppWordData);
	const char *dict_name();
	const char *dict_id();
private:
	StarDictVirtualDictPlugInObject *obj;
};

class StarDictVirtualDictPlugins {
public:
	StarDictVirtualDictPlugins();
	~StarDictVirtualDictPlugins();
	void add(StarDictPluginBaseObject *baseobj, StarDictVirtualDictPlugInObject *virtualdict_plugin_obj);
	void lookup(size_t iPlugin, const gchar *word, char ***pppWord, char ****ppppWordData);
	size_t ndicts() { return oPlugins.size(); }
	const char *dict_name(size_t iPlugin);
	const char *dict_id(size_t iPlugin);
	bool find_dict_by_id(const char *id, size_t &iPlugin);
	void unload_plugin(const char *filename);
	void configure_plugin(const char *filename);
	void reorder(const std::list<std::string>& order_list);
private:
	std::vector<StarDictVirtualDictPlugin *> oPlugins;
};

class StarDictNetDictPlugin : public StarDictPluginBase {
public:
	StarDictNetDictPlugin(StarDictPluginBaseObject *baseobj, StarDictNetDictPlugInObject *netdict_plugin_obj);
	~StarDictNetDictPlugin();
	void lookup(const char *word);
	const char *dict_name();
	const char *dict_id();
private:
	StarDictNetDictPlugInObject *obj;
};

class StarDictNetDictPlugins {
public:
	StarDictNetDictPlugins();
	~StarDictNetDictPlugins();
	void add(StarDictPluginBaseObject *baseobj, StarDictNetDictPlugInObject *netdict_plugin_obj);
	void lookup(size_t iPlugin, const gchar *word);
	size_t ndicts() { return oPlugins.size(); }
	const char *dict_name(size_t iPlugin);
	const char *dict_id(size_t iPlugin);
	bool find_dict_by_id(const char *id, size_t &iPlugin);
	void unload_plugin(const char *filename);
	void configure_plugin(const char *filename);
	void reorder(const std::list<std::string>& order_list);
private:
	std::vector<StarDictNetDictPlugin *> oPlugins;
};

class StarDictTtsPlugin : public StarDictPluginBase {
public:
	StarDictTtsPlugin(StarDictPluginBaseObject *baseobj, StarDictTtsPlugInObject *tts_plugin_obj);
	~StarDictTtsPlugin();
	void saytext(const gchar *text);
	const char *tts_name();
private:
	StarDictTtsPlugInObject *obj;
};

class StarDictTtsPlugins {
public:
	StarDictTtsPlugins();
	~StarDictTtsPlugins();
	void add(StarDictPluginBaseObject *baseobj, StarDictTtsPlugInObject *tts_plugin_obj);
	void saytext(size_t iPlugin, const gchar *text);
	const char* tts_name(size_t iPlugin);
	size_t nplugins() { return oPlugins.size(); }
	void unload_plugin(const char *filename);
	void configure_plugin(const char *filename);
	void reorder(const std::list<std::string>& order_list);
private:
	std::vector<StarDictTtsPlugin *> oPlugins;
};

class StarDictParseDataPlugin : public StarDictPluginBase {
public:
	StarDictParseDataPlugin(StarDictPluginBaseObject *baseobj, StarDictParseDataPlugInObject *parsedata_plugin_obj);
	~StarDictParseDataPlugin();
	bool parse(const char *p, unsigned int *parsed_size, ParseResult &result, const char *oword);
private:
	StarDictParseDataPlugInObject *obj;
};

class StarDictParseDataPlugins {
public:
	StarDictParseDataPlugins();
	~StarDictParseDataPlugins();
	void add(StarDictPluginBaseObject *baseobj, StarDictParseDataPlugInObject *parsedata_plugin_obj);
	bool parse(size_t iPlugin, const char *p, unsigned int *parsed_size, ParseResult &result, const char *oword);
	size_t nplugins() { return oPlugins.size(); }
	void unload_plugin(const char *filename);
	void configure_plugin(const char *filename);
	void reorder(const std::list<std::string>& order_list);
private:
	std::vector<StarDictParseDataPlugin *> oPlugins;
};

class StarDictMiscPlugin : public StarDictPluginBase {
public:
	StarDictMiscPlugin(StarDictPluginBaseObject *baseobj);
	~StarDictMiscPlugin();
};

class StarDictMiscPlugins {
public:
	StarDictMiscPlugins();
	~StarDictMiscPlugins();
	void add(StarDictPluginBaseObject *baseobj);
	void unload_plugin(const char *filename);
	void configure_plugin(const char *filename);
	void reorder(const std::list<std::string>& order_list);
private:
	std::vector<StarDictMiscPlugin *> oPlugins;
};

struct StarDictPluginInfo {
	std::string filename;
	StarDictPlugInType plugin_type;
	std::string info_xml;
	bool can_configure;
};

class StarDictPlugins {
public:
	StarDictPlugins(const char *dirpath, const std::list<std::string>& order_list, const std::list<std::string>& disable_list);
	~StarDictPlugins();
	void get_plugin_list(const std::list<std::string>& order_list, std::list<std::pair<StarDictPlugInType, std::list<StarDictPluginInfo> > > &plugin_list);
	bool get_loaded(const char *filename);
	void load_plugin(const char *filename);
	void configure_plugin(const char *filename, StarDictPlugInType plugin_type);
	void unload_plugin(const char *filename, StarDictPlugInType plugin_type);
	void reorder(const std::list<std::string>& order_list);
	StarDictVirtualDictPlugins VirtualDictPlugins;
	StarDictNetDictPlugins NetDictPlugins;
	StarDictTtsPlugins TtsPlugins;
	StarDictParseDataPlugins ParseDataPlugins;
private:
	StarDictMiscPlugins MiscPlugins;
	std::string plugindirpath;
	std::list<std::string> loaded_plugin_list;
	void load(const char *dirpath, const std::list<std::string>& order_list, const std::list<std::string>& disable_list);
	void get_plugin_info(const char *filename, StarDictPlugInType &plugin_type, std::string &info_xml, bool &can_configure);
	friend class PluginLoader;
	friend class PluginInfoLoader;
};

#endif
