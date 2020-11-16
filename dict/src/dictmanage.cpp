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

#include <glib.h>
#include <glib/gi18n.h>
#include <glib/gstdio.h>
#include "dictmanage.h"
#include "lib/file-utils.h"
#include "conf.h"
#include "stardict.h"
#include "lib/pluginmanager.h"
#include "lib/verify_dict.h"

class GetAllDictList {
public:
	GetAllDictList(std::list<DictItemId> &dict_all_list_) :
		dict_all_list(dict_all_list_) {}
	void operator()(const std::string& url, bool disable) {
		dict_all_list.push_back(DictItemId(url));
	}
private:
	std::list<DictItemId> &dict_all_list;
};

class GetAllPluginList {
public:
	GetAllPluginList(std::list<DictItemId> &plugin_all_list_) :
		plugin_all_list(plugin_all_list_) {}
	void operator()(const std::string& url, bool disable) {
		plugin_all_list.push_back(DictItemId(url));
	}
private:
	std::list<DictItemId> &plugin_all_list;
};

/* List of dictionaries that are present on the hard disk in known directories. */
static void get_all_dict_list(std::list<DictItemId> &dict_all_list)
{
	std::list<std::string> dict_order_list;
	std::list<std::string> dict_disable_list;
#ifdef _WIN32
	std::list<std::string> dict_dirs_list;
	{
		const std::list<std::string>& dict_dirs_list_rel
			= conf->get_strlist("/apps/stardict/manage_dictionaries/dict_dirs_list");
		abs_path_to_data_dir(dict_dirs_list_rel, dict_dirs_list);
	}
#else
	const std::list<std::string>& dict_dirs_list
		= conf->get_strlist("/apps/stardict/manage_dictionaries/dict_dirs_list");
#endif
	for_each_file_restricted(
		dict_dirs_list, ".ifo",
		dict_order_list, dict_disable_list, GetAllDictList(dict_all_list));
}

/* List of plugins that are present on the hard disk in known directories. */
static void get_all_plugin_list(std::list<DictItemId> &plugin_all_list)
{
	std::list<std::string> plugin_order_list;
	std::list<std::string> plugin_disable_list;
	std::list<std::string> plugins_dirs;
	plugins_dirs.push_back(conf_dirs->get_plugin_dir());
	for_each_file(plugins_dirs, "." G_MODULE_SUFFIX, plugin_order_list,
		plugin_disable_list, GetAllPluginList(plugin_all_list));
}

/* Remove all items of the var_list that are not in the all_list */
static void remove_list_items(std::list<DictItemId> &var_list,
	const std::list<DictItemId> &all_list)
{
	for(std::list<DictItemId>::iterator i = var_list.begin(); i != var_list.end();) {
		if(all_list.end() == std::find(all_list.begin(), all_list.end(), *i))
			i = var_list.erase(i);
		else
			++i;
	}
}

/* rename IDs in the var_list to match spelling in the all_list
On Windows DictItemId-s are compared case-insensitivly,
we may modify IDs so they'll be binary equal.
all_list - contains all possible IDs, it prescribe the preferrend spelling of them.
var_list - list of IDs that should be modified according to the all_list.
*/
static void update_list_items(std::list<DictItemId> &var_list,
	const std::list<DictItemId> &all_list)
{
#ifdef _WIN32
	for(std::list<DictItemId>::iterator i = var_list.begin(); i != var_list.end(); ++i) {
		std::list<DictItemId>::const_iterator j = std::find(all_list.begin(), all_list.end(), *i);
		g_assert(j != all_list.end());
		*i = *j;
	}
#endif
}

std::list<DictManageGroup>::iterator DictManageInfo::get_active_group(void)
{
	std::list<DictManageGroup>::iterator i;
	for (i = groups.begin(); i != groups.end(); ++i) {
		if (i->name == active_group)
			break;
	}
	if (i == groups.end()) {
		i = groups.begin();
		active_group = (i != groups.end()) ? i->name : "";
	}
	return i;
}

