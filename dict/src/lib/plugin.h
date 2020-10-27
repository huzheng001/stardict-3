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

#ifndef _STARDICT_PLUGIN_H_
#define _STARDICT_PLUGIN_H_

#include <gtk/gtk.h>
#include <string>
#include <list>


#define PLUGIN_SYSTEM_VERSION "3.0.7"

enum StarDictPlugInType {
	StarDictPlugInType_UNKNOWN,
	StarDictPlugInType_VIRTUALDICT,
	StarDictPlugInType_NETDICT,
	StarDictPlugInType_SPECIALDICT,
	StarDictPlugInType_TTS,
	StarDictPlugInType_PARSEDATA,
	StarDictPlugInType_MISC,
};

struct NetDictResponse;

struct StarDictPluginSystemService {
	typedef void (*get_http_response_func_t)(const char *buffer, size_t buffer_len, gpointer userdata);
	typedef void (*send_http_request_func_t)(const char* shost, const char* sfile, get_http_response_func_t callback_func, gpointer userdata);
	send_http_request_func_t send_http_request;
	typedef void (*show_url_func_t)(const char *url);
	show_url_func_t show_url;
	typedef void (*set_news_func_t)(const char *news, const char *links);
	set_news_func_t set_news;
	typedef char *(*encode_uri_string_func_t)(const char *string);
	encode_uri_string_func_t encode_uri_string;
	typedef char *(*build_dictdata_func_t)(char type, const char *definition);
	build_dictdata_func_t build_dictdata;
	typedef void (*netdict_save_cache_resp_func_t)(const char *dict, const char *key, NetDictResponse *resp);
	netdict_save_cache_resp_func_t netdict_save_cache_resp;
	typedef void (*show_netdict_resp_func_t)(const char *dict, NetDictResponse *resp, bool ismainwin);
	show_netdict_resp_func_t show_netdict_resp;
	typedef void (*lookup_dict_func_t)(size_t dictid, const char *word, char ****Word, char *****WordData);
	lookup_dict_func_t lookup_dict;
	typedef void (*FreeResultData_func_t)(size_t dictmask_size, char ***pppWord, char ****ppppWordData);
	FreeResultData_func_t FreeResultData;
	typedef void (*ShowPangoTips_func_t)(const char *word, const char *text);
	ShowPangoTips_func_t ShowPangoTips;
};

struct StarDictPluginSystemInfo {
	std::string datadir;
	GtkWidget *mainwin;
	GtkWidget *pluginwin;
};

// Notice: You need to init these structs' members before creating a StarDictPlugins object.
extern StarDictPluginSystemInfo oStarDictPluginSystemInfo;
extern StarDictPluginSystemService oStarDictPluginSystemService;

typedef void (*plugin_configure_func_t)();

struct StarDictPlugInObject {
	StarDictPlugInObject();
	~StarDictPlugInObject();

	const char* version_str;
	StarDictPlugInType type;
	char* info_xml;
	plugin_configure_func_t configure_func;

	const StarDictPluginSystemInfo *plugin_info;
	const StarDictPluginSystemService *plugin_service;
};

#endif
