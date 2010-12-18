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
	virtual std::string get_data_dir(void) const = 0;
};

/* This global pointer exports application directories to stardict-lib.
You should assign the pointer before using the library.
conf_dirs global variable from conf.h is not available here, 
it is not part of the library. */
extern IAppDirs* app_dirs;

#endif