std::list<DictManageGroup>::iterator
DictManageInfo::set_active_group(const std::string& new_group)
{
	std::list<DictManageGroup>::iterator i;
	for (i = groups.begin(); i != groups.end(); ++i) {
		if (i->name == new_group)
			break;
	}
	if (i == groups.end()) {
		if (groups.empty()) {
			DictManageGroup group;
			group.name = _("Default Group");
			groups.push_back(group);
		}
		i = groups.begin();
	}
	active_group = i->name;
	return i;
}

struct config_ParseUserData {
	DictManageInfo *info;
	bool in_querydict;
	bool in_scandict;
};

static void config_parse_start_element(GMarkupParseContext *context, const gchar *element_name, const gchar **attribute_names, const gchar **attribute_values, gpointer user_data, GError **error)
{
	config_ParseUserData *Data = (config_ParseUserData *)user_data;
	if (strcmp(element_name, "dictgroup")==0) {
		const char *name = NULL;
		size_t i = 0;
		while (attribute_names[i]) {
			if (strcmp(attribute_names[i], "name")==0) {
				name = attribute_values[i];
				break;
			}
			i++;
		}
		if (!name)
			name = _("Default Group");
		DictManageGroup group;
		group.name = name;
		Data->info->groups.push_back(group);
	} else if (strcmp(element_name, "querydict")==0) {
		Data->in_querydict = true;
	} else if (strcmp(element_name, "scandict")==0) {
		Data->in_scandict = true;
	} else if (strcmp(element_name, "localdict")==0) {
		bool enable = true;
		const gchar *file = NULL;
		size_t i = 0;
		while (attribute_names[i]) {
			if (strcmp(attribute_names[i], "enable")==0) {
				if (strcmp(attribute_values[i], "false")==0) {
					enable = false;
				}
			} else if (strcmp(attribute_names[i], "file")==0) {
				file = attribute_values[i];
			}
			i++;
		}
		if (file) {
			DictManageItem item;
			item.type = LOCAL_DICT;
			item.enable = enable;
#ifdef _WIN32
			item.file_or_id = DictItemId(abs_path_to_data_dir(file));
#else
			item.file_or_id = DictItemId(file);
#endif
			if (Data->in_querydict) {
				Data->info->groups.back().querydict.push_back(item);
			} else if (Data->in_scandict) {
				Data->info->groups.back().scandict.push_back(item);
			}
		}
	} else if (strcmp(element_name, "virtualdict")==0) {
		bool enable = true;
		const gchar *id = NULL;
		size_t i = 0;
		while (attribute_names[i]) {
			if (strcmp(attribute_names[i], "enable")==0) {
				if (strcmp(attribute_values[i], "false")==0) {
					enable = false;
				}
			} else if (strcmp(attribute_names[i], "id")==0) {
				id = attribute_values[i];
			}
			i++;
		}
		if (id) {
			DictManageItem item;
			item.type = VIRTUAL_DICT;
			item.enable = enable;
#ifdef _WIN32
			item.file_or_id = DictItemId(abs_path_to_data_dir(id));
#else
			item.file_or_id = DictItemId(id);
#endif
			if (Data->in_querydict) {
				Data->info->groups.back().querydict.push_back(item);
			} else if (Data->in_scandict) {
				Data->info->groups.back().scandict.push_back(item);
			}
		}
	} else if (strcmp(element_name, "netdict")==0) {
		bool enable = true;
		const gchar *id = NULL;
		size_t i = 0;
		while (attribute_names[i]) {
			if (strcmp(attribute_names[i], "enable")==0) {
				if (strcmp(attribute_values[i], "false")==0) {
					enable = false;
				}
			} else if (strcmp(attribute_names[i], "id")==0) {
				id = attribute_values[i];
			}
			i++;
		}
		if (id) {
			DictManageItem item;
			item.type = NET_DICT;
			item.enable = enable;
#ifdef _WIN32
			item.file_or_id = DictItemId(abs_path_to_data_dir(id));
#else
			item.file_or_id = DictItemId(id);
#endif
			if (Data->in_querydict) {
				Data->info->groups.back().querydict.push_back(item);
			} else if (Data->in_scandict) {
				Data->info->groups.back().scandict.push_back(item);
			}
		}
	}
}

