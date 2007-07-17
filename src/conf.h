#ifndef __SD_CONF_H__
#define __SD_CONF_H__

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib.h>
#include <map>
#include <memory>
#include <string>
#include <cstring>

#include "config_file.hpp"

const int MIN_MAX_FLOATWIN_WIDTH=80; // if user 's MAX_FLOATWIN_WIDTH setting is less then this,it will be set to DEFAULT_MAX_FLOATWIN_WIDTH.
const int MIN_MAX_FLOATWIN_HEIGHT=60;
#ifdef CONFIG_GPE
const int DEFAULT_MAX_FLOATWIN_WIDTH=160;
const int DEFAULT_MAX_FLOATWIN_HEIGHT=120;
#else
const int DEFAULT_MAX_FLOATWIN_WIDTH=320; // This is the lable's width in fact.
const int DEFAULT_MAX_FLOATWIN_HEIGHT=240;
#endif

template <typename T>
struct Type2Type {
	typedef T OriginType;
};

/*
 * AppConf class encapsulate
 * all preference of stardict.
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
	std::auto_ptr<config_file> cf;
	cache_t cache;

#ifdef _WIN32
static void *memrchr(const void *mem, int c, size_t len) {
	char *res;
	char *cmem = (char *)mem;

	if (!len)
		return NULL;
	res = cmem + len - 1;
	while (res != cmem - 1 && *res != c)
		--res;
	return res == cmem - 1 ? NULL : res;
}
#endif

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
		//TODO: do not user memrchr
		const char *key = (char *)memrchr(name, '/', len);
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

extern std::auto_ptr<AppConf> conf;//global exemplar of AppConf class
extern std::string gStarDictDataDir;

#endif
