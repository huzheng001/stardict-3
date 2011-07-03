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

#ifndef _FILE_UTILS_H_
#define _FILE_UTILS_H_

#include <algorithm>
#include <glib.h>
#include <list>
#include <string>
#include <cstring>
#include "utils.h"
#include "libcommon.h"

typedef std::list<std::string> List;

class PathComparePred
{
public:
	PathComparePred(const std::string& path)
		: path(path)
	{
	}
	bool operator()(const PathComparePred& right)
	{
		return is_equal_paths(path, right.path);
	}

private:
	std::string path;
};

template<typename Function>
void __for_each_file(const std::string& dirname, const std::string& suff,
			 const List& order_list, const List& disable_list, 
			 Function f)
{
	GDir *dir = g_dir_open(dirname.c_str(), 0, NULL);
	if (dir) {
		const gchar *filename;
		while ((filename = g_dir_read_name(dir))!=NULL) {
			std::string fullfilename(build_path(dirname, filename));
			if (g_file_test(fullfilename.c_str(), G_FILE_TEST_IS_DIR))
				__for_each_file(fullfilename, suff, order_list, disable_list, f);
			else if (is_path_end_with(filename, suff)
					&& std::find_if(order_list.begin(), order_list.end(), PathComparePred(fullfilename))==order_list.end()) { 
				bool disable=std::find_if(disable_list.begin(), disable_list.end(), PathComparePred(fullfilename))!=disable_list.end();
				f(fullfilename, disable);
			}
		}
		g_dir_close(dir);
	}
}

/* for each file in the order_list invoke f(<file name>, <disable>)
 * for each dir in dirs_list find all files ending with suff, invoke 
 * f(<file name>, <disable>) for each found file. */
template<typename Function>
void for_each_file(const List& dirs_list, const std::string& suff,
			 const List& order_list, const List& disable_list, 
			 Function f)
{
	List::const_iterator it;
	for (it=order_list.begin(); it!=order_list.end(); ++it) {
		bool disable=std::find_if(disable_list.begin(), disable_list.end(), PathComparePred(*it))!=disable_list.end();
		f(*it, disable);
	}
	for (it=dirs_list.begin(); it!=dirs_list.end(); ++it)
		__for_each_file(*it, suff, order_list, disable_list, f);
}

template<typename Function>
void __for_each_file_restricted(const std::string& dirname, const std::string& suff,
			 const List& order_list, const List& disable_list, 
			 Function f)
{
	GDir *dir = g_dir_open(dirname.c_str(), 0, NULL);
	if(!dir)
		return;
	const gchar *filename;
	bool found = false;
	// search for a file with a suff suffix
	while ((filename = g_dir_read_name(dir))!=NULL) {
		std::string fullfilename(build_path(dirname, filename));
		if (!g_file_test(fullfilename.c_str(), G_FILE_TEST_IS_DIR)
			&& is_path_end_with(filename, suff)) {
			found = true;
			if(std::find_if(order_list.begin(), order_list.end(), PathComparePred(fullfilename))==order_list.end()) { 
				bool disable=std::find_if(disable_list.begin(), disable_list.end(), PathComparePred(fullfilename))!=disable_list.end();
				f(fullfilename, disable);
			}
		}
	}
	g_dir_close(dir);
	// descend into subdirectories
	dir = g_dir_open(dirname.c_str(), 0, NULL);
	if(!dir)
		return;
	while ((filename = g_dir_read_name(dir))!=NULL) {
		if(found && is_equal_paths(filename, "res"))
			continue;
		std::string fullfilename(build_path(dirname, filename));
		if (g_file_test(fullfilename.c_str(), G_FILE_TEST_IS_DIR))
			__for_each_file_restricted(fullfilename, suff, order_list, disable_list, f);
	}
	g_dir_close(dir);
}

/* like for_each_file, but does not descend into "res" subdirectory of 
 * a directory where a file with suff suffix is found. */
template<typename Function>
void for_each_file_restricted(const List& dirs_list, const std::string& suff,
			 const List& order_list, const List& disable_list, 
			 Function f)
{
	List::const_iterator it;
	for (it=order_list.begin(); it!=order_list.end(); ++it) {
		bool disable=std::find_if(disable_list.begin(), disable_list.end(), PathComparePred(*it))!=disable_list.end();
		f(*it, disable);
	}
	for (it=dirs_list.begin(); it!=dirs_list.end(); ++it)
		__for_each_file_restricted(*it, suff, order_list, disable_list, f);
}

template<typename Function>
void __for_each_dir(const std::string& dirname, Function f)
{
	GDir *dir = g_dir_open(dirname.c_str(), 0, NULL);
	if (dir) {
		const gchar *filename;
		while ((filename = g_dir_read_name(dir))) {
			std::string fullfilename(build_path(dirname, filename));
			if (g_file_test(fullfilename.c_str(), G_FILE_TEST_IS_DIR))
				 f(fullfilename, false);
		}
		g_dir_close(dir);
	}
}

template<typename Function>
void for_each_dir(const List& dirs_list, Function f)
{
	List::const_iterator it;
	for (it=dirs_list.begin(); it!=dirs_list.end(); ++it)
		__for_each_dir(*it, f);
}

#endif//!_FILE_UTILS_H_
