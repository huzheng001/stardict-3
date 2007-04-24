#include "pluginmanager.h"
#include <string>

StarDictPlugins::StarDictPlugins(const char *dirpath)
{
	load(dirpath);
}

StarDictPlugins::~StarDictPlugins()
{
}

void StarDictPlugins::load(const char *dirpath)
{
	GDir *dir = g_dir_open(dirpath, 0, NULL);
	if (dir) {
		const gchar *filename;
		while ((filename = g_dir_read_name(dir))!=NULL) {
			if (g_str_has_suffix(filename, "."G_MODULE_SUFFIX)) {
				std::string fullfilename = dirpath;
				fullfilename += G_DIR_SEPARATOR;
				fullfilename += filename;
				load_plugin(fullfilename.c_str());
			}
		}
		g_dir_close(dir);
	}
}

typedef bool (*stardict_plugin_init_func_t)(StarDictPlugInObject *obj);
typedef bool (*stardict_virtualdict_plugin_init_func_t)(StarDictVirtualDictPlugInObject *obj);

void StarDictPlugins::load_plugin(const char *filename)
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
	StarDictPlugInType ptype = plugin_obj->type;
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
			return;
		}
		StarDictVirtualDictPlugInObject *virtualdict_plugin_obj = new StarDictVirtualDictPlugInObject();
		failed = func2.stardict_virtualdict_plugin_init(virtualdict_plugin_obj);
		if (failed) {
			g_print("Load %s failed!\n", filename);
			g_module_close (module);
			delete virtualdict_plugin_obj;
			return;
		}
		VirtualDictPlugins.add(module, virtualdict_plugin_obj);
	} else {
		g_print("Load %s failed: Unknow type plugin!\n", filename);
		g_module_close (module);
		return;
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

void StarDictVirtualDictPlugins::add(GModule *module, StarDictVirtualDictPlugInObject *virtualdict_plugin_obj)
{
	StarDictVirtualDictPlugin *plugin = new StarDictVirtualDictPlugin(module, virtualdict_plugin_obj);
	oPlugins.push_back(plugin);
}

void StarDictVirtualDictPlugins::lookup(const gchar *word, size_t iPlugin)
{
	oPlugins[iPlugin]->lookup(word);
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

StarDictVirtualDictPlugin::StarDictVirtualDictPlugin(GModule *module_, StarDictVirtualDictPlugInObject *virtualdict_plugin_obj)
{
	module = module_;
	obj = virtualdict_plugin_obj;
}

typedef void (*stardict_plugin_exit_func_t)(void);

StarDictVirtualDictPlugin::~StarDictVirtualDictPlugin()
{
	union {
		stardict_plugin_exit_func_t stardict_plugin_exit;
		gpointer stardict_plugin_exit_avoid_warning;
	} func;
	func.stardict_plugin_exit = 0;
	if (g_module_symbol (module, "stardict_plugin_exit", (gpointer *)&(func.stardict_plugin_exit_avoid_warning))) {
		func.stardict_plugin_exit();
	}
	g_module_close (module);
	delete obj;
}

void StarDictVirtualDictPlugin::lookup(const char *word)
{
	obj->lookup_func(word);
}

bool StarDictVirtualDictPlugin::is_instant()
{
	return obj->is_instant;
}
