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

#ifndef _IAPPDIRS_H_
#define _IAPPDIRS_H_

#include <gtk/gtk.h>
#include <string>
#include <list>

#ifdef _WIN32
std::string rel_path_to_data_dir(const std::string& path);
std::string abs_path_to_data_dir(const std::string& path);
void rel_path_to_data_dir(const std::list<std::string>& paths_abs, 
	std::list<std::string>& paths_rel);
void abs_path_to_data_dir(const std::list<std::string>& paths_rel,
	std::list<std::string>& paths_abs);
#endif

class IAppDirs
{
public:
	virtual std::string get_user_config_dir(void) const = 0;
	virtual std::string get_user_cache_dir(void) const = 0;
	virtual std::string get_data_dir(void) const = 0;
};

/* This global pointer exports application directories to stardict-lib.
You should assign the pointer before using the library.
conf_dirs global variable from conf.h is not available here, 
it is not part of the library. */
extern IAppDirs* app_dirs;

#endif
