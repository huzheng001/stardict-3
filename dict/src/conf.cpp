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
#  include "config.h"
#endif

#include <glib.h>
#include <glib/gstdio.h>
#include <glib/gi18n.h>
#include <cstring>
#include <cstdlib>

#include "class_factory.h"
#include "lib/utils.h"
#include "inifile.h"
#include "libcommon.h"

#include "conf.h"

#ifdef CONFIG_GPE
const int DEFAULT_WINDOW_WIDTH=238;
const int DEFAULT_WINDOW_HEIGHT=279;
const int DEFAULT_HPANED_POS=79;
#else
const int DEFAULT_WINDOW_WIDTH=600;
const int DEFAULT_WINDOW_HEIGHT=390;
const int DEFAULT_HPANED_POS=120;
#endif

std::unique_ptr<AppConf> conf;
std::unique_ptr<AppDirs> conf_dirs;

#ifdef _WIN32
HINSTANCE stardictexe_hInstance;

/* Get full path to the directory containing StarDict executable file 
	- application directory. */
std::string get_application_dir(void)
{
	TCHAR path_win[MAX_PATH];
	DWORD dwRes = GetModuleFileName(NULL, path_win, MAX_PATH);
	if(dwRes == 0 || dwRes == MAX_PATH)
		exit(EXIT_FAILURE);
	std::string path_utf8;
	if(windows_to_utf8(path_win, path_utf8)) {
		glib::CharStr buf(g_path_get_dirname(path_utf8.c_str()));
		return get_impl(buf);
	} else
		exit(EXIT_FAILURE);
}

/* transform a path relative to the application dir to an absolute path.
Do nothing if the path is already an absolute path.
Return value: EXIT_FAILURE or EXIT_SUCCESS. */
int abs_path_to_app_dir(const std::string& path, std::string& abs_path)
{
	abs_path.clear();
	std::string path_norm;
	if(norm_path_win(path, path_norm))
		return EXIT_FAILURE;
	if(!is_valid_path_win(path_norm))
		return EXIT_FAILURE;
	if(is_absolute_path_win(path_norm)) {
		abs_path = path_norm;
		return EXIT_SUCCESS;
	}
	std::string path_abs(build_path(get_application_dir(), path_norm));
	std::string path_abs_norm;
	if(norm_path_win(path_abs, path_abs_norm))
		return EXIT_FAILURE;
	abs_path = path_abs_norm;
	return EXIT_SUCCESS;
}

#endif // #ifdef _WIN32

