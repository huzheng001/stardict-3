#include "pluginmanager.h"
#include <string>

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

StarDictPlugins::StarDictPlugins(const char *dirpath, const std::list<std::string>& disable_list)
{
	plugindirpath = dirpath;
	load(dirpath, disable_list);
}

StarDictPlugins::~StarDictPlugins()
{
}

void StarDictPlugins::load(const char *dirpath, const std::list<std::string>& disable_list)
{
	GDir *dir = g_dir_open(dirpath, 0, NULL);
	if (dir) {
		const gchar *filename;
		while ((filename = g_dir_read_name(dir))!=NULL) {
			if (g_str_has_suffix(filename, "."G_MODULE_SUFFIX)) {
				std::string fullfilename = dirpath;
				fullfilename += G_DIR_SEPARATOR;
				fullfilename += filename;
				bool disable = false;
				for (std::list<std::string>::const_iterator iter = disable_list.begin(); iter != disable_list.end(); ++iter) {
					if (*iter == fullfilename) {
						disable = true;
						break;
					}
				}
				if (!disable)
					load_plugin(fullfilename.c_str());
			}
		}
		g_dir_close(dir);
	}
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

void StarDictPlugins::get_plugin_list(std::list<std::pair<StarDictPlugInType, std::list<StarDictPluginInfo> > > &plugin_list)
{
	plugin_list.clear();
	std::list<StarDictPluginInfo> virtualdict_pluginlist;
	std::list<StarDictPluginInfo> tts_pluginlist;
	GDir *dir = g_dir_open(plugindirpath.c_str(), 0, NULL);
	if (dir) {
		const gchar *filename;
		while ((filename = g_dir_read_name(dir))!=NULL) {
			if (g_str_has_suffix(filename, "."G_MODULE_SUFFIX)) {
				std::string fullfilename = plugindirpath;
				fullfilename += G_DIR_SEPARATOR;
				fullfilename += filename;
				StarDictPlugInType plugin_type = StarDictPlugInType_UNKNOWN;
				std::string info_xml;
				bool can_configure;
				get_plugin_info(fullfilename.c_str(), plugin_type, info_xml, can_configure);
				if (plugin_type != StarDictPlugInType_UNKNOWN && (!info_xml.empty())) {
					StarDictPluginInfo plugin_info;
					plugin_info.filename = fullfilename;
					plugin_info.plugin_type = plugin_type;
					plugin_info.info_xml = info_xml;
					plugin_info.can_configure = can_configure;
					switch (plugin_type) {
						case StarDictPlugInType_VIRTUALDICT:
							virtualdict_pluginlist.push_back(plugin_info);
							break;
						case StarDictPlugInType_TTS:
							tts_pluginlist.push_back(plugin_info);
							break;
						default:
							break;
					}
				}
			}
		}
		g_dir_close(dir);
	}
	if (!virtualdict_pluginlist.empty()) {
		plugin_list.push_back(std::pair<StarDictPlugInType, std::list<StarDictPluginInfo> >(StarDictPlugInType_VIRTUALDICT, virtualdict_pluginlist));
	}
	if (!tts_pluginlist.empty()) {
		plugin_list.push_back(std::pair<StarDictPlugInType, std::list<StarDictPluginInfo> >(StarDictPlugInType_TTS, tts_pluginlist));
	}
}

typedef bool (*stardict_plugin_init_func_t)(StarDictPlugInObject *obj);

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
	bool failed = func.stardict_plugin_init(plugin_obj);
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
typedef bool (*stardict_tts_plugin_init_func_t)(StarDictTtsPlugInObject *obj);

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
	bool failed = func.stardict_plugin_init(plugin_obj);
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
			printf("Load %s failed: No stardict_virtualdict_plugin_init func!\n", filename);
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
	} else if (ptype == StarDictPlugInType_TTS) {
		union {
			stardict_tts_plugin_init_func_t stardict_tts_plugin_init;
			gpointer stardict_tts_plugin_init_avoid_warning;
		} func2;
		func2.stardict_tts_plugin_init = 0;
		if (!g_module_symbol (module, "stardict_tts_plugin_init", (gpointer *)&(func2.stardict_tts_plugin_init_avoid_warning))) {
			printf("Load %s failed: No stardict_tts_plugin_init func!\n", filename);
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
	} else {
		g_print("Load %s failed: Unknow type plugin!\n", filename);
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
		case StarDictPlugInType_TTS:
			TtsPlugins.unload_plugin(filename);
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
		case StarDictPlugInType_TTS:
			TtsPlugins.configure_plugin(filename);
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

void StarDictVirtualDictPlugins::SetDictMask(std::vector<InstantDictIndex> &dictmask)
{
	InstantDictIndex instance_dict_index;
	instance_dict_index.type = InstantDictType_VIRTUAL;
	for (std::vector<StarDictVirtualDictPlugin *>::size_type i = 0; i < oPlugins.size(); i++) {
		if (oPlugins[i]->is_instant()) {
			instance_dict_index.index = i;
			dictmask.push_back(instance_dict_index);
		}
	}
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

bool StarDictVirtualDictPlugin::is_instant()
{
	return obj->is_instant;
}

const char *StarDictVirtualDictPlugin::dict_name()
{
	return obj->dict_name;
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