static void config_parse_end_element(GMarkupParseContext *context, const gchar *element_name, gpointer user_data, GError **error)
{
	config_ParseUserData *Data = (config_ParseUserData *)user_data;
	if (strcmp(element_name, "querydict")==0) {
		Data->in_querydict = false;
	} else if (strcmp(element_name, "scandict")==0) {
		Data->in_scandict = false;
	}
}

static void DictConfigXmlToInfo(const char *configxml, DictManageInfo &info)
{
	info.groups.clear();
	config_ParseUserData Data;
	Data.info = &info;
	Data.in_querydict = false;
	Data.in_scandict = false;
	GMarkupParser parser;
	parser.start_element = config_parse_start_element;
	parser.end_element = config_parse_end_element;
	parser.text = NULL;
	parser.passthrough = NULL;
	parser.error = NULL;
	GMarkupParseContext* context = g_markup_parse_context_new(&parser, (GMarkupParseFlags)0, &Data, NULL);
	g_markup_parse_context_parse(context, configxml, -1, NULL);
	g_markup_parse_context_end_parse(context, NULL);
	g_markup_parse_context_free(context);
}

static void itemlist_to_xml(std::string &newxml, std::list<DictManageItem> &item_list, bool is_query)
{
	if (is_query)
		newxml += "<querydict>";
	else
		newxml += "<scandict>";
	gchar *estr;
	for (std::list<DictManageItem>::iterator j = item_list.begin(); j != item_list.end(); ++j) {
		if (j->type == LOCAL_DICT)
			newxml += "<localdict";
		else if (j->type == VIRTUAL_DICT)
			newxml += "<virtualdict";
		else
			newxml += "<netdict";
		newxml += " enable=\"";
		if (j->enable)
			newxml += "true";
		else
			newxml += "false";
		if (j->type == LOCAL_DICT)
			newxml  += "\" file=\"";
		else
			newxml += "\" id=\"";
#ifdef _WIN32
		estr = g_markup_escape_text(rel_path_to_data_dir(j->file_or_id.str()).c_str(), -1);
#else
		estr = g_markup_escape_text(j->file_or_id.c_str(), -1);
#endif
		newxml += estr;
		g_free(estr);
		newxml  += "\"/>";
	}
	if (is_query)
		newxml += "</querydict>";
	else
		newxml += "</scandict>";
}

static void InfoToConfigXml(std::string &newxml, DictManageInfo &info)
{
	newxml.clear();
	gchar *estr;
	for (std::list<DictManageGroup>::iterator i = info.groups.begin(); i != info.groups.end(); ++i) {
		newxml += "<dictgroup name=\"";
		estr = g_markup_escape_text(i->name.c_str(), -1);
		newxml += estr;
		g_free(estr);
		newxml += "\">";
		itemlist_to_xml(newxml, i->querydict, true);
		itemlist_to_xml(newxml, i->scandict, false);
		newxml += "</dictgroup>";
	}
}

static void get_dictlist_from_itemlist(std::list<DictItemId> &dict_list,
	const std::list<DictManageItem> &itemlist)
{
	for (std::list<DictManageItem>::const_iterator i = itemlist.begin(); i != itemlist.end(); ++i) {
		if (i->type == LOCAL_DICT && i->enable == true) {
			if (std::find(dict_list.begin(), dict_list.end(), i->file_or_id) == dict_list.end()) {
				dict_list.push_back(i->file_or_id);
			}
		}
	}
}

/* remove unavailable dictionaries and plugins */
static void remove_unavailable_dicts_plugins(std::list<DictManageItem>& dict_list,
	const std::list<DictItemId> &dict_all_list,
	const std::list<DictItemId> &plugin_all_list)
{
	for(std::list<DictManageItem>::iterator i=dict_list.begin();
		i!=dict_list.end(); ) {
		bool found=false;
		switch(i->type) {
		case LOCAL_DICT:
			found = dict_all_list.end()
				!= std::find(dict_all_list.begin(), dict_all_list.end(), i->file_or_id);
			break;
		case VIRTUAL_DICT:
		case NET_DICT:
			found = plugin_all_list.end() != std::find(plugin_all_list.begin(),
				plugin_all_list.end(), i->file_or_id);
			break;
		default:
			g_error("Unknown plugin type %s\n", i->file_or_id.c_str());
			found = false;
		}
		if(found)
			++i;
		else
			i = dict_list.erase(i);
	}
}