//---------------------------------------------------------------------------------
AppConf::AppConf() :
	cf(static_cast<config_file *>(PlatformFactory::create_class_by_name("config_file")))
{
	add_entry("/apps/stardict/preferences/main_window/maximized", false);
#ifdef _WIN32
	add_entry("/apps/stardict/preferences/dictionary/use_custom_font", get_win32_use_custom_font());
#else
#ifdef CONFIG_DARWIN
	add_entry("/apps/stardict/preferences/dictionary/use_custom_font", get_darwin_use_custom_font());
#else
	add_entry("/apps/stardict/preferences/dictionary/use_custom_font", false);
#endif
#endif
#if defined(_WIN32) || defined(CONFIG_DARWIN)
	add_entry("/apps/stardict/preferences/network/enable_netdict", true);
#else
	add_entry("/apps/stardict/preferences/network/enable_netdict", false);
#endif
	// Default stardictd server.
	add_entry("/apps/stardict/preferences/network/server", std::string(_("dict2.stardict.net")));
	add_entry("/apps/stardict/preferences/network/port", 2629);
	add_entry("/apps/stardict/preferences/network/user", std::string());
	add_entry("/apps/stardict/preferences/network/md5saltpasswd", std::string());
	// may store relative path
	add_entry("/apps/stardict/preferences/main_window/skin", std::string());
	add_entry("/apps/stardict/preferences/main_window/hide_on_startup", false);
	add_entry("/apps/stardict/preferences/main_window/keep_above", false);
	add_entry("/apps/stardict/preferences/main_window/search_while_typing", true);
	add_entry("/apps/stardict/preferences/main_window/word_change_timeout", 300);
	add_entry("/apps/stardict/preferences/main_window/showfirst_when_notfound", true);
	add_entry("/apps/stardict/preferences/translate/engine", 0);
	add_entry("/apps/stardict/preferences/translate/fromlang", 0);
	add_entry("/apps/stardict/preferences/translate/tolang", 0);
	add_entry("/apps/stardict/preferences/dictionary/enable_sound_event", true);
	add_entry("/apps/stardict/preferences/dictionary/use_tts_program", false);
	add_entry("/apps/stardict/preferences/dictionary/tts_program_cmdline", std::string()); // absolute command
	add_entry("/apps/stardict/preferences/main_window/hide_list", false);
	add_entry("/apps/stardict/preferences/dictionary/scan_selection", true);
	add_entry("/apps/stardict/preferences/dictionary/bookname_style", 0);
	add_entry("/apps/stardict/preferences/dictionary/markup_search_word", false);
#ifdef _WIN32
	add_entry("/apps/stardict/preferences/dictionary/scan_clipboard", false);
#endif
#ifndef CONFIG_DARWIN
	add_entry("/apps/stardict/preferences/dictionary/use_scan_hotkey", true);
	add_entry("/apps/stardict/preferences/dictionary/use_mainwindow_hotkey", true);
	add_entry("/apps/stardict/preferences/dictionary/scan_hotkey", std::string("<Ctrl><Alt>x"));
 	add_entry("/apps/stardict/preferences/dictionary/mainwindow_hotkey", std::string("<Ctrl><Alt>z"));
#endif
	add_entry("/apps/stardict/preferences/notification_area_icon/middle_click_action", int(namclaQueryFloatWindow));
	add_entry("/apps/stardict/preferences/dictionary/only_scan_while_modifier_key", false);
	add_entry("/apps/stardict/preferences/dictionary/hide_floatwin_when_modifier_key_released", true);
	add_entry("/apps/stardict/preferences/floating_window/pronounce_when_popup", false);
	add_entry("/apps/stardict/preferences/floating_window/lock", false);
	add_entry("/apps/stardict/preferences/floating_window/show_if_not_found", true);
	add_entry("/apps/stardict/preferences/floating_window/use_custom_bg", false);
#if GTK_MAJOR_VERSION >= 3
	add_entry("/apps/stardict/preferences/floating_window/bg_red", 1.0);
	add_entry("/apps/stardict/preferences/floating_window/bg_green", 1.0);
	add_entry("/apps/stardict/preferences/floating_window/bg_blue", (51200/(double)65535));
#else
	add_entry("/apps/stardict/preferences/floating_window/bg_red", 65535);
	add_entry("/apps/stardict/preferences/floating_window/bg_green", 65535);
	add_entry("/apps/stardict/preferences/floating_window/bg_blue", 51200);
#endif
	add_entry("/apps/stardict/preferences/floating_window/transparent", 0);

	add_entry("/apps/stardict/preferences/floating_window/lock_x", 0);
	add_entry("/apps/stardict/preferences/floating_window/lock_y", 0);
	add_entry("/apps/stardict/preferences/dictionary/scan_modifier_key", 0);
	add_entry("/apps/stardict/preferences/main_window/transparent", 0);
	add_entry("/apps/stardict/preferences/main_window/hpaned_pos", DEFAULT_HPANED_POS);
	add_entry("/apps/stardict/preferences/main_window/window_width", DEFAULT_WINDOW_WIDTH);
	add_entry("/apps/stardict/preferences/main_window/window_height", DEFAULT_WINDOW_HEIGHT);
	add_entry("/apps/stardict/preferences/floating_window/max_window_width", DEFAULT_MAX_FLOATWIN_WIDTH);
	add_entry("/apps/stardict/preferences/floating_window/max_window_height", DEFAULT_MAX_FLOATWIN_HEIGHT);

#ifdef _WIN32
	add_entry("/apps/stardict/preferences/dictionary/custom_font", get_win32_custom_font());
#else
#ifdef CONFIG_DARWIN
	add_entry("/apps/stardict/preferences/dictionary/custom_font", get_darwin_custom_font());
#else
	add_entry("/apps/stardict/preferences/dictionary/custom_font", std::string());
#endif
#endif

	add_entry("/apps/stardict/preferences/dictionary/create_cache_file", true);
	add_entry("/apps/stardict/preferences/dictionary/enable_collation", false);
	add_entry("/apps/stardict/preferences/dictionary/collate_function", 0);
	add_entry("/apps/stardict/preferences/dictionary/do_not_load_bad_dict", true);
	//add_entry("/apps/stardict/preferences/dictionary/add_new_dict_in_active_group", true);
	//add_entry("/apps/stardict/preferences/dictionary/add_new_plugin_in_active_group", true);

	add_entry("/apps/stardict/preferences/dictionary/always_use_sound_play_command", false);
#if defined(_WIN32)
	add_entry("/apps/stardict/preferences/dictionary/sound_play_command", std::string("play")); // absolute command
#else
	add_entry("/apps/stardict/preferences/dictionary/sound_play_command", std::string("aplay")); // absolute command
#endif
	add_entry("/apps/stardict/preferences/dictionary/video_play_command", std::string("play")); // absolute command
#if defined(CONFIG_GPE)
	add_entry("/apps/stardict/preferences/dictionary/url_open_command", std::string("gpe-mini-browser"));
#else
	add_entry("/apps/stardict/preferences/dictionary/url_open_command", std::string("firefox")); // absolute command
#endif
#if defined(_WIN32) || defined(CONFIG_GNOME)
	add_entry("/apps/stardict/preferences/dictionary/always_use_open_url_command", false);
#endif
	{
		std::list<std::string> pathlist;
#ifdef _WIN32
		pathlist.push_back("C:\\Program Files\\WyabdcRealPeopleTTS");
		pathlist.push_back("C:\\Program Files\\OtdRealPeopleTTS");
		pathlist.push_back("WyabdcRealPeopleTTS");
		pathlist.push_back("OtdRealPeopleTTS");
		// stores absolute and relative paths
		add_entry("/apps/stardict/preferences/dictionary/tts_path", pathlist);
#else
		pathlist.push_back("/usr/share/WyabdcRealPeopleTTS");
		pathlist.push_back("/usr/share/OtdRealPeopleTTS");
		add_entry("/apps/stardict/preferences/dictionary/tts_path", pathlist);
#endif
	}
	// may store relative path
	add_entry("/apps/stardict/preferences/dictionary/history", get_default_history_filename());
	add_entry("/apps/stardict/preferences/dictionary/only_export_word", true);
	// may store relative path
	add_entry("/apps/stardict/preferences/dictionary/export_file", get_default_export_filename());

	add_entry("/apps/stardict/preferences/main_window/search_website_list", std::list<std::string>());
	// stores absolute and relative paths
	add_entry("/apps/stardict/manage_dictionaries/treedict_order_list", std::list<std::string>());
	// stores absolute and relative paths
	add_entry("/apps/stardict/manage_dictionaries/treedict_disable_list", std::list<std::string>());
	add_entry("/apps/stardict/manage_dictionaries/dict_order_list", std::list<std::string>());
	// stores absolute and relative paths
	add_entry("/apps/stardict/manage_dictionaries/dict_config_xml", std::string());
	add_entry("/apps/stardict/manage_dictionaries/dict_default_group", std::string());

	// stores absolute and relative paths
	add_entry("/apps/stardict/manage_plugins/plugin_order_list", std::list<std::string>());
	// stores absolute and relative paths
	add_entry("/apps/stardict/manage_plugins/plugin_disable_list", std::list<std::string>());

	std::list<std::string> dirs;
	{
		std::string dir(build_path(conf_dirs->get_data_dir(), "dic"));
#ifdef _WIN32
		dirs.push_back(rel_path_to_data_dir(dir));
#else
		dirs.push_back(dir);
#endif
	}
#ifndef _WIN32
	if (conf_dirs->get_data_dir() != "/usr/share/stardict") {
		dirs.push_back("/usr/share/stardict/dic");
	}
	dirs.push_back(std::string(g_get_home_dir())+"/.stardict/dic");
#endif
	// stores absolute and relative paths
	add_entry("/apps/stardict/manage_dictionaries/dict_dirs_list", dirs);

	dirs.clear();
	{
		std::string dir(build_path(conf_dirs->get_data_dir(), "treedict"));
#ifdef _WIN32
		dirs.push_back(abs_path_to_data_dir(dir));
#else
		dirs.push_back(dir);
#endif
	}
#ifndef _WIN32
	dirs.push_back(std::string(g_get_home_dir())+"/.stardict/treedict");
#endif
	// stores absolute and relative paths
	add_entry("/apps/stardict/manage_dictionaries/treedict_dirs_list", dirs);

	Load();
}
//---------------------------------------------------------------------------------
AppConf::~AppConf()
{
	for (cache_t::iterator it=cache.begin(); it!=cache.end(); ++it)
		delete it->second;
}
//---------------------------------------------------------------------------------
static std::pair<std::string, std::string> split(const std::string& s)
{
	std::string::size_type pos=s.rfind("/");
	std::pair<std::string, std::string> res;
	if (pos!=std::string::npos)
		res.second = s.substr(pos+1);
	else
		pos=s.length();

	res.first=s.substr(0, pos);

	return res;
}
//---------------------------------------------------------------------------------

