#ifndef _FILE_HPP_
#define _FILE_HPP_

#include <algorithm>
#include <glib.h>
#include <list>
#include <string>
#include <cstring>
#include "utils.h"

typedef std::list<std::string> List;

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
			else if (g_str_has_suffix(filename, suff.c_str()) &&
					 std::find(order_list.begin(), order_list.end(), fullfilename)==order_list.end()) { 
				 bool disable=std::find(disable_list.begin(), disable_list.end(), fullfilename)!=disable_list.end();
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
		bool disable=std::find(disable_list.begin(), disable_list.end(), *it)!=disable_list.end();
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
			&& g_str_has_suffix(filename, suff.c_str())) {
			found = true;
			if(std::find(order_list.begin(), order_list.end(), fullfilename)==order_list.end()) { 
				bool disable=std::find(disable_list.begin(), disable_list.end(), fullfilename)!=disable_list.end();
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
		if(found && strcmp(filename, "res") == 0)
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
		bool disable=std::find(disable_list.begin(), disable_list.end(), *it)!=disable_list.end();
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

#endif//!_FILE_HPP_