/* update IDs of dictionaries and plugins in dict_list, 
so they spell as in dict_all_list and plugin_all_list.
See update_list_items function. */
static void update_dicts_plugins(std::list<DictManageItem>& dict_list,
	const std::list<DictItemId> &dict_all_list,
	const std::list<DictItemId> &plugin_all_list)
{
#ifdef _WIN32
	std::list<DictItemId>::const_iterator j;
	for(std::list<DictManageItem>::iterator i=dict_list.begin();
		i!=dict_list.end(); ++i) {
		switch(i->type) {
		case LOCAL_DICT:
			j = std::find(dict_all_list.begin(), dict_all_list.end(), i->file_or_id);
			g_assert(j != dict_all_list.end());
			break;
		case VIRTUAL_DICT:
		case NET_DICT:
			j = std::find(plugin_all_list.begin(), plugin_all_list.end(), i->file_or_id);
			g_assert(j != plugin_all_list.end());
			break;
		default:
			g_error("Unknown plugin type %s\n", i->file_or_id.c_str());
		}
		i->file_or_id = *j;
	}
#endif
}

void UpdateConfigXML(
	const std::list<DictItemId> &dict_new_install_list,
	const std::list<DictItemId> &plugin_new_install_list,
	const StarDictPlugins* oStarDictPlugins)
{
	std::list<DictItemId> dict_all_list;
	std::list<DictItemId> plugin_all_list;
	{
#ifdef _WIN32
		std::list<std::string> s_dict_all_list;
		std::list<std::string> s_plugin_all_list;
		{
			const std::list<std::string> &dict_all_list_rel
				= conf->get_strlist("/apps/stardict/manage_dictionaries/dict_order_list");
			const std::list<std::string> &plugin_all_list_rel
				= conf->get_strlist("/apps/stardict/manage_plugins/plugin_order_list");
			abs_path_to_data_dir(dict_all_list_rel, s_dict_all_list);
			abs_path_to_data_dir(plugin_all_list_rel, s_plugin_all_list);
		}
#else
		const std::list<std::string> &s_dict_all_list
			= conf->get_strlist("/apps/stardict/manage_dictionaries/dict_order_list");
		const std::list<std::string> &s_plugin_all_list
			= conf->get_strlist("/apps/stardict/manage_plugins/plugin_order_list");
#endif
		DictItemId::convert(dict_all_list, s_dict_all_list);
		DictItemId::convert(plugin_all_list, s_plugin_all_list);
	}
	const std::string &configxml
		= conf->get_string("/apps/stardict/manage_dictionaries/dict_config_xml");
	DictManageInfo dictinfo;
	DictConfigXmlToInfo(configxml.c_str(), dictinfo);

	// remove dictionaries and plugins that are not available now
	for(std::list<DictManageGroup>::iterator igroup=dictinfo.groups.begin();
		igroup!=dictinfo.groups.end(); ++igroup) {
		remove_unavailable_dicts_plugins(igroup->querydict, dict_all_list, plugin_all_list);
		remove_unavailable_dicts_plugins(igroup->scandict, dict_all_list, plugin_all_list);
		update_dicts_plugins(igroup->querydict, dict_all_list, plugin_all_list);
		update_dicts_plugins(igroup->scandict, dict_all_list, plugin_all_list);
	}
	
	std::list<DictManageGroup>::iterator iactivegroup
		= dictinfo.set_active_group(
			conf->get_string("/apps/stardict/manage_dictionaries/dict_default_group"));
	
	//if(conf->get_bool("/apps/stardict/preferences/dictionary/add_new_dict_in_active_group")) {
		// add new dictionaries to the active group
		for (std::list<DictItemId>::const_iterator j = dict_new_install_list.begin(); j != dict_new_install_list.end(); ++j) {
			DictManageItem item;
			item.type = LOCAL_DICT;
			item.enable = true;
			item.file_or_id = *j;
			iactivegroup->querydict.push_back(item);
			iactivegroup->scandict.push_back(item);
		}
	//}
	//if(conf->get_bool("/apps/stardict/preferences/dictionary/add_new_plugin_in_active_group")) {
		// add new plugins to the active group
		for(std::list<DictItemId>::const_iterator j=plugin_new_install_list.begin(); j != plugin_new_install_list.end(); ++j) {
			size_t iPlugin;
			if(oStarDictPlugins->VirtualDictPlugins.find_dict_by_id(*j, iPlugin)) {
				DictManageItem item;
				item.type = VIRTUAL_DICT;
				item.enable = true;
				item.file_or_id = *j;
				iactivegroup->querydict.push_back(item);
				iactivegroup->scandict.push_back(item);
			} else if(oStarDictPlugins->NetDictPlugins.find_dict_by_id(*j, iPlugin)) {
				DictManageItem item;
				item.type = NET_DICT;
				item.enable = true;
				item.file_or_id = *j;
				iactivegroup->querydict.push_back(item);
				iactivegroup->scandict.push_back(item);
			}
		}
	//}
	std::string newxml;
	InfoToConfigXml(newxml, dictinfo);
	conf->set_string("/apps/stardict/manage_dictionaries/dict_config_xml", newxml);
}

