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

#ifndef _GCONF_FILE_HPP_
#define _GCONF_FILE_HPP_

#include <gconf/gconf.h>
#include <gconf/gconf-client.h>
#include <string>
#include <vector>

#include "config_file.h"

class gconf_file : public config_file {
public:
	explicit gconf_file(const std::string& path);
	~gconf_file();
	bool read_bool(const gchar *sect, const gchar *key, bool& val);
	bool read_int(const gchar *sect, const gchar *key, int& val);
	bool read_double(const gchar *sect, const gchar *key, double& val);
	bool read_string(const gchar *, const gchar *, std::string&);
	bool read_strlist(const gchar *, const gchar *, std::list<std::string>&);

	void write_bool(const gchar *sect, const gchar *key, bool val);
	void write_int(const gchar *sect, const gchar *key, int val);
	void write_double(const gchar *sect, const gchar *key, double val);
	void write_string(const gchar *sect, const gchar *key, const std::string& val);
	void write_strlist(const gchar *sect, const gchar *key, const std::list<std::string>& slist);
	void notify_add(const gchar *, const gchar *,
                        const sigc::slot<void, const baseconfval*>&);
private:
	std::string cfgname;
	GConfClient *gconf_client;
	std::vector<guint> notification_ids;
};

#endif//!_GCONF_FILE_HPP_
