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

#ifndef __SD_CONF_H__
#define __SD_CONF_H__

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib.h>
#include <cstring>

#ifdef _WIN32
#  include <windows.h>
#endif

#include <string>
#include <memory>
#include <map>

#include "config_file.h"
#include "lib/iappdirs.h"

const int MIN_MAX_FLOATWIN_WIDTH=80; // if user 's MAX_FLOATWIN_WIDTH setting is less then this,it will be set to DEFAULT_MAX_FLOATWIN_WIDTH.
const int MIN_MAX_FLOATWIN_HEIGHT=60;
#ifdef CONFIG_GPE
const int DEFAULT_MAX_FLOATWIN_WIDTH=160;
const int DEFAULT_MAX_FLOATWIN_HEIGHT=120;
#else
const int DEFAULT_MAX_FLOATWIN_WIDTH=320; // This is the label's width in fact.
const int DEFAULT_MAX_FLOATWIN_HEIGHT=240;
#endif

#ifdef _WIN32
extern HINSTANCE stardictexe_hInstance;

std::string get_application_dir(void);
int abs_path_to_app_dir(const std::string& path, std::string& abs_path);
#endif

template <typename T>
struct Type2Type {
	typedef T OriginType;
};

enum TNotifAreaMiddleClickAction {
	namclaQueryFloatWindow,
	namclaQueryMainWindow,
	namclaDoNothing,
};

/*
 * AppConf class encapsulate
 * all preference of StarDict.
*/

class AppConf {
public:
	AppConf();
	~AppConf();
	void Load();

	typedef std::map<std::string, baseconfval *> cache_t;

	bool get_bool(const char *name) const {
		return get(name, Type2Type<bool>());
	}
	bool get_bool_at(const char *name) const {
		return get_at(name, Type2Type<bool>());
	}

	int get_int(const char *name) const {
		return get(name, Type2Type<int>());
	}

	int get_int_at(const char *name) const {
		return get_at(name, Type2Type<int>());
	}

	double get_double(const char *name) const {
		return get(name, Type2Type<double>());
	}

	double get_double_at(const char *name) const {
		return get_at(name, Type2Type<double>());
	}

	const std::string& get_string(const char *name) const {
		return get(name, Type2Type<std::string>());
	}

	const std::string& get_string_at(const char *name) const {
		return get_at(name, Type2Type<std::string>());
	}

	const std::list<std::string>& get_strlist(const char *name) const {
		return get(name, Type2Type< std::list<std::string> >());
	}

	const std::list<std::string>& get_strlist_at(const char *name) const {
		return get_at(name, Type2Type< std::list<std::string> >());
	}

	void set_bool(const char *name, const bool& v) {
		set_value(name, v);
	}

	void set_bool_at(const char *name, const bool& v) {
		set_value(name, v, false);
	}

	void set_int(const char *name, const int& v) {
		set_value(name, v);
	}

	void set_int_at(const char *name, const int& v) {
		set_value(name, v, false);
	}

	void set_double(const char *name, const double& v) {
		set_value(name, v);
	}

	void set_double_at(const char *name, const double& v) {
		set_value(name, v, false);
	}

	void set_string(const char *name, const std::string& v) {
		set_value(name, v);
	}

	void set_string_at(const char *name, const std::string& v) {
		set_value(name, v, false);
	}

	void set_strlist(const char *name, const std::list<std::string>& v) {
		set_value(name, v);
	}

	void set_strlist_at(const char *name, const std::list<std::string>& v) {
		set_value(name, v, false);
	}

	template <typename T>
	void add_entry(const char *name, const T& def_val) {
		confval<T> *v = new confval<T>(def_val);
		g_assert(v->type_ != baseconfval::UNKNOWN_TYPE);
		cache[name] = v;
	}
	void notify_add(const char * name, const sigc::slot<void, const baseconfval*>&);
private:
	std::unique_ptr<config_file> cf;
	cache_t cache;