//---------------------------------------------------------------------------------
void AppConf::notify_add(const char *name, const sigc::slot<void, const baseconfval*>& slot)
{
	std::pair<std::string, std::string> split_name = split(name);
	cf->notify_add(split_name.first.c_str(), split_name.second.c_str(), slot);
}
//---------------------------------------------------------------------------------
//load preference
void AppConf::Load()
{
	for (cache_t::iterator p=cache.begin(); p!=cache.end(); ++p) {
		std::pair<std::string, std::string> name = split(p->first);
		p->second->load(*cf, name.first.c_str(), name.second.c_str());
	}
	const std::list<std::string> &list=
		get_strlist("/apps/stardict/preferences/main_window/search_website_list");
	if (list.empty()) {
		cache_t::iterator p =
			cache.find("/apps/stardict/preferences/main_window/search_website_list");
		static_cast<confval< std::list<std::string> > *>(p->second)->val_ =
			get_default_search_website_list();
	}
}
//---------------------------------------------------------------------------------
std::string AppConf::get_default_history_filename()
{
	std::string histname;
#ifdef _WIN32
	histname = build_path(conf_dirs->get_user_config_dir(), "history.txt");
	histname = rel_path_to_data_dir(histname);
#else
	histname = build_path(conf_dirs->get_user_config_dir(), "history");
#endif

	return histname;
}

