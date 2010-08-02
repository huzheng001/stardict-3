#ifndef _LIBCOMMON_H_
#define _LIBCOMMON_H_

#include <glib.h>
#include <string>
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


#endif

