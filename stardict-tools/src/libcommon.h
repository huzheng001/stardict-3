#ifndef _LIBCOMMON_H_
#define _LIBCOMMON_H_

#include <glib.h>
#include <string>
#include <vector>
#ifdef _WIN32
#include <windows.h>
#endif

typedef void (*print_info_t)(const char *info, ...);

extern gint stardict_strcmp(const gchar *s1, const gchar *s2);
extern bool file_name_to_utf8(const std::string& str, std::string& out);
extern bool utf8_to_file_name(const std::string& str, std::string& out);
#ifdef _WIN32
typedef std::basic_string<TCHAR> std_win_string;
extern bool utf8_to_windows(const std::string& str_utf8, std_win_string& out);
extern bool windows_to_utf8(const std_win_string& str, std::string& out_utf8);
#endif
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

enum TLoadResult { lrOK, lrError, lrNotFound };

int unpack_zlib(const char* arch_file_name, const char* out_file_name, print_info_t print_info);

/* allows to create a temp file, remove the temp file when the object is destroyed. */
class TempFile
{
public:
	explicit TempFile(print_info_t print_info = g_print)
		: print_info(print_info)
	{
	}
	~TempFile(void)
	{
		clear();
	}
	const std::string& create_temp_file(void);
	const std::string& get_file_name(void) const
	{
		return file_name;
	}
	void set_print_info(print_info_t print_info)
	{
		this->print_info = print_info;
	}
private:
	void clear(void);
	std::string file_name;
	print_info_t print_info;
};

/* Create a new temporary file. Return file name in file name encoding.
Return an empty string if file cannot be created. */
std::string create_temp_file(void);

#define read_file_err "Error reading file %s.\n"
#define write_file_err "Error writing file %s.\n"
#define index_file_truncated_err "Index file is truncated, last record is truncated.\n"

#endif