std::string AppConf::get_default_export_filename()
{
	std::string exportname;
#ifdef _WIN32
	exportname = build_path(conf_dirs->get_data_dir(), "dic.txt");
	exportname = rel_path_to_data_dir(exportname);
#else
	exportname = g_get_home_dir();
	exportname+= G_DIR_SEPARATOR_S "dic.txt";
#endif
	return exportname;
}

std::list<std::string> AppConf::get_default_search_website_list()
{
	/* xgettext: no-c-format */
	gchar *default_website = _("StarDict.net	http://www.stardict.net	http://www.stardict.net/query.php?q=%s\n"
		"Dictionary.com	http://dictionary.reference.com	http://dictionary.reference.com/search?q=%s\n"
		"dict.leo.org	http://dict.leo.org	http://dict.leo.org/?search=%s&lang=en\n"
		"H2G2	http://www.h2g2.com	http://www.h2g2.com/Search?searchstring=%s&searchtype=ARTICLE&skip=0&show=20&showapproved=1&shownormal=1&showsubmitted=1\n"
		"WhatIs	http://whatis.techtarget.com	http://whatis.techtarget.com/wsearchResults/1,290214,sid9,00.html?query=%s\n"
		"Altavista	http://www.altavista.com	http://www.altavista.com/cgi-bin/query?q=%s\n"
		"WEB.DE	http://suche.web.de	http://suche.web.de/search/?su=%s\n"
		"WebCrawler	http://www.webcrawler.com	http://www.webcrawler.com/cgi-bin/WebQuery?searchText=%s\n"
		"Google	http://www.google.com	http://www.google.com/search?q=%s\n"
		"Yahoo	http://search.yahoo.com	http://search.yahoo.com/bin/search?p=%s\n"
		"CMU	http://www.speech.cs.cmu.edu	http://www.speech.cs.cmu.edu/cgi-bin/cmudict?in=%s\n"
		);
//TODO: use split instead?
	gchar *p = default_website;
	gchar *p1;
	std::list<std::string> list;
	while ((p1 = strchr(p, '\n'))!= NULL) {
		list.push_back(std::string(p, p1-p));
		p= p1+1;
	}
	return list;
}

#ifdef _WIN32
bool AppConf::get_win32_use_custom_font()
{
	// You may translate it to "win32_use_custom_font=1" for your language.
	gchar *ch = _("win32_use_custom_font=0");
	gchar *s = strstr(ch, "win32_use_custom_font=");
	if (s) {
		if (*(s+ sizeof("win32_use_custom_font=")-1)=='1')
			return true;
	}
	return false;
}

std::string AppConf::get_win32_custom_font()
{
	// You may translate it as "win32_custom_font=tahoma 9".
	gchar *ch = _("win32_custom_font=");
	gchar *s = strstr(ch, "win32_custom_font=");
	if (s) {
		return (s+ sizeof("win32_custom_font=")-1);
	}
	return "";
}
#endif

