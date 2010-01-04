#ifndef STARDICTUTILS_H_
#define STARDICTUTILS_H_

#include <glib.h>
#include <string>

#define DB_DIR_SEPARATOR '/'
#define DB_DIR_SEPARATOR_S "/"
/* functions to convert directory separator characters 
 * 1. file system separator character = G_DIR_SEPARATOR
 * 2. database separator character = DB_DIR_SEPARATOR */
#if DB_DIR_SEPARATOR == G_DIR_SEPARATOR
inline std::string dir_separator_fs_to_db(const std::string& path)
{
	return path;
}
inline std::string dir_separator_db_to_fs(const std::string& path)
{
	return path;
}
#else
extern std::string dir_separator_fs_to_db(const std::string& path);
extern std::string dir_separator_db_to_fs(const std::string& path);
#endif

#endif /* STARDICTUTILS_H_ */
