#include "stardict-utils.h"

#if DB_DIR_SEPARATOR == G_DIR_SEPARATOR
#else
std::string dir_separator_fs_to_db(const std::string& path)
{
	std::string temp(path);
	const std::string::size_type len = temp.length();
	for(size_t i=0; i<len; ++i)
		if(temp[i] == G_DIR_SEPARATOR)
			temp[i] = DB_DIR_SEPARATOR;
	return temp;
}

std::string dir_separator_db_to_fs(const std::string& path)
{
	std::string temp(path);
	const std::string::size_type len = temp.length();
	for(size_t i=0; i<len; ++i)
		if(temp[i] == DB_DIR_SEPARATOR)
			temp[i] = G_DIR_SEPARATOR;
	return temp;
}
#endif