#ifdef CONFIG_DARWIN
bool AppConf::get_darwin_use_custom_font()
{
	// You may translate it to "darwin_use_custom_font=1" for your language.
	gchar *ch = _("darwin_use_custom_font=0");
	gchar *s = strstr(ch, "darwin_use_custom_font=");
	if (s) {
		if (*(s+ sizeof("darwin_use_custom_font=")-1)=='1')
			return true;
	}
	return false;
}

std::string AppConf::get_darwin_custom_font()
{
	// You may translate it as "darwin_custom_font=STSong 12".
	gchar *ch = _("darwin_custom_font=");
	gchar *s = strstr(ch, "darwin_custom_font=");
	if (s) {
		return (s+ sizeof("darwin_custom_font=")-1);
	}
	return "";
}
#endif
//---------------------------------------------------------------------------------

/* Wrapper of stardict-dirs.cfg configuration file.
 * Provides access to configuration parameters.
 * If a parameter is not defined, an empty string is returned.
 *  
 * Do not use conf_dirs in this class! conf_dirs creation is in progress
 * when an instance of this class is created. */
class AppDirsConf
{
public:
	AppDirsConf(void)
	: loaded(false)
	{
	}
	
	void load(const std::string& conf_file)
	{
		if(g_file_test(conf_file.c_str(), GFileTest(G_FILE_TEST_IS_REGULAR))) {
			g_debug("Loading StarDict dirs config: %s", conf_file.c_str());
			if(!ini.load(conf_file, true, false))
				exit(EXIT_FAILURE);
			loaded = true;
		}
	}

	std::string get_string_at(const char* key)
	{
		if(!loaded)
			return "";
		std::string val;
		if(ini.read_string("general", key, val))
			return val;
		else
			return "";
	}
	
private:
	inifile ini;
	bool loaded;
};

AppDirs::AppDirs(const std::string& dirs_config_file)
{
#ifdef _WIN32
	std::string t_path;
#endif
	std::string l_dirs_config_file = get_dirs_config_file(dirs_config_file);
#ifdef _WIN32
	if(abs_path_to_app_dir(l_dirs_config_file, t_path)) {
		g_error(_("Unable to resolve StarDict directories config file: %s."), 
			l_dirs_config_file.c_str());
	}
	l_dirs_config_file = t_path;
#endif
	AppDirsConf app_conf;
	app_conf.load(l_dirs_config_file);

	std::string path;

	path = app_conf.get_string_at("user_config_dir");
	user_config_dir = path.empty() ? get_default_user_config_dir() : path;
#ifdef _WIN32
	if(abs_path_to_app_dir(user_config_dir, t_path)) {
		g_error(_("Unable to resolve user config directory: %s."), 
			user_config_dir.c_str());
	}
	user_config_dir = t_path;
#endif
	if (!g_file_test(user_config_dir.c_str(), G_FILE_TEST_IS_DIR)) {
		if (-1 == g_mkdir_with_parents(user_config_dir.c_str(), S_IRWXU))
			g_warning(_("Cannot create user config directory %s."), 
				user_config_dir.c_str());
	}

	path = app_conf.get_string_at("user_cache_dir");
	user_cache_dir = path.empty() ? get_default_user_cache_dir() : path;
#ifdef _WIN32
	if(abs_path_to_app_dir(user_cache_dir, t_path)) {
		g_error(_("Unable to resolve user cache directory: %s."), 
			user_cache_dir.c_str());
	}
	user_cache_dir = t_path;
#endif

	path = app_conf.get_string_at("data_dir");
	data_dir = path.empty() ? get_default_data_dir() : path;
#ifdef _WIN32
	if(abs_path_to_app_dir(data_dir, t_path)) {
		g_error(_("Unable to resolve data directory: %s."), 
			data_dir.c_str());
	}
	data_dir = t_path;
#endif

	path = app_conf.get_string_at("log_dir");
	log_dir = path.empty() ? get_default_log_dir() : path;
#ifdef _WIN32
	if(abs_path_to_app_dir(log_dir, t_path)) {
		g_error(_("Unable to resolve log directory: %s."), 
			log_dir.c_str());
	}
	log_dir = t_path;
#endif
	if(!g_file_test(log_dir.c_str(), G_FILE_TEST_IS_DIR))
		if(-1 == g_mkdir_with_parents(log_dir.c_str(), S_IRWXU))
			g_warning(_("Cannot create log directory %s."), log_dir.c_str());

#ifdef _WIN32
	path = app_conf.get_string_at("dll_dir");
	dll_dir = path.empty() ? data_dir : path;
	if(abs_path_to_app_dir(dll_dir, t_path)) {
		g_error(_("Unable to resolve DLL directory: %s."), 
			dll_dir.c_str());
	}
	dll_dir = t_path;
#endif
	path = app_conf.get_string_at("plugin_dir");
	plugin_dir = path.empty() ? get_default_plugin_dir() : path;
#ifdef _WIN32
	if(abs_path_to_app_dir(plugin_dir, t_path)) {
		g_error(_("Unable to resolve plugin directory: %s."), 
			plugin_dir.c_str());
	}
	plugin_dir = t_path;
#endif
#ifndef CONFIG_GNOME
	path = app_conf.get_string_at("help_dir");
	help_dir = path.empty() ? get_default_help_dir() : path;
#endif
	locale_dir = get_default_locale_dir();
#ifdef _WIN32
	if(abs_path_to_app_dir(locale_dir, t_path)) {
		g_error(_("Unable to resolve locale directory: %s."), 
			locale_dir.c_str());
	}
	locale_dir = t_path;
#endif
}

