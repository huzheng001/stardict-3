/*
 * Copyright 2011 kubtek <kubtek@mail.com>
 *
 * This file is part of StarDict.
 *
 * StarDict is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * StarDict is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with StarDict.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include <string>
#include <cstring>

#include "pluginmanager.h"
#include "file-utils.h"
#include "iappdirs.h"

StarDictPluginBaseObject::StarDictPluginBaseObject(const char *filename, GModule *module_, plugin_configure_func_t configure_func_):
	plugin_filename(filename), module(module_), configure_func(configure_func_)
{
}

StarDictPluginBase::StarDictPluginBase(StarDictPluginBaseObject *baseobj_):
	baseobj(baseobj_)
{
}

typedef void (*stardict_plugin_exit_func_t)(void);

StarDictPluginBase::~StarDictPluginBase()
{
	union {
		stardict_plugin_exit_func_t stardict_plugin_exit;
		gpointer stardict_plugin_exit_avoid_warning;
	} func;
	func.stardict_plugin_exit = 0;
	if (g_module_symbol (baseobj->module, "stardict_plugin_exit", (gpointer *)&(func.stardict_plugin_exit_avoid_warning))) {
		func.stardict_plugin_exit();
	}
	g_module_close (baseobj->module);
	delete baseobj;
}

void StarDictPluginBase::configure()
{
	if (baseobj->configure_func != NULL)
		baseobj->configure_func();
}

const char *StarDictPluginBase::get_filename()
{
	return baseobj->plugin_filename.c_str();
}

StarDictPlugins::StarDictPlugins(const std::string& dirpath,
	const std::list<std::string>& order_list,
	const std::list<std::string>& disable_list)
{
	plugindirpath = dirpath;
	load(dirpath, order_list, disable_list);
}

StarDictPlugins::~StarDictPlugins()
{
}

class PluginLoader {
public:
	PluginLoader(StarDictPlugins& plugins_): plugins(plugins_) {}
	void operator()(const std::string& url, bool disable) {
		if (!disable)
			plugins.load_plugin(url.c_str());
	}
private:
	StarDictPlugins& plugins;
};

void StarDictPlugins::load(const std::string& dirpath, const std::list<std::string>& order_list, const std::list<std::string>& disable_list)
{
	std::list<std::string> plugins_dirs;
	plugins_dirs.push_back(dirpath);
	for_each_file(plugins_dirs, "." G_MODULE_SUFFIX, order_list, disable_list, PluginLoader(*this));
}

void StarDictPlugins::reorder(const std::list<std::string>& order_list)
{
	VirtualDictPlugins.reorder(order_list);
	NetDictPlugins.reorder(order_list);
	SpecialDictPlugins.reorder(order_list);
	TtsPlugins.reorder(order_list);
	ParseDataPlugins.reorder(order_list);
	MiscPlugins.reorder(order_list);
}

bool StarDictPlugins::get_loaded(const char *filename)
{
	bool found = false;
	for (std::list<std::string>::iterator iter = loaded_plugin_list.begin(); iter != loaded_plugin_list.end(); ++iter) {
		if (*iter == filename) {
			found = true;
			break;
		}
	}
	return found;
}

class PluginInfoLoader {
public:
	PluginInfoLoader(StarDictPlugins& plugins_, std::list<StarDictPluginInfo> &virtualdict_pluginlist_, std::list<StarDictPluginInfo> &netdict_pluginlist_, std::list<StarDictPluginInfo> &specialdict_pluginlist_, std::list<StarDictPluginInfo> &tts_pluginlist_, std::list<StarDictPluginInfo> &parsedata_pluginlist_, std::list<StarDictPluginInfo> &misc_pluginlist_): plugins(plugins_), virtualdict_pluginlist(virtualdict_pluginlist_), netdict_pluginlist(netdict_pluginlist_), specialdict_pluginlist(specialdict_pluginlist_), tts_pluginlist(tts_pluginlist_), parsedata_pluginlist(parsedata_pluginlist_), misc_pluginlist(misc_pluginlist_) {}
	void operator()(const std::string& url, bool disable) {
		if (!disable) {
			StarDictPlugInType plugin_type = StarDictPlugInType_UNKNOWN;
			std::string info_xml;
			bool can_configure;
			plugins.get_plugin_info(url.c_str(), plugin_type, info_xml, can_configure);
			if (plugin_type != StarDictPlugInType_UNKNOWN && (!info_xml.empty())) {
				StarDictPluginInfo plugin_info;
				plugin_info.filename = url;
				plugin_info.plugin_type = plugin_type;
				plugin_info.info_xml = info_xml;
				plugin_info.can_configure = can_configure;
				switch (plugin_type) {
					case StarDictPlugInType_VIRTUALDICT:
						virtualdict_pluginlist.push_back(plugin_info);
						break;
					case StarDictPlugInType_NETDICT:
						netdict_pluginlist.push_back(plugin_info);
						break;
					case StarDictPlugInType_SPECIALDICT:
						specialdict_pluginlist.push_back(plugin_info);
						break;
					case StarDictPlugInType_TTS:
						tts_pluginlist.push_back(plugin_info);
						break;
					case StarDictPlugInType_PARSEDATA:
						parsedata_pluginlist.push_back(plugin_info);
						break;
					case StarDictPlugInType_MISC:
						misc_pluginlist.push_back(plugin_info);
						break;
					default:
						break;
				}
			}
		}
	}
private:
	StarDictPlugins& plugins;
	std::list<StarDictPluginInfo> &virtualdict_pluginlist;
	std::list<StarDictPluginInfo> &netdict_pluginlist;
	std::list<StarDictPluginInfo> &specialdict_pluginlist;
	std::list<StarDictPluginInfo> &tts_pluginlist;
	std::list<StarDictPluginInfo> &parsedata_pluginlist;
	std::list<StarDictPluginInfo> &misc_pluginlist;
};

void StarDictPlugins::get_plugin_list(const std::list<std::string>& order_list, std::list<std::pair<StarDictPlugInType, std::list<StarDictPluginInfo> > > &plugin_list)
{
	plugin_list.clear();
	std::list<StarDictPluginInfo> virtualdict_pluginlist;
	std::list<StarDictPluginInfo> netdict_pluginlist;
	std::list<StarDictPluginInfo> specialdict_pluginlist;
	std::list<StarDictPluginInfo> tts_pluginlist;
	std::list<StarDictPluginInfo> parsedata_pluginlist;
	std::list<StarDictPluginInfo> misc_pluginlist;

	std::list<std::string> plugins_dirs;
	plugins_dirs.push_back(plugindirpath);
	std::list<std::string> disable_list;
	for_each_file(plugins_dirs, "." G_MODULE_SUFFIX, order_list, disable_list, PluginInfoLoader(*this, virtualdict_pluginlist, netdict_pluginlist, specialdict_pluginlist, tts_pluginlist, parsedata_pluginlist, misc_pluginlist));

	if (!virtualdict_pluginlist.empty()) {
		plugin_list.push_back(std::pair<StarDictPlugInType, std::list<StarDictPluginInfo> >(StarDictPlugInType_VIRTUALDICT, virtualdict_pluginlist));
	}
	if (!netdict_pluginlist.empty()) {
		plugin_list.push_back(std::pair<StarDictPlugInType, std::list<StarDictPluginInfo> >(StarDictPlugInType_NETDICT, netdict_pluginlist));
	}
	if (!specialdict_pluginlist.empty()) {
		plugin_list.push_back(std::pair<StarDictPlugInType, std::list<StarDictPluginInfo> >(StarDictPlugInType_SPECIALDICT, specialdict_pluginlist));
	}
	if (!tts_pluginlist.empty()) {
		plugin_list.push_back(std::pair<StarDictPlugInType, std::list<StarDictPluginInfo> >(StarDictPlugInType_TTS, tts_pluginlist));
	}
	if (!parsedata_pluginlist.empty()) {
		plugin_list.push_back(std::pair<StarDictPlugInType, std::list<StarDictPluginInfo> >(StarDictPlugInType_PARSEDATA, parsedata_pluginlist));
	}
	if (!misc_pluginlist.empty()) {
		plugin_list.push_back(std::pair<StarDictPlugInType, std::list<StarDictPluginInfo> >(StarDictPlugInType_MISC, misc_pluginlist));
	}
}

typedef bool (*stardict_plugin_init_func_t)(StarDictPlugInObject *obj, IAppDirs* appDirs);

void StarDictPlugins::get_plugin_info(const char *filename, StarDictPlugInType &plugin_type, std::string &info_xml, bool &can_configure)
{
	GModule *module;
	module = g_module_open (filename, G_MODULE_BIND_LAZY);
	if (!module) {
		g_print("Load %s failed!\n", filename);
		return;
	}
	union {
		stardict_plugin_init_func_t stardict_plugin_init;
		gpointer stardict_plugin_init_avoid_warning;
	} func;
	func.stardict_plugin_init = 0;
	if (!g_module_symbol (module, "stardict_plugin_init", (gpointer *)&(func.stardict_plugin_init_avoid_warning))) {
		g_print("Load %s failed: No stardict_plugin_init func!\n", filename);
		g_module_close (module);
		return;
	}
	StarDictPlugInObject *plugin_obj = new StarDictPlugInObject();
	bool failed = func.stardict_plugin_init(plugin_obj, app_dirs);
	if (failed) {
		g_print("Load %s failed!\n", filename);
		g_module_close (module);
		delete plugin_obj;
		return;
	}
	plugin_type = plugin_obj->type;
	info_xml = plugin_obj->info_xml;
	can_configure = (plugin_obj->configure_func != NULL);
	delete plugin_obj;
	g_module_close (module);
}

typedef bool (*stardict_virtualdict_plugin_init_func_t)(StarDictVirtualDictPlugInObject *obj);
typedef bool (*stardict_netdict_plugin_init_func_t)(StarDictNetDictPlugInObject *obj);
typedef bool (*stardict_specialdict_plugin_init_func_t)(StarDictSpecialDictPlugInObject *obj);
typedef bool (*stardict_tts_plugin_init_func_t)(StarDictTtsPlugInObject *obj);
typedef bool (*stardict_parsedata_plugin_init_func_t)(StarDictParseDataPlugInObject *obj);
typedef bool (*stardict_misc_plugin_init_func_t)(void);

void StarDictPlugins::load_plugin(const char *filename)
{
	bool found = false;
	for (std::list<std::string>::iterator iter = loaded_plugin_list.begin(); iter != loaded_plugin_list.end(); ++iter) {
		if (*iter == filename) {
			found = true;
			break;
		}
	}
	if (found)
		return;
	loaded_plugin_list.push_back(filename);
	GModule *module;
	module = g_module_open (filename, G_MODULE_BIND_LAZY);
	if (!module) {
		g_print("Load %s failed!\n", filename);
		return;
	}
	union {
		stardict_plugin_init_func_t stardict_plugin_init;
		gpointer stardict_plugin_init_avoid_warning;
	} func;
	func.stardict_plugin_init = 0;
	if (!g_module_symbol (module, "stardict_plugin_init", (gpointer *)&(func.stardict_plugin_init_avoid_warning))) {
		g_print("Load %s failed: No stardict_plugin_init func!\n", filename);
		g_module_close (module);
		return;
	}
	StarDictPlugInObject *plugin_obj = new StarDictPlugInObject();
	bool failed = func.stardict_plugin_init(plugin_obj, app_dirs);
	if (failed) {
		g_print("Load %s failed!\n", filename);
		g_module_close (module);
		delete plugin_obj;
		return;
	}
	StarDictPlugInType ptype = plugin_obj->type;
	StarDictPluginBaseObject *baseobj = new StarDictPluginBaseObject(filename, module, plugin_obj->configure_func);
	delete plugin_obj;
	if (ptype == StarDictPlugInType_VIRTUALDICT) {
		union {
			stardict_virtualdict_plugin_init_func_t stardict_virtualdict_plugin_init;
			gpointer stardict_virtualdict_plugin_init_avoid_warning;
		} func2;
		func2.stardict_virtualdict_plugin_init = 0;
		if (!g_module_symbol (module, "stardict_virtualdict_plugin_init", (gpointer *)&(func2.stardict_virtualdict_plugin_init_avoid_warning))) {
			g_print("Load %s failed: No stardict_virtualdict_plugin_init func!\n", filename);
			g_module_close (module);
			delete baseobj;
			return;
		}
		StarDictVirtualDictPlugInObject *virtualdict_plugin_obj = new StarDictVirtualDictPlugInObject();
		failed = func2.stardict_virtualdict_plugin_init(virtualdict_plugin_obj);
		if (failed) {
			g_print("Load %s failed!\n", filename);
			g_module_close (module);
			delete baseobj;
			delete virtualdict_plugin_obj;
			return;
		}
		VirtualDictPlugins.add(baseobj, virtualdict_plugin_obj);
	} else if (ptype == StarDictPlugInType_NETDICT) {
		union {
			stardict_netdict_plugin_init_func_t stardict_netdict_plugin_init;
			gpointer stardict_netdict_plugin_init_avoid_warning;
		} func2;
		func2.stardict_netdict_plugin_init = 0;
		if (!g_module_symbol (module, "stardict_netdict_plugin_init", (gpointer *)&(func2.stardict_netdict_plugin_init_avoid_warning))) {
			g_print("Load %s failed: No stardict_netdict_plugin_init func!\n", filename);
			g_module_close (module);
			delete baseobj;
			return;
		}
		StarDictNetDictPlugInObject *netdict_plugin_obj = new StarDictNetDictPlugInObject();
		failed = func2.stardict_netdict_plugin_init(netdict_plugin_obj);
		if (failed) {
			g_print("Load %s failed!\n", filename);
			g_module_close (module);
			delete baseobj;
			delete netdict_plugin_obj;
			return;
		}
		NetDictPlugins.add(baseobj, netdict_plugin_obj);
	} else if (ptype == StarDictPlugInType_SPECIALDICT) {
		union {
			stardict_specialdict_plugin_init_func_t stardict_specialdict_plugin_init;
			gpointer stardict_specialdict_plugin_init_avoid_warning;
		} func2;
		func2.stardict_specialdict_plugin_init = 0;
		if (!g_module_symbol (module, "stardict_specialdict_plugin_init", (gpointer *)&(func2.stardict_specialdict_plugin_init_avoid_warning))) {
			g_print("Load %s failed: No stardict_specialdict_plugin_init func!\n", filename);
			g_module_close (module);
			delete baseobj;
			return;
		}
		StarDictSpecialDictPlugInObject *specialdict_plugin_obj = new StarDictSpecialDictPlugInObject();
		failed = func2.stardict_specialdict_plugin_init(specialdict_plugin_obj);
		if (failed) {
			g_print("Load %s failed!\n", filename);
			g_module_close (module);
			delete baseobj;
			delete specialdict_plugin_obj;
			return;
		}
		SpecialDictPlugins.add(baseobj, specialdict_plugin_obj);
	} else if (ptype == StarDictPlugInType_TTS) {
		union {
			stardict_tts_plugin_init_func_t stardict_tts_plugin_init;
			gpointer stardict_tts_plugin_init_avoid_warning;
		} func2;
		func2.stardict_tts_plugin_init = 0;
		if (!g_module_symbol (module, "stardict_tts_plugin_init", (gpointer *)&(func2.stardict_tts_plugin_init_avoid_warning))) {
			g_print("Load %s failed: No stardict_tts_plugin_init func!\n", filename);
			g_module_close (module);
			delete baseobj;
			return;
		}
		StarDictTtsPlugInObject *tts_plugin_obj = new StarDictTtsPlugInObject();
		failed = func2.stardict_tts_plugin_init(tts_plugin_obj);
		if (failed) {
			g_print("Load %s failed!\n", filename);
			g_module_close (module);
			delete baseobj;
			delete tts_plugin_obj;
			return;
		}
		TtsPlugins.add(baseobj, tts_plugin_obj);
	} else if (ptype == StarDictPlugInType_PARSEDATA) {
		union {
			stardict_parsedata_plugin_init_func_t stardict_parsedata_plugin_init;
			gpointer stardict_parsedata_plugin_init_avoid_warning;
		} func3;
		func3.stardict_parsedata_plugin_init = 0;
		if (!g_module_symbol (module, "stardict_parsedata_plugin_init", (gpointer *)&(func3.stardict_parsedata_plugin_init_avoid_warning))) {
			g_print("Load %s failed: No stardict_parsedata_plugin_init func!\n", filename);
			g_module_close (module);
			delete baseobj;
			return;
		}
		StarDictParseDataPlugInObject *parsedata_plugin_obj = new StarDictParseDataPlugInObject();
		failed = func3.stardict_parsedata_plugin_init(parsedata_plugin_obj);
		if (failed) {
			g_print("Load %s failed!\n", filename);
			g_module_close (module);
			delete baseobj;
			delete parsedata_plugin_obj;
			return;
		}
		ParseDataPlugins.add(baseobj, parsedata_plugin_obj);
	} else if (ptype == StarDictPlugInType_MISC) {
		union {
			stardict_misc_plugin_init_func_t stardict_misc_plugin_init;
			gpointer stardict_misc_plugin_init_avoid_warning;
		} func4;
		func4.stardict_misc_plugin_init = 0;
		if (!g_module_symbol (module, "stardict_misc_plugin_init", (gpointer *)&(func4.stardict_misc_plugin_init_avoid_warning))) {
			g_print("Load %s failed: No stardict_misc_plugin_init func!\n", filename);
			g_module_close (module);
			delete baseobj;
			return;
		}
		failed = func4.stardict_misc_plugin_init();
		if (failed) {
			g_print("Load %s failed!\n", filename);
			g_module_close (module);
			delete baseobj;
			return;
		}
		MiscPlugins.add(baseobj);
	} else {
		g_print("Load %s failed: Unknown type plugin!\n", filename);
		g_module_close (module);
		delete baseobj;
		return;
	}
}

void StarDictPlugins::unload_plugin(const char *filename, StarDictPlugInType plugin_type)
{
	for (std::list<std::string>::iterator iter = loaded_plugin_list.begin(); iter != loaded_plugin_list.end(); ++iter) {
		if (*iter == filename) {
			loaded_plugin_list.erase(iter);
			break;
		}
	}
	switch (plugin_type) {
		case StarDictPlugInType_VIRTUALDICT:
			VirtualDictPlugins.unload_plugin(filename);
			break;
		case StarDictPlugInType_NETDICT:
			NetDictPlugins.unload_plugin(filename);
			break;
		case StarDictPlugInType_SPECIALDICT:
			SpecialDictPlugins.unload_plugin(filename);
			break;
		case StarDictPlugInType_TTS:
			TtsPlugins.unload_plugin(filename);
			break;
		case StarDictPlugInType_PARSEDATA:
			ParseDataPlugins.unload_plugin(filename);
			break;
		case StarDictPlugInType_MISC:
			MiscPlugins.unload_plugin(filename);
			break;
		default:
			break;
	}
}

void StarDictPlugins::configure_plugin(const char *filename, StarDictPlugInType plugin_type)
{
	switch (plugin_type) {
		case StarDictPlugInType_VIRTUALDICT:
			VirtualDictPlugins.configure_plugin(filename);
			break;
		case StarDictPlugInType_NETDICT:
			NetDictPlugins.configure_plugin(filename);
			break;
		case StarDictPlugInType_SPECIALDICT:
			SpecialDictPlugins.configure_plugin(filename);
			break;
		case StarDictPlugInType_TTS:
			TtsPlugins.configure_plugin(filename);
			break;
		case StarDictPlugInType_PARSEDATA:
			ParseDataPlugins.configure_plugin(filename);
			break;
		case StarDictPlugInType_MISC:
			MiscPlugins.configure_plugin(filename);
			break;
		default:
			break;
	}
}

//
// class StarDictVirtualDictPlugins begin.
//

StarDictVirtualDictPlugins::StarDictVirtualDictPlugins()
{
}

StarDictVirtualDictPlugins::~StarDictVirtualDictPlugins()
{
	for (std::vector<StarDictVirtualDictPlugin *>::iterator i = oPlugins.begin(); i != oPlugins.end(); ++i) {
		delete *i;
	}
}

void StarDictVirtualDictPlugins::add(StarDictPluginBaseObject *baseobj, StarDictVirtualDictPlugInObject *virtualdict_plugin_obj)
{
	StarDictVirtualDictPlugin *plugin = new StarDictVirtualDictPlugin(baseobj, virtualdict_plugin_obj);
	oPlugins.push_back(plugin);
}

void StarDictVirtualDictPlugins::unload_plugin(const char *filename)
{
	for (std::vector<StarDictVirtualDictPlugin *>::iterator iter = oPlugins.begin(); iter != oPlugins.end(); ++iter) {
		if (strcmp((*iter)->get_filename(), filename) == 0) {
			delete *iter;
			oPlugins.erase(iter);
			break;
		}
	}
}

void StarDictVirtualDictPlugins::configure_plugin(const char *filename)
{
	for (std::vector<StarDictVirtualDictPlugin *>::iterator iter = oPlugins.begin(); iter != oPlugins.end(); ++iter) {
		if (strcmp((*iter)->get_filename(), filename) == 0) {
			(*iter)->configure();
			break;
		}
	}
}

void StarDictVirtualDictPlugins::lookup(size_t iPlugin, const gchar *word, char ***pppWord, char ****ppppWordData)
{
	oPlugins[iPlugin]->lookup(word, pppWord, ppppWordData);
}

const char *StarDictVirtualDictPlugins::dict_name(size_t iPlugin)
{
	return oPlugins[iPlugin]->dict_name();
}

const char *StarDictVirtualDictPlugins::dict_id(size_t iPlugin)
{
	return oPlugins[iPlugin]->get_filename();
}

bool StarDictVirtualDictPlugins::find_dict_by_id(const DictItemId& id, size_t &iPlugin) const
{
	for (std::vector<StarDictVirtualDictPlugin *>::size_type i = 0; i < oPlugins.size(); i++) {
		if (is_equal_paths(oPlugins[i]->get_filename(), id.str())) {
			iPlugin = i;
			return true;
		}
	}
	return false;
}

//
// class StarDictVirtualDictPlugin begin.
//

StarDictVirtualDictPlugin::StarDictVirtualDictPlugin(StarDictPluginBaseObject *baseobj_, StarDictVirtualDictPlugInObject *virtualdict_plugin_obj):
	StarDictPluginBase(baseobj_)
{
	obj = virtualdict_plugin_obj;
}

StarDictVirtualDictPlugin::~StarDictVirtualDictPlugin()
{
	delete obj;
}

void StarDictVirtualDictPlugin::lookup(const char *word, char ***pppWord, char ****ppppWordData)
{
	obj->lookup_func(word, pppWord, ppppWordData);
}

const char *StarDictVirtualDictPlugin::dict_name()
{
	return obj->dict_name;
}

//
// class StarDictNetDictPlugins begin.
//

StarDictNetDictPlugins::StarDictNetDictPlugins()
{
}

StarDictNetDictPlugins::~StarDictNetDictPlugins()
{
	for (std::vector<StarDictNetDictPlugin *>::iterator i = oPlugins.begin(); i != oPlugins.end(); ++i) {
		delete *i;
	}
}

void StarDictNetDictPlugins::add(StarDictPluginBaseObject *baseobj, StarDictNetDictPlugInObject *netdict_plugin_obj)
{
	StarDictNetDictPlugin *plugin = new StarDictNetDictPlugin(baseobj, netdict_plugin_obj);
	oPlugins.push_back(plugin);
}

void StarDictNetDictPlugins::unload_plugin(const char *filename)
{
	for (std::vector<StarDictNetDictPlugin *>::iterator iter = oPlugins.begin(); iter != oPlugins.end(); ++iter) {
		if (strcmp((*iter)->get_filename(), filename) == 0) {
			delete *iter;
			oPlugins.erase(iter);
			break;
		}
	}
}

void StarDictNetDictPlugins::configure_plugin(const char *filename)
{
	for (std::vector<StarDictNetDictPlugin *>::iterator iter = oPlugins.begin(); iter != oPlugins.end(); ++iter) {
		if (strcmp((*iter)->get_filename(), filename) == 0) {
			(*iter)->configure();
			break;
		}
	}
}

void StarDictNetDictPlugins::lookup(size_t iPlugin, const gchar *word, bool ismainwin)
{
	oPlugins[iPlugin]->lookup(word, ismainwin);
}

const char *StarDictNetDictPlugins::dict_name(size_t iPlugin)
{
	return oPlugins[iPlugin]->dict_name();
}

const char *StarDictNetDictPlugins::dict_link(size_t iPlugin)
{
	return oPlugins[iPlugin]->dict_link();
}

const char *StarDictNetDictPlugins::dict_id(size_t iPlugin)
{
	return oPlugins[iPlugin]->get_filename();
}

const char *StarDictNetDictPlugins::dict_cacheid(size_t iPlugin)
{
	return oPlugins[iPlugin]->dict_cacheid();
}

bool StarDictNetDictPlugins::find_dict_by_id(const DictItemId& id, size_t &iPlugin) const
{
	for (std::vector<StarDictNetDictPlugin *>::size_type i = 0; i < oPlugins.size(); i++) {
		if (is_equal_paths(oPlugins[i]->get_filename(), id.str())) {
			iPlugin = i;
			return true;
		}
	}
	return false;
}

//
// class StarDictNetDictPlugin begin.
//

StarDictNetDictPlugin::StarDictNetDictPlugin(StarDictPluginBaseObject *baseobj_, StarDictNetDictPlugInObject *netdict_plugin_obj):
	StarDictPluginBase(baseobj_)
{
	obj = netdict_plugin_obj;
}

StarDictNetDictPlugin::~StarDictNetDictPlugin()
{
	delete obj;
}

void StarDictNetDictPlugin::lookup(const char *word, bool ismainwin)
{
	obj->lookup_func(word, ismainwin);
}

const char *StarDictNetDictPlugin::dict_name()
{
	return obj->dict_name;
}

const char *StarDictNetDictPlugin::dict_link()
{
	return obj->dict_link;
}

const char *StarDictNetDictPlugin::dict_cacheid()
{
	return obj->dict_cacheid;
}

//
// class StarDictSpecialDictPlugins begin.
//

StarDictSpecialDictPlugins::StarDictSpecialDictPlugins()
{
}

StarDictSpecialDictPlugins::~StarDictSpecialDictPlugins()
{
	for (std::vector<StarDictSpecialDictPlugin *>::iterator i = oPlugins.begin(); i != oPlugins.end(); ++i) {
		delete *i;
	}
}

void StarDictSpecialDictPlugins::add(StarDictPluginBaseObject *baseobj, StarDictSpecialDictPlugInObject *specialdict_plugin_obj)
{
	StarDictSpecialDictPlugin *plugin = new StarDictSpecialDictPlugin(baseobj, specialdict_plugin_obj);
	oPlugins.push_back(plugin);
}

void StarDictSpecialDictPlugins::unload_plugin(const char *filename)
{
	for (std::vector<StarDictSpecialDictPlugin *>::iterator iter = oPlugins.begin(); iter != oPlugins.end(); ++iter) {
		if (strcmp((*iter)->get_filename(), filename) == 0) {
			delete *iter;
			oPlugins.erase(iter);
			break;
		}
	}
}

void StarDictSpecialDictPlugins::configure_plugin(const char *filename)
{
	for (std::vector<StarDictSpecialDictPlugin *>::iterator iter = oPlugins.begin(); iter != oPlugins.end(); ++iter) {
		if (strcmp((*iter)->get_filename(), filename) == 0) {
			(*iter)->configure();
			break;
		}
	}
}

void StarDictSpecialDictPlugins::render_widget(size_t iPlugin, bool ismainwin, size_t dictid, const gchar *orig_word, gchar **Word, gchar ***WordData, GtkWidget **widget)
{
	oPlugins[iPlugin]->render_widget(ismainwin, dictid, orig_word, Word, WordData, widget);
}

const char *StarDictSpecialDictPlugins::dict_type(size_t iPlugin)
{
	return oPlugins[iPlugin]->dict_type();
}

//
// class StarDictSpecialDictPlugin begin.
//

StarDictSpecialDictPlugin::StarDictSpecialDictPlugin(StarDictPluginBaseObject *baseobj_, StarDictSpecialDictPlugInObject *specialdict_plugin_obj):
	StarDictPluginBase(baseobj_)
{
	obj = specialdict_plugin_obj;
}

StarDictSpecialDictPlugin::~StarDictSpecialDictPlugin()
{
	delete obj;
}

void StarDictSpecialDictPlugin::render_widget(bool ismainwin, size_t dictid, const gchar *orig_word, gchar **Word, gchar ***WordData, GtkWidget **widget)
{
	obj->render_widget_func(ismainwin, dictid, orig_word, Word, WordData, widget);
}

const char *StarDictSpecialDictPlugin::dict_type()
{
	return obj->dict_type;
}

//
// class StarDictTtsPlugins begin.
//

StarDictTtsPlugins::StarDictTtsPlugins()
{
}

StarDictTtsPlugins::~StarDictTtsPlugins()
{
	for (std::vector<StarDictTtsPlugin *>::iterator i = oPlugins.begin(); i != oPlugins.end(); ++i) {
		delete *i;
	}
}

void StarDictTtsPlugins::add(StarDictPluginBaseObject *baseobj, StarDictTtsPlugInObject *tts_plugin_obj)
{
	StarDictTtsPlugin *plugin = new StarDictTtsPlugin(baseobj, tts_plugin_obj);
	oPlugins.push_back(plugin);
}

void StarDictTtsPlugins::unload_plugin(const char *filename)
{
	for (std::vector<StarDictTtsPlugin *>::iterator iter = oPlugins.begin(); iter != oPlugins.end(); ++iter) {
		if (strcmp((*iter)->get_filename(), filename) == 0) {
			delete *iter;
			oPlugins.erase(iter);
			break;
		}
	}
}

void StarDictTtsPlugins::configure_plugin(const char *filename)
{
	for (std::vector<StarDictTtsPlugin *>::iterator iter = oPlugins.begin(); iter != oPlugins.end(); ++iter) {
		if (strcmp((*iter)->get_filename(), filename) == 0) {
			(*iter)->configure();
			break;
		}
	}
}

void StarDictTtsPlugins::saytext(size_t iPlugin, const gchar *text)
{
	oPlugins[iPlugin]->saytext(text);
}

const char *StarDictTtsPlugins::tts_name(size_t iPlugin)
{
	return oPlugins[iPlugin]->tts_name();
}

//
// class StarDictTtsPlugin begin.
//

StarDictTtsPlugin::StarDictTtsPlugin(StarDictPluginBaseObject *baseobj_, StarDictTtsPlugInObject *tts_plugin_obj):
	StarDictPluginBase(baseobj_)
{
	obj = tts_plugin_obj;
}

StarDictTtsPlugin::~StarDictTtsPlugin()
{
	delete obj;
}

void StarDictTtsPlugin::saytext(const char *text)
{
	obj->saytext_func(text);
}

const char *StarDictTtsPlugin::tts_name()
{
	return obj->tts_name;
}

//
// class StarDictParseDataPlugins begin.
//

StarDictParseDataPlugins::StarDictParseDataPlugins()
{
}

StarDictParseDataPlugins::~StarDictParseDataPlugins()
{
	for (std::vector<StarDictParseDataPlugin *>::iterator i = oPlugins.begin(); i != oPlugins.end(); ++i) {
		delete *i;
	}
}

void StarDictParseDataPlugins::add(StarDictPluginBaseObject *baseobj, StarDictParseDataPlugInObject *tts_plugin_obj)
{
	StarDictParseDataPlugin *plugin = new StarDictParseDataPlugin(baseobj, tts_plugin_obj);
	oPlugins.push_back(plugin);
}

void StarDictParseDataPlugins::unload_plugin(const char *filename)
{
	for (std::vector<StarDictParseDataPlugin *>::iterator iter = oPlugins.begin(); iter != oPlugins.end(); ++iter) {
		if (strcmp((*iter)->get_filename(), filename) == 0) {
			delete *iter;
			oPlugins.erase(iter);
			break;
		}
	}
}

void StarDictParseDataPlugins::configure_plugin(const char *filename)
{
	for (std::vector<StarDictParseDataPlugin *>::iterator iter = oPlugins.begin(); iter != oPlugins.end(); ++iter) {
		if (strcmp((*iter)->get_filename(), filename) == 0) {
			(*iter)->configure();
			break;
		}
	}
}

bool StarDictParseDataPlugins::parse(size_t iPlugin, const char *p, unsigned int *parsed_size, ParseResult &result, const char *oword)
{
	return oPlugins[iPlugin]->parse(p, parsed_size, result, oword);
}

//
// class StarDictParseDataPlugin begin.
//

StarDictParseDataPlugin::StarDictParseDataPlugin(StarDictPluginBaseObject *baseobj_, StarDictParseDataPlugInObject *parsedata_plugin_obj):
	StarDictPluginBase(baseobj_)
{
	obj = parsedata_plugin_obj;
}

StarDictParseDataPlugin::~StarDictParseDataPlugin()
{
	delete obj;
}

bool StarDictParseDataPlugin::parse(const char *p, unsigned int *parsed_size, ParseResult &result, const char *oword)
{
	return obj->parse_func(p, parsed_size, result, oword);
}

//
// class StarDictMiscPlugins begin.
//

StarDictMiscPlugins::StarDictMiscPlugins()
{
}

StarDictMiscPlugins::~StarDictMiscPlugins()
{
	for (std::vector<StarDictMiscPlugin *>::iterator i = oPlugins.begin(); i != oPlugins.end(); ++i) {
		delete *i;
	}
}

void StarDictMiscPlugins::add(StarDictPluginBaseObject *baseobj)
{
	StarDictMiscPlugin *plugin = new StarDictMiscPlugin(baseobj);
	oPlugins.push_back(plugin);
}

void StarDictMiscPlugins::unload_plugin(const char *filename)
{
	for (std::vector<StarDictMiscPlugin *>::iterator iter = oPlugins.begin(); iter != oPlugins.end(); ++iter) {
		if (strcmp((*iter)->get_filename(), filename) == 0) {
			delete *iter;
			oPlugins.erase(iter);
			break;
		}
	}
}

void StarDictMiscPlugins::on_mainwin_finish()
{
	for (std::vector<StarDictMiscPlugin *>::iterator iter = oPlugins.begin(); iter != oPlugins.end(); ++iter) {
		(*iter)->on_mainwin_finish();
	}
}

void StarDictMiscPlugins::configure_plugin(const char *filename)
{
	for (std::vector<StarDictMiscPlugin *>::iterator iter = oPlugins.begin(); iter != oPlugins.end(); ++iter) {
		if (strcmp((*iter)->get_filename(), filename) == 0) {
			(*iter)->configure();
			break;
		}
	}
}

//
// class StarDictMiscPlugin begin.
//

StarDictMiscPlugin::StarDictMiscPlugin(StarDictPluginBaseObject *baseobj_):
	StarDictPluginBase(baseobj_)
{
}

StarDictMiscPlugin::~StarDictMiscPlugin()
{
}

typedef void (*stardict_misc_plugin_on_mainwin_finish_func_t)(void);

void StarDictMiscPlugin::on_mainwin_finish()
{
	//This function will only run once, so slove the symbol here.
	union {
		stardict_misc_plugin_on_mainwin_finish_func_t stardict_misc_plugin_on_mainwin_finish;
		gpointer stardict_misc_plugin_on_mainwin_finish_avoid_warning;
	} func4;
	func4.stardict_misc_plugin_on_mainwin_finish = 0;
	if (!g_module_symbol (baseobj->module, "stardict_misc_plugin_on_mainwin_finish", (gpointer *)&(func4.stardict_misc_plugin_on_mainwin_finish_avoid_warning))) {
		return;
	}
	func4.stardict_misc_plugin_on_mainwin_finish();
}


template<typename TV, typename TI>
void plugins_reorder(TV &oPlugins, const std::list<std::string>& order_list)
{
	TV prev(oPlugins);
	oPlugins.clear();
	for (std::list<std::string>::const_iterator i = order_list.begin(); i != order_list.end(); ++i) {
		for (TI j = prev.begin(); j != prev.end(); ++j) {
			if (*i == (*j)->get_filename()) {
				oPlugins.push_back(*j);
			}
		}
	}
	for (TI i=prev.begin(); i!=prev.end(); ++i) {
		TI j;
		for (j=oPlugins.begin(); j!=oPlugins.end(); ++j) {
			if (*j == *i)
				break;
		}
		if (j == oPlugins.end())
			delete *i;
	}
}

void StarDictVirtualDictPlugins::reorder(const std::list<std::string>& order_list)
{
	plugins_reorder<std::vector<StarDictVirtualDictPlugin *>, std::vector<StarDictVirtualDictPlugin *>::iterator>(oPlugins, order_list);
}

void StarDictNetDictPlugins::reorder(const std::list<std::string>& order_list)
{
	plugins_reorder<std::vector<StarDictNetDictPlugin *>, std::vector<StarDictNetDictPlugin *>::iterator>(oPlugins, order_list);
}

void StarDictSpecialDictPlugins::reorder(const std::list<std::string>& order_list)
{
	plugins_reorder<std::vector<StarDictSpecialDictPlugin *>, std::vector<StarDictSpecialDictPlugin *>::iterator>(oPlugins, order_list);
}

void StarDictTtsPlugins::reorder(const std::list<std::string>& order_list)
{
	plugins_reorder<std::vector<StarDictTtsPlugin *>, std::vector<StarDictTtsPlugin *>::iterator>(oPlugins, order_list);
}

void StarDictParseDataPlugins::reorder(const std::list<std::string>& order_list)
{
	plugins_reorder<std::vector<StarDictParseDataPlugin *>, std::vector<StarDictParseDataPlugin *>::iterator>(oPlugins, order_list);
}

void StarDictMiscPlugins::reorder(const std::list<std::string>& order_list)
{
	plugins_reorder<std::vector<StarDictMiscPlugin *>, std::vector<StarDictMiscPlugin *>::iterator>(oPlugins, order_list);
}