/* get a list of all used dictionaries, those that are part of some dictionary group,
 * not only the active dictionary group */
void GetUsedDictList(std::list<DictItemId> &dict_list)
{
	dict_list.clear();
	for (std::list<DictManageGroup>::iterator i = gpAppFrame->dictinfo.groups.begin(); i != gpAppFrame->dictinfo.groups.end(); ++i) {
		get_dictlist_from_itemlist(dict_list, i->querydict);
		get_dictlist_from_itemlist(dict_list, i->scandict);
	}
}

static void set_dictmask_by_itemlist(const std::list<DictManageItem> &itemlist, bool is_query)
{
	if (is_query)
		gpAppFrame->query_dictmask.clear();
	else
		gpAppFrame->scan_dictmask.clear();
	for (std::list<DictManageItem>::const_iterator i = itemlist.begin(); i != itemlist.end(); ++i) {
		if (i->enable) {
			if (i->type == LOCAL_DICT) {
				size_t iLib;
				if (gpAppFrame->oLibs.find_lib_by_id(i->file_or_id, iLib)) {
					InstantDictIndex instance_dict_index;
					instance_dict_index.type = InstantDictType_LOCAL;
					instance_dict_index.index = iLib;
					if (is_query)
						gpAppFrame->query_dictmask.push_back(instance_dict_index);
					else
						gpAppFrame->scan_dictmask.push_back(instance_dict_index);
				}
			} else if (i->type == VIRTUAL_DICT) {
				size_t iPlugin;
				if (gpAppFrame->oStarDictPlugins->VirtualDictPlugins.find_dict_by_id(i->file_or_id, iPlugin)) {
					InstantDictIndex instance_dict_index;
					instance_dict_index.type = InstantDictType_VIRTUAL;
					instance_dict_index.index = iPlugin;
					if (is_query)
						gpAppFrame->query_dictmask.push_back(instance_dict_index);
					else
						gpAppFrame->scan_dictmask.push_back(instance_dict_index);
				}
			} else {
				g_assert(i->type == NET_DICT);
				size_t iPlugin;
				if (gpAppFrame->oStarDictPlugins->NetDictPlugins.find_dict_by_id(i->file_or_id, iPlugin)) {
					InstantDictIndex instance_dict_index;
					instance_dict_index.type = InstantDictType_NET;
					instance_dict_index.index = iPlugin;
					if (is_query)
						gpAppFrame->query_dictmask.push_back(instance_dict_index);
					else
						gpAppFrame->scan_dictmask.push_back(instance_dict_index);
				}
			}
		}
	}
}

void UpdateDictMask()
{
	std::list<DictManageGroup>::iterator i = gpAppFrame->dictinfo.get_active_group();
	set_dictmask_by_itemlist(i->querydict, true);
	set_dictmask_by_itemlist(i->scandict, false);
	g_free(gpAppFrame->iCurrentIndex);
	gpAppFrame->iCurrentIndex = (CurrentIndex*)g_malloc0(sizeof(CurrentIndex) * gpAppFrame->query_dictmask.size());
	conf->set_string("/apps/stardict/manage_dictionaries/dict_default_group", gpAppFrame->dictinfo.active_group);
}