#ifndef _WIN32
std::string AppDirs::get_system_icon_dir(void) const
{
#ifdef CONFIG_DARWIN
	const gchar* dir = g_getenv("STARDICT_ICON_DIR");
	if (dir) {
		return dir;
	} else {
		return SYSTEM_ICON_DIR; 
	}
#else
	return SYSTEM_ICON_DIR; 
#endif
}
#endif

std::string AppDirs::get_dirs_config_file(const std::string& dirs_config_file) const
{
	if(!dirs_config_file.empty())
		return dirs_config_file;
	const gchar * conf_file_env = g_getenv("STARDICT_DIRS_CONFIG_FILE");
	if(conf_file_env)
		return conf_file_env;
	return build_path(get_default_user_config_dir(), "stardict-dirs.cfg");
}

std::string AppDirs::get_default_user_config_dir(void) const
{
	/* Note
	 * StarDict plugins use user config dir.
	 * Search for get_cfg_filename and g_get_user_config_dir functions.
	 * If you make change to this function, do not forget to change other
	 * functions as well. */
	const gchar *config_path_from_env = g_getenv("STARDICT_CONFIG_PATH");
	if (config_path_from_env)
		return config_path_from_env;
#ifdef _WIN32
	std::string res = g_get_user_config_dir();
	res += G_DIR_SEPARATOR_S "StarDict";
	return res;
#else
	std::string res = g_get_user_config_dir();
	res += G_DIR_SEPARATOR_S "stardict";
	return res;
#endif
}

std::string AppDirs::get_default_user_cache_dir(void) const
{
	std::string res = g_get_user_cache_dir();
	res += G_DIR_SEPARATOR_S "stardict";
	return res;
}

std::string AppDirs::get_default_data_dir(void) const
{
#ifdef _WIN32
	return get_application_dir();
#elif defined(CONFIG_DARWIN)
	const gchar* dir = g_getenv("STARDICT_DATA_DIR");
	if (dir) {
		return dir;
	} else {
		return STARDICT_DATA_DIR;
	}
#else
	return STARDICT_DATA_DIR;
#endif
}

std::string AppDirs::get_default_log_dir(void) const
{
	std::string res = g_get_tmp_dir();
#ifdef _WIN32
	res += G_DIR_SEPARATOR_S "StarDict";
#else
	res += G_DIR_SEPARATOR_S "stardict";
#endif
	return res;
}

std::string AppDirs::get_default_plugin_dir(void) const
{
#ifdef _WIN32
	return build_path(data_dir, "plugins");
#elif defined(CONFIG_DARWIN)
	const gchar* dir = g_getenv("STARDICT_LIB_DIR");
	if (dir) {
		return build_path(dir, "plugins");
	} else {
		return build_path(STARDICT_LIB_DIR, "plugins");
	}
#else
	return build_path(STARDICT_LIB_DIR, "plugins");
#endif
}

#ifndef CONFIG_GNOME
std::string AppDirs::get_default_help_dir(void) const
{
	return build_path(data_dir, "help");
}
#endif

std::string AppDirs::get_default_locale_dir(void) const
{
#ifdef _WIN32
	return build_path(data_dir, "locale");
#elif defined(CONFIG_DARWIN)
	const gchar* dir = g_getenv("STARDICT_LOCALE_DIR");
	if (dir) {
		return dir;
	} else {
		return STARDICT_LOCALE_DIR;
	}
#else
	return STARDICT_LOCALE_DIR;
#endif
}
