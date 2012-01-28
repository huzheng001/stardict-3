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

#ifndef _INIFILE_HPP_
#define _INIFILE_HPP_

#include <map>
#include <string>
#include <glib.h>

#include "config_file.h"

class inifile : public config_file {
public:
	inifile();
	~inifile();
	/* read_only = true guarantees that the ini file will not be changed.
	 * if strict = true, check ini file version. */
	bool load(const std::string& path, bool read_only = false, bool strict = true);
	bool read_bool(const gchar *sect, const gchar *key, bool& val);
	bool read_int(const gchar *sect, const gchar *key, int& val);
	bool read_double(const gchar *sect, const gchar *key, double& val);
	bool read_string(const gchar * sect, const gchar *key, std::string& val);
	bool read_strlist(const gchar * sect, const gchar *key, std::list<std::string>& slist);

	void write_bool(const gchar *sect, const gchar *key, bool val);
	void write_int(const gchar *sect, const gchar *key, int val);
	void write_double(const gchar *sect, const gchar *key, double val);
	void write_string(const gchar *sect, const gchar *key, const std::string& val);
	void write_strlist(const gchar *sect, const gchar *key, const std::list<std::string>& slist);

	void notify_add(const gchar *sect, const gchar *key, 
			const sigc::slot<void, const baseconfval*>&);

private:
	typedef std::map<std::string, sigc::signal<void, const baseconfval *> >
		ChangeEventsMap;
	enum CheckResult { crOK, crError, crRetry };
	ChangeEventsMap change_events_map_;
	GKeyFile *gkeyfile_;
	std::string fname_;
	bool read_only_;

	template <typename T>
	void expose_event(const gchar *sect, const gchar *key, const T& val) {
		std::string name = std::string(sect) + "/" + key;
		ChangeEventsMap::iterator p = change_events_map_.find(name);
		if (p == change_events_map_.end())
			return;
		confval<T> cv(val);
		p->second.emit(&cv);
	}
	bool save();
	bool fix_and_save();
	bool convert_from_locale_enc();
	CheckResult check_config_version();
	CheckResult load_ini_file();
};

#endif//!_INIFILE_HPP_