void LoadDictInfo()
{
	const std::string &configxml
		= conf->get_string("/apps/stardict/manage_dictionaries/dict_config_xml");
	DictConfigXmlToInfo(configxml.c_str(), gpAppFrame->dictinfo);
	gpAppFrame->dictinfo.set_active_group(
		conf->get_string("/apps/stardict/manage_dictionaries/dict_default_group"));
}

/* update manage_dictionaries/dict_order_list configuration option
 * - add new dictionaries
 * - remove dictionaries that are no longer available
 * */
void UpdateDictList(std::list<DictItemId> &dict_new_install_list, show_progress_t *sp, bool verify_dict)
{
	dict_new_install_list.clear();
	std::list<DictItemId> dict_all_list;
	get_all_dict_list(dict_all_list);
	// verify dictionaries, remove invalid dictionaries from dict_all_list
	if (verify_dict) {
		std::list<std::string> s_dict_valid_list;
		std::list<std::string> s_dict_all_list;
		DictItemId::convert(s_dict_all_list, dict_all_list);
		filter_verify_dicts(s_dict_all_list, s_dict_valid_list, sp);
		DictItemId::convert(dict_all_list, s_dict_valid_list);
	}
	std::list<DictItemId> dict_order_list;
	{
#ifdef _WIN32
		std::list<std::string> s_dict_order_list;
		{
			const std::list<std::string>& dict_order_list_rel
				= conf->get_strlist("/apps/stardict/manage_dictionaries/dict_order_list");
			abs_path_to_data_dir(dict_order_list_rel, s_dict_order_list);
		}
#else
		std::list<std::string> s_dict_order_list(conf->get_strlist("/apps/stardict/manage_dictionaries/dict_order_list"));
#endif
		DictItemId::convert(dict_order_list, s_dict_order_list);
	}
	// remove dictionaries that are not available
	remove_list_items(dict_order_list, dict_all_list);
	update_list_items(dict_order_list, dict_all_list);
	// find new dictionaries
	for (std::list<DictItemId>::iterator i = dict_all_list.begin(); i != dict_all_list.end(); ++i) {
		if (dict_order_list.end() == std::find(dict_order_list.begin(), dict_order_list.end(), *i)) {
			dict_new_install_list.push_back(*i);
		}
	}
	dict_order_list.insert(dict_order_list.end(), dict_new_install_list.begin(), dict_new_install_list.end());
	{
		std::list<std::string> s_dict_order_list;
		DictItemId::convert(s_dict_order_list, dict_order_list);
#ifdef _WIN32
		{
			std::list<std::string> dict_order_list_rel;
			rel_path_to_data_dir(s_dict_order_list, dict_order_list_rel);
			conf->set_strlist("/apps/stardict/manage_dictionaries/dict_order_list", dict_order_list_rel);
		}
#else
		conf->set_strlist("/apps/stardict/manage_dictionaries/dict_order_list", s_dict_order_list);
#endif
	}
}

/* update manage_plugins/plugin_order_list configuration list
 * - add new plug-ins,
 * - remove plug-ins that are no longer available
 * update manage_plugins/plugin_disable_list configuration list
 * - remove plug-ins that are no longer available
 * */
