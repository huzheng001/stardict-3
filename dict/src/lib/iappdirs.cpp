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

#include "utils.h"
#include "libcommon.h"
#include "iappdirs.h"

IAppDirs* app_dirs = NULL;

#ifdef _WIN32
/* 
rel_path_to_data_dir functions:
convert a full path into a relative path using the application directory as the base dir
If a relative path cannot be created, for example, different disk or any error 
occurred in the convertion routine, return the original path.

abs_path_to_data_dir functions:
convert a path relative to the application directory to an absolute path
If the path is absolute or cannot be resolved, return the original path. */
std::string rel_path_to_data_dir(const std::string& path)
{
	std::string path_norm;
	if(norm_path_win(path, path_norm))
		return path;
	if(!is_absolute_path_win(path_norm))
		return path_norm;
	std::string path_rel;
	if(build_relative_path(app_dirs->get_data_dir(), path_norm, path_rel))
		return path_norm;
	return path_rel;
}

std::string abs_path_to_data_dir(const std::string& path)
{
	std::string path_norm;
	if(norm_path_win(path, path_norm))
		return path;
	if(is_absolute_path_win(path_norm))
		return path_norm;
	std::string path_abs(build_path(app_dirs->get_data_dir(), path_norm));
	std::string path_abs_norm;
	if(norm_path_win(path_abs, path_abs_norm))
		return path_abs;
	return path_abs_norm;
}

void rel_path_to_data_dir(const std::list<std::string>& paths_abs, 
	std::list<std::string>& paths_rel)
{
	paths_rel.clear();
	for(std::list<std::string>::const_iterator it=paths_abs.begin(); it != paths_abs.end(); ++it)
		paths_rel.push_back(rel_path_to_data_dir(*it));
}

void abs_path_to_data_dir(const std::list<std::string>& paths_rel,
	std::list<std::string>& paths_abs)
{
	paths_abs.clear();
	for(std::list<std::string>::const_iterator it=paths_rel.begin(); it != paths_rel.end(); ++it)
		paths_abs.push_back(abs_path_to_data_dir(*it));
}

#endif // #ifdef _WIN32
