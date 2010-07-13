/*
 * This file part of StarDict - A international dictionary for GNOME.
 * http://stardict.sourceforge.net
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <glib.h>
#include <glib/gstdio.h>
#include <glib/gi18n.h>
#include <cstring>
#include <cstdlib>

#include "class_factory.hpp"
#include "lib/utils.h"
#include "inifile.hpp"

#include "conf.h"

#ifdef CONFIG_GPE
const int DEFAULT_WINDOW_WIDTH=238;
const int DEFAULT_WINDOW_HEIGHT=279;
const int DEFAULT_HPANED_POS=79;
#else
const int DEFAULT_WINDOW_WIDTH=463;
const int DEFAULT_WINDOW_HEIGHT=321;
const int DEFAULT_HPANED_POS=127;
#endif

std::auto_ptr<AppConf> conf;
std::auto_ptr<AppDirs> conf_dirs;

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
	add_entry("/apps/stardict/preferences/network/enable_netdict", true);
	// Default stardictd server.
	add_entry("/apps/stardict/preferences/network/server", std::string(_("dict.stardict.org")));
	add_entry("/apps/stardict/preferences/network/port", 2628);
	add_entry("/apps/stardict/preferences/network/user", std::string());
	add_entry("/apps/stardict/preferences/network/md5passwd", std::string());
	add_entry("/apps/stardict/preferences/main_window/skin", std::string());
	add_entry("/apps/stardict/preferences/main_window/hide_on_startup", false);
	add_entry("/apps/stardict/preferences/main_window/search_while_typing", true);
	add_entry("/apps/stardict/preferences/main_window/word_change_timeout", 300);
	add_entry("/apps/stardict/preferences/main_window/showfirst_when_notfound", true);
	add_entry("/apps/stardict/preferences/translate/engine", 0);
	add_entry("/apps/stardict/preferences/translate/fromlang", 0);
	add_entry("/apps/stardict/preferences/translate/tolang", 0);
	add_entry("/apps/stardict/preferences/dictionary/enable_sound_event", true);
	add_entry("/apps/stardict/preferences/dictionary/use_tts_program", false);
	add_entry("/apps/stardict/preferences/dictionary/tts_program_cmdline", std::string());
	add_entry("/apps/stardict/preferences/main_window/hide_list", false);
	add_entry("/apps/stardict/preferences/dictionary/scan_selection", true);
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
	add_entry("/apps/stardict/preferences/floating_window/bg_red", 65535);
	add_entry("/apps/stardict/preferences/floating_window/bg_green", 65535);
	add_entry("/apps/stardict/preferences/floating_window/bg_blue", 51200);
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

	add_entry("/apps/stardict/preferences/dictionary/sound_play_command", std::string("play"));
#if defined(_WIN32) || defined(CONFIG_GNOME)
	add_entry("/apps/stardict/preferences/dictionary/always_use_sound_play_command", false);
#endif
	add_entry("/apps/stardict/preferences/dictionary/video_play_command", std::string("play"));
#if defined(CONFIG_GPE)
	add_entry("/apps/stardict/preferences/dictionary/url_open_command", std::string("gpe-mini-browser"));
#else
	add_entry("/apps/stardict/preferences/dictionary/url_open_command", std::string("firefox"));
#endif
#if defined(_WIN32) || defined(CONFIG_GNOME)
	add_entry("/apps/stardict/preferences/dictionary/always_use_open_url_command", false);
#endif
#ifdef _WIN32
	add_entry("/apps/stardict/preferences/dictionary/tts_path", std::string("C:\\Program Files\\WyabdcRealPeopleTTS\nC:\\Program Files\\OtdRealPeopleTTS\nWyabdcRealPeopleTTS\nOtdRealPeopleTTS"));
#else
	add_entry("/apps/stardict/preferences/dictionary/tts_path", std::string("/usr/share/WyabdcRealPeopleTTS\n/usr/share/OtdRealPeopleTTS"));
#endif
	add_entry("/apps/stardict/preferences/dictionary/history", get_default_history_filename());
	add_entry("/apps/stardict/preferences/dictionary/only_export_word", true);
	add_entry("/apps/stardict/preferences/dictionary/export_file", get_default_export_filename());

	add_entry("/apps/stardict/preferences/main_window/search_website_list", std::list<std::string>());
	add_entry("/apps/stardict/manage_dictionaries/treedict_order_list", std::list<std::string>());
	add_entry("/apps/stardict/manage_dictionaries/treedict_disable_list", std::list<std::string>());
	add_entry("/apps/stardict/manage_dictionaries/dict_order_list", std::list<std::string>());
	add_entry("/apps/stardict/manage_dictionaries/dict_config_xml", std::string());
	add_entry("/apps/stardict/manage_dictionaries/dict_default_group", std::string());

	add_entry("/apps/stardict/manage_plugins/plugin_order_list", std::list<std::string>());
	add_entry("/apps/stardict/manage_plugins/plugin_disable_list", std::list<std::string>());

	std::list<std::string> dirs;
	dirs.push_back(conf_dirs->get_data_dir() + G_DIR_SEPARATOR_S "dic");
#ifndef _WIN32
	if (conf_dirs->get_data_dir() != "/usr/share/stardict") {
		dirs.push_back("/usr/share/stardict/dic");
	}
	dirs.push_back(std::string(g_get_home_dir())+"/.stardict/dic");
#endif
	add_entry("/apps/stardict/manage_dictionaries/dict_dirs_list", dirs);

	dirs.clear();
	dirs.push_back(conf_dirs->get_data_dir()+ G_DIR_SEPARATOR_S "treedict");
#ifndef _WIN32
	dirs.push_back(std::string(g_get_home_dir())+"/.stardict/treedict");
#endif
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
	histname = conf_dirs->get_user_config_dir() + G_DIR_SEPARATOR_S "history.txt";
#else
	histname = conf_dirs->get_user_config_dir() + G_DIR_SEPARATOR_S "history";
#endif

	return histname;
}

std::string AppConf::get_default_export_filename()
{
	std::string exportname;
#ifdef _WIN32
	exportname = conf_dirs->get_data_dir() + G_DIR_SEPARATOR_S "dic.txt";
#else
	exportname = g_get_home_dir();
	exportname+= G_DIR_SEPARATOR_S "dic.txt";
#endif
	return exportname;
}

std::list<std::string> AppConf::get_default_search_website_list()
{
	/* xgettext: no-c-format */
	gchar *default_website = _("StarDict.org	http://www.stardict.org	http://www.stardict.org/query.php?q=%s\n"
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
	
	void load(const std::string& user_config_dir)
	{
		std::string conf_file
			= user_config_dir + G_DIR_SEPARATOR_S + "stardict-dirs.cfg";
		const gchar * conf_file_env = g_getenv("STARDICT_DIRS_CONFIG_FILE");
		if(conf_file_env)
			conf_file = conf_file_env;
		if(g_file_test(conf_file.c_str(),
			GFileTest(G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR))) {
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

AppDirs::AppDirs(void)
{
	user_config_dir = get_default_user_config_dir();
	if (!g_file_test(user_config_dir.c_str(), G_FILE_TEST_IS_DIR)) {
		if (-1 == g_mkdir(user_config_dir.c_str(), S_IRWXU))
			g_warning("Cannot create user config directory %s.", user_config_dir.c_str());
	}
	AppDirsConf app_conf;
	app_conf.load(user_config_dir);

	std::string path;
	path = app_conf.get_string_at("data_dir");
	data_dir = path.empty() ? get_default_data_dir() : path;

	path = app_conf.get_string_at("log_dir");
	log_dir = path.empty() ? get_default_log_dir() : path;
	if(!g_file_test(log_dir.c_str(), G_FILE_TEST_IS_DIR))
		if(-1 == g_mkdir_with_parents(log_dir.c_str(), S_IRWXU))
			g_warning("Cannot create log directory %s.", log_dir.c_str());

#ifdef _WIN32
	path = app_conf.get_string_at("dll_dir");
	dll_dir = path.empty() ? data_dir : path;
#endif
	path = app_conf.get_string_at("plugin_dir");
	plugin_dir = path.empty() ? get_default_plugin_dir() : path;
#ifndef CONFIG_GNOME
	path = app_conf.get_string_at("help_dir");
	help_dir = path.empty() ? get_default_help_dir() : path;
#endif
	locale_dir = get_default_locale_dir();
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
	std::string res = g_get_home_dir();
	res += G_DIR_SEPARATOR_S ".stardict";
	return res;
#endif
}

std::string AppDirs::get_default_data_dir(void) const
{
#ifdef _WIN32
	HMODULE hmod;

	if ((hmod = GetModuleHandle(NULL))==0)
		exit(EXIT_FAILURE);
	TCHAR path_win[MAX_PATH];
	DWORD dwRes = GetModuleFileName(hmod, path_win, MAX_PATH);
	if(dwRes == 0 || dwRes == MAX_PATH)
		exit(EXIT_FAILURE);
	std::string path_utf8;
	std::string path;
	if(windows_to_utf8(path_win, path_utf8) && utf8_to_file_name(path_utf8, path)) {
		glib::CharStr buf(g_path_get_dirname(path.c_str()));
		return get_impl(buf);
	} else
		exit(EXIT_FAILURE);
	return "";
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
	return data_dir + G_DIR_SEPARATOR_S "plugins";
#else
	return STARDICT_LIB_DIR G_DIR_SEPARATOR_S "plugins";
#endif
}

#ifndef CONFIG_GNOME
std::string AppDirs::get_default_help_dir(void) const
{
	return data_dir + G_DIR_SEPARATOR_S "help";
}
#endif

std::string AppDirs::get_default_locale_dir(void) const
{
#ifdef _WIN32
	return data_dir + G_DIR_SEPARATOR_S "locale";
#else
	return STARDICT_LOCALE_DIR;
#endif
}