	template <typename T>
	void set_value(const char *name, const T& val, bool abs = true) {
		cache_t::iterator p;
		p = abs ? cache.find(name) : 
			cache.find(std::string("/apps/stardict/preferences/") +
				   name);
		if (p == cache.end() ||
		    static_cast<confval<T> *>(p->second)->val_ == val)
			return;

		confval<T> *cfgval = static_cast<confval<T> *> (p->second);
		cfgval->val_ = val;

		size_t len = strlen(name);
		const char *key = strrchr(name, '/');
		if (!key)
			key = name + len;
		std::string sect(name, 0, key - name);
		if (!*key)
			key = "";
		else
			++key;
		if (!abs)
			sect = "/apps/stardict/preferences/" + sect;

		cfgval->save(*cf, sect.c_str(), key);
	}

	static std::string get_default_history_filename();
	static std::string get_default_export_filename();
	static std::list<std::string> get_default_search_website_list();
#ifdef _WIN32
	static bool get_win32_use_custom_font();
	static std::string get_win32_custom_font();
#endif
#ifdef CONFIG_DARWIN
	static bool get_darwin_use_custom_font();
	static std::string get_darwin_custom_font();
#endif

	template <typename T>
	const T& get(const char *name, Type2Type<T>) const {
		static T empty;
		cache_t::const_iterator p = cache.find(name);
		if (p != cache.end())
			return static_cast<confval<T> *>(p->second)->val_;
		return empty;
	}

	template <typename T>
	const T& get_at(const char *name, Type2Type<T>) const {
		return get((std::string("/apps/stardict/preferences/") +
			    name).c_str(), Type2Type<T>());
	}
};

/* Collection of StarDict directories.
 * Most of the dirs can be overridden with stardict-dirs.cfg file.
 * 
 * Implementation note
 * 
 * An object of this class is created very early in application initialization
 * process. Be careful implementing this class, restrict used library objects
 * to minimum. If it's found out that inifile class can not be used, we may
 * retreat to environment variables. For example, STARDICT_DATA_DIR for 
 * data_dir.
 * */
class AppDirs final : public IAppDirs
{
public:
	explicit AppDirs(const std::string& dirs_config_file);
	std::string get_user_config_dir(void) const { return user_config_dir; }
	std::string get_user_cache_dir(void) const { return user_cache_dir; }
	std::string get_data_dir(void) const { return data_dir; }
	std::string get_log_dir(void) const { return log_dir; }
#ifdef _WIN32
	std::string get_dll_dir(void) const { return dll_dir; }
#endif
	std::string get_plugin_dir(void) const { return plugin_dir; }
#ifndef CONFIG_GNOME
	std::string get_help_dir(void) const { return help_dir; }
#endif
	std::string get_locale_dir(void) const { return locale_dir; }
#ifndef _WIN32
	std::string get_system_icon_dir(void) const;
#endif
#ifdef CONFIG_GNOME
	std::string get_system_data_dir(void) const { return SYSTEM_DATA_DIR; }
#endif
	
private:
	std::string get_dirs_config_file(const std::string& dirs_config_file) const;
	std::string get_default_user_config_dir(void) const;
	std::string get_default_user_cache_dir(void) const;
	std::string get_default_data_dir(void) const;
	std::string get_default_log_dir(void) const;
	std::string get_default_plugin_dir(void) const;
	std::string get_default_help_dir(void) const;
	std::string get_default_locale_dir(void) const;
	
private:
	std::string user_config_dir;
	std::string user_cache_dir;
	/* contains subdirs: dic, treedict, sounds, skins, pixmaps, locale */
	std::string data_dir;
	std::string log_dir;
#ifdef _WIN32
	/* dir with TextOutSpy.dll and TextOutHook.dll */
	std::string dll_dir;
#endif
	/* StarDict plugins dir */
	std::string plugin_dir;
#ifndef CONFIG_GNOME
	/* help dir, contains subdirs: C, mk, nl,... with localized manuals. */
	std::string help_dir;
#endif
	/* dir containing getgext translated messages */
	std::string locale_dir;
};


extern std::unique_ptr<AppConf> conf;//global exemplar of AppConf class
extern std::unique_ptr<AppDirs> conf_dirs;

#endif
