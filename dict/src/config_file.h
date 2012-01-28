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

#ifndef _CONFIG_FILE_HPP_
#define _CONFIG_FILE_HPP_

#include <glib.h>
#include <list>
#include <string>

#include "lib/stardict-sigc++.h"

class config_file;

struct baseconfval {
	enum Type {
		UNKNOWN_TYPE, BOOL_TYPE, INT_TYPE, DOUBLE_TYPE,
		STRING_TYPE, STRLIST_TYPE
	} type_;

	baseconfval(Type t = UNKNOWN_TYPE) : type_(t) {}
	virtual ~baseconfval() {}
	virtual bool load(config_file&, const gchar *, const gchar *) = 0;
};

template<typename T> struct confval;

/*
 * This interface must be implemented if you want
 * realize your own way of loading and saving stardict's preference
 */
class config_file {
public:
	virtual ~config_file() {}
	virtual bool read_bool(const gchar *sect, const gchar *key, bool& val) = 0;
	virtual bool read_int(const gchar *sect, const gchar *key, int& val) = 0;
	virtual bool read_double(const gchar *sect, const gchar *key, double& val) = 0;
	virtual bool read_string(const gchar *sect, const gchar *key, std::string& val) = 0;
	virtual bool read_strlist(const gchar * sect, const gchar *key, std::list<std::string>& slist) = 0;

	virtual void write_bool(const gchar *sect, const gchar *key, bool val) = 0;
	virtual void write_int(const gchar *sect, const gchar *key, int val) = 0;
	virtual void write_double(const gchar *sect, const gchar *key, double val) = 0;
	virtual void write_string(const gchar *sect, const gchar *key, const std::string& val) = 0;
	virtual void write_strlist(const gchar *, const gchar *, const std::list<std::string>&) = 0;
	virtual void notify_add(const gchar *sect, const gchar *key,
				const sigc::slot<void, const baseconfval*>&) = 0;
};

#define DEFINE_CONFVAL(Type, Name, MethodExt)		\
	template <> \
	struct confval<Type> : public baseconfval { \
			Type val_; \
			confval() : baseconfval(Name) {} \
			confval(const Type & v) : baseconfval(Name), val_(v) {} \
			bool load(config_file& cf, const gchar *sect, const gchar *key) { \
				return cf.read_##MethodExt(sect, key, val_); \
			} \
			void save(config_file& cf, const gchar *sect, const gchar *key) { \
				cf.write_##MethodExt(sect, key, val_); \
			} \
	}

DEFINE_CONFVAL(bool, BOOL_TYPE, bool);
DEFINE_CONFVAL(int, INT_TYPE, int);
DEFINE_CONFVAL(double, DOUBLE_TYPE, double);
DEFINE_CONFVAL(std::string, STRING_TYPE, string);
typedef std::list<std::string> StringList;
DEFINE_CONFVAL(StringList, STRLIST_TYPE, strlist);


#endif//!_CONFIG_FILE_HPP_