void UpdatePluginList(std::list<DictItemId> &plugin_new_install_list)
{
	plugin_new_install_list.clear();
	std::list<DictItemId> plugin_all_list;
	std::list<DictItemId> plugin_order_list;

	get_all_plugin_list(plugin_all_list);
	{
		std::list<std::string> s_plugin_order_list;
#ifdef _WIN32
		{
			const std::list<std::string>& plugin_order_list_rel
				= conf->get_strlist("/apps/stardict/manage_plugins/plugin_order_list");
			abs_path_to_data_dir(plugin_order_list_rel, s_plugin_order_list);
		}
#else
		s_plugin_order_list = conf->get_strlist("/apps/stardict/manage_plugins/plugin_order_list");
#endif
		DictItemId::convert(plugin_order_list, s_plugin_order_list);
	}
	// remove plugins that are not available
	remove_list_items(plugin_order_list, plugin_all_list);
	update_list_items(plugin_order_list, plugin_all_list);
	// find new plugins
	for(std::list<DictItemId>::iterator it = plugin_all_list.begin(); it != plugin_all_list.end(); ++it) {
		if(plugin_order_list.end() == std::find(plugin_order_list.begin(), plugin_order_list.end(), *it))
			plugin_new_install_list.push_back(*it);
	}
	plugin_order_list.insert(plugin_order_list.end(), plugin_new_install_list.begin(), plugin_new_install_list.end());
	{
		std::list<std::string> s_plugin_order_list;
		DictItemId::convert(s_plugin_order_list, plugin_order_list);
#ifdef _WIN32
		{
			std::list<std::string> plugin_order_list_rel;
			rel_path_to_data_dir(s_plugin_order_list, plugin_order_list_rel);
			conf->set_strlist("/apps/stardict/manage_plugins/plugin_order_list", plugin_order_list_rel);
		}
#else
		conf->set_strlist("/apps/stardict/manage_plugins/plugin_order_list", s_plugin_order_list);
#endif
	}

	std::list<DictItemId> plugin_disable_list;
	{
#ifdef _WIN32
		std::list<std::string> s_plugin_disable_list;
		{
			const std::list<std::string> &plugin_disable_list_rel
				= conf->get_strlist("/apps/stardict/manage_plugins/plugin_disable_list");
			abs_path_to_data_dir(plugin_disable_list_rel, s_plugin_disable_list);
		}
#else
		std::list<std::string> s_plugin_disable_list(
			conf->get_strlist("/apps/stardict/manage_plugins/plugin_disable_list"));
#endif
		DictItemId::convert(plugin_disable_list, s_plugin_disable_list);
	}
	remove_list_items(plugin_disable_list, plugin_all_list);
	update_list_items(plugin_disable_list, plugin_all_list);
	{
		std::list<std::string> s_plugin_disable_list;
		DictItemId::convert(s_plugin_disable_list, plugin_disable_list);
#ifdef _WIN32
		{
			std::list<std::string> plugin_disable_list_rel;
			rel_path_to_data_dir(s_plugin_disable_list, plugin_disable_list_rel);
			conf->set_strlist("/apps/stardict/manage_plugins/plugin_disable_list",
				plugin_disable_list_rel);
		}
#else
		conf->set_strlist("/apps/stardict/manage_plugins/plugin_disable_list",
			s_plugin_disable_list);
#endif
	}
}

void RemoveCacheFiles(void)
{
	/* We may not simply remove all ".oft" and ".clt" files in all known
	 * directories, there are resource storage directories! */
#ifdef _WIN32
	std::list<std::string> dict_list;
	{
		const std::list<std::string>& dict_list_rel = conf->get_strlist("/apps/stardict/manage_dictionaries/dict_order_list");
		abs_path_to_data_dir(dict_list_rel, dict_list);
	}
#else
	const std::list<std::string>& dict_list = conf->get_strlist("/apps/stardict/manage_dictionaries/dict_order_list");
#endif
	std::list<std::string> dir_list;
	glib::CharStr gdir;
	/* Collect a list of directories where to remove cache files. */
	for(std::list<std::string>::const_iterator it = dict_list.begin(); it != dict_list.end(); ++it) {
		gdir.reset(g_path_get_dirname(it->c_str()));
		if(dir_list.end() == std::find_if(dir_list.begin(), dir_list.end(), PathComparePred(get_impl(gdir))))
			dir_list.push_back(get_impl(gdir));
	}
	dir_list.push_back(conf_dirs->get_user_cache_dir());
	// remove cache files
	const gchar *filename;
	GDir *dir;
	for(std::list<std::string>::const_iterator it = dir_list.begin(); it != dir_list.end(); ++it) {
		dir = g_dir_open(it->c_str(), 0, NULL);
		if(!dir)
			continue;
		while ((filename = g_dir_read_name(dir))!=NULL) {
			if(!is_path_end_with(filename, ".oft") && !is_path_end_with(filename, ".clt"))
				continue;
			std::string fullfilename(build_path(*it, filename));
			if (!g_file_test(fullfilename.c_str(), G_FILE_TEST_IS_DIR)) {
				if(g_unlink(fullfilename.c_str()))
					g_warning(_("Unable to remove file: %s"), fullfilename.c_str());
			}
		}
		g_dir_close(dir);
	}
}
