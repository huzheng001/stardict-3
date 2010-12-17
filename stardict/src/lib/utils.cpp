/* 
 * This file part of StarDict - A international dictionary for GNOME.
 * http://stardict.sourceforge.net
 * Copyright (C) 2005-2006 Evgeniy <dushistov@mail.ru>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <cstring>
#include <glib.h>
#include <glib/gi18n.h>
#include <cstdlib>
#include <gtk/gtk.h>

#ifdef CONFIG_GNOME
#  include <libgnome/libgnome.h>
#  include <libgnomeui/libgnomeui.h>
#elif defined(_WIN32)
#  include <gdk/gdkwin32.h>
#  include <Shlwapi.h>
#endif

#include "my_global.h"
#include "utils.h"


void ProcessGtkEvent()
{
  while (gtk_events_pending())
    gtk_main_iteration();
}

std::string combnum2str(gint comb_code)
{
  switch (comb_code) {
#ifdef _WIN32
  case 0:
    return "Shift";
  case 1:
    return "Alt";
  case 2:
    return "Ctrl";
  case 3:
    return "Ctrl+Alt";
#else
  case 0:
    return "Win";
  case 1:
    return "Shift";
  case 2:
    return "Alt";
  case 3:
    return "Ctrl";
  case 4:
    return "Ctrl+Alt";
  case 5:
    return "Ctrl+e";
  case 6:
    return "Win+d";
  case 7:
    return "F1";
  case 8:
    return "F2";
  case 9:
    return "F3";
  case 10:
    return "F4";
#endif
  default:
    return "";
  }
}

std::vector<std::string> split(const std::string& str, char sep)
{
	std::vector<std::string> res;
	std::string::size_type prev_pos=0, pos = 0;
	while ((pos=str.find(sep, prev_pos))!=std::string::npos) {
		res.push_back(std::string(str, prev_pos, pos-prev_pos));
		prev_pos=pos+1;
	}
	res.push_back(std::string(str, prev_pos, str.length()-prev_pos));

	return res;
}

GdkPixbuf *load_image_from_file(const std::string& filename)
{
	GError *err=NULL;
	GdkPixbuf *res=gdk_pixbuf_new_from_file(filename.c_str(), &err);
	if (!res) {
		g_error(_("Can not load image. %s"), err->message);
		g_error_free(err);
	}

	return res;
}

static gchar * byte_to_hex(unsigned char nr) {
	gchar *result = NULL;

	result = g_strdup_printf("%%%x%x", nr / 0x10, nr % 0x10);
	return result;
}

char *common_encode_uri_string(const char *string)
{
	gchar		*newURIString;
	gchar		*hex, *tmp = NULL;
	int		i, j, len, bytes;

	/* the UTF-8 string is casted to ASCII to treat
	   the characters bytewise and convert non-ASCII
	   compatible chars to URI hexcodes */
	newURIString = g_strdup("");
	len = strlen(string);
	for(i = 0; i < len; i++) {
		if(g_ascii_isalnum(string[i]) || strchr("-_.!~*'()", (int)string[i]))
		   	tmp = g_strdup_printf("%s%c", newURIString, string[i]);
		else if(string[i] == ' ')
			tmp = g_strdup_printf("%s%%20", newURIString);
		else if((unsigned char)string[i] <= 127) {
			tmp = g_strdup_printf("%s%s", newURIString, hex = byte_to_hex(string[i]));g_free(hex);
		} else {
			bytes = 0;
			if(((unsigned char)string[i] >= 192) && ((unsigned char)string[i] <= 223))
				bytes = 2;
			else if(((unsigned char)string[i] > 223) && ((unsigned char)string[i] <= 239))
				bytes = 3;
			else if(((unsigned char)string[i] > 239) && ((unsigned char)string[i] <= 247))
				bytes = 4;
			else if(((unsigned char)string[i] > 247) && ((unsigned char)string[i] <= 251))
				bytes = 5;
			else if(((unsigned char)string[i] > 247) && ((unsigned char)string[i] <= 251))
				bytes = 6;
				
			if(0 != bytes) {
				if((i + (bytes - 1)) > len) {
					g_warning(("Unexpected end of character sequence or corrupt UTF-8 encoding! Some characters were dropped!"));
					break;
				}

				for(j=0; j < (bytes - 1); j++) {
					tmp = g_strdup_printf("%s%s", newURIString, hex = byte_to_hex((unsigned char)string[i++]));
					g_free(hex);
					g_free(newURIString);
					newURIString = tmp;
				}
				tmp = g_strdup_printf("%s%s", newURIString, hex = byte_to_hex((unsigned char)string[i]));
				g_free(hex);
			} else {
				/* sh..! */
				g_error("Internal error while converting UTF-8 chars to HTTP URI!");
			}
		}
		g_free(newURIString); 
		newURIString = tmp;
	}
	return newURIString;
}

bool file_name_to_utf8(const std::string& str, std::string& out)
{
	size_t len = str.length();
	gsize bytes_read, bytes_written;
	glib::CharStr gstr(g_filename_to_utf8(str.c_str(), len, &bytes_read, 
		&bytes_written, NULL));
	if(!gstr || bytes_read != len) {
		g_error("Unable to convert string %s into utf-8 encoding", str.c_str());
		return false;
	}
	out = get_impl(gstr);
	return true;
}

bool utf8_to_file_name(const std::string& str, std::string& out)
{
	size_t len = str.length();
	gsize bytes_read, bytes_written;
	glib::CharStr gstr(g_filename_from_utf8(str.c_str(), len, &bytes_read, 
		&bytes_written, NULL));
	if(!gstr || bytes_read != len) {
		g_error("Unable to convert utf8 string %s into file name encoding", str.c_str());
		return false;
	}
	out = get_impl(gstr);
	return true;
}

#ifdef _WIN32
bool utf8_to_windows(const std::string& str_utf8, std_win_string& out)
{
#ifdef UNICODE
	const int buf_size = MultiByteToWideChar(
		CP_UTF8, //__in   UINT CodePage,
		0, //__in   DWORD dwFlags,
		str_utf8.c_str(), //__in   LPCSTR lpMultiByteStr,
		-1, //__in   int cbMultiByte,
		NULL, // __out  LPWSTR lpWideCharStr,
		0 //__in   int cchWideChar
	);
	if(buf_size == 0) {
		g_warning("Unable to convert from utf-8 to windows encoding. String: %s",
			str_utf8.c_str());
		return false;
	}
	std::vector<TCHAR> buf(buf_size);
	const int char_num = MultiByteToWideChar(
		CP_UTF8, //__in   UINT CodePage,
		0, //__in   DWORD dwFlags,
		str_utf8.c_str(), //__in   LPCSTR lpMultiByteStr,
		-1, //__in   int cbMultiByte,
		&buf[0], // __out  LPWSTR lpWideCharStr,
		buf_size //__in   int cchWideChar
	);
	if(char_num != buf_size) {
		g_warning("Unable to convert from utf-8 to windows encoding. String: %s",
			str_utf8.c_str());
		return false;
	}
	out = &buf[0];
	return true;
#else
	glib::Error err;
	gchar* tmp = g_locale_from_utf8(str_utf8.c_str(), -1, NULL, NULL, get_addr(err));
	if(!tmp) {
		g_warning("Unable to convert from utf-8 to windows encoding: %s", err->message);
		return false;
	}
	out = tmp;
	g_free(tmp);
	return true;
#endif
}

bool windows_to_utf8(const std_win_string& str, std::string& out_utf8)
{
#ifdef UNICODE
	const int buf_size = WideCharToMultiByte(
		CP_UTF8, // __in   UINT CodePage,
		0, // __in   DWORD dwFlags,
		str.c_str(), // __in   LPCWSTR lpWideCharStr,
		-1, // __in   int cchWideChar,
		NULL, // __out  LPSTR lpMultiByteStr,
		0, // __in   int cbMultiByte,
		NULL, // __in   LPCSTR lpDefaultChar,
		NULL //__out  LPBOOL lpUsedDefaultChar
	);
	if(buf_size == 0) {
		g_warning("Unable to convert from windows encoding to utf-8.");
		return false;
	}
	std::vector<char> buf(buf_size);
	const int char_num = WideCharToMultiByte(
		CP_UTF8, // __in   UINT CodePage,
		0, // __in   DWORD dwFlags,
		str.c_str(), // __in   LPCWSTR lpWideCharStr,
		-1, // __in   int cchWideChar,
		&buf[0], // __out  LPSTR lpMultiByteStr,
		buf_size, // __in   int cbMultiByte,
		NULL, // __in   LPCSTR lpDefaultChar,
		NULL //__out  LPBOOL lpUsedDefaultChar
	);
	if(char_num != buf_size) {
		g_warning("Unable to convert from windows encoding to utf-8.");
		return false;
	}
	out_utf8 = &buf[0];
	return true;
#else
	glib::Error err;
	gchar* tmp = g_locale_to_utf8(str.c_str(), -1, NULL, NULL, get_addr(err));
	if(!tmp) {
		g_warning("Unable to convert from windows encoding to utf-8: %s", err->message);
		return false;
	}
	out_utf8 = tmp;
	g_free(tmp);
	return true;
#endif
}

/* Returns a pointer to the first char after the root component.
If str is like "c:\path\...", root_end points after the "c:\".
If str is like "\\server\path\...", root_end points after the "\\server\".
If str is like "\\server", root_end points after the "\\server".
If str is like "\dir\dir", root_end points after the "\".
Otherwise the str is considered to have no root element and root_end points 
to the beginning of the string. 
The function returns NULL if the path is invalid.
T is either "char" or "const char". */
template<class T>
T* path_root_end_win(T* str)
{
	if(!str)
		return NULL;
	if(g_ascii_isalpha(str[0]) && str[1] == ':' && str[2] == '\\')
		return str + 3;
	else if(str[0] == '\\' && str[1] == '\\') {
		if(str[2] == '\0') // "\\" - invalid path
			return NULL;
		char* p = strchr(str+2, '\\');
		if(p) {
			if(p == str+2) // "\\\..." - empty server - invalid path
				return NULL;
			return p + 1;
		} else { // str is "\\server"
			return strchr(str, '\0');
		}
	} else if(str[0] == '\\' && str[1] != '\\') {
		return str + 1;
	}
	return str;
}

/* The same as path_root_end_win but for wide chars */
template<class T>
T* path_root_end_winW(T* str)
{
	if(!str)
		return NULL;
	if(is_ascii_alpha(str[0]) && str[1] == L':' && str[2] == L'\\')
		return str + 3;
	else if(str[0] == L'\\' && str[1] == L'\\') {
		if(str[2] == L'\0') // "\\" - invalid path
			return NULL;
		T* p = StrChr(str+2, L'\\');
		if(p) {
			if(p == str+2) // "\\\..." - empty server - invalid path
				return NULL;
			return p + 1;
		} else { // str is "\\server"
			return StrChr(str, L'\0');
		}
	} else if(str[0] == L'\\' && str[1] != L'\\') {
		return str + 1;
	}
	return str;
}

/* normalize path - resolve relative components in a path.
For example, path "c:\dir1\dir2\..\file" is converted to "c:\dir1\file".
This function accepts the following paths:
- an absolute path starting with disk name: "c:\", "c:\file", "c:\dir\file", ... 
	("c:" is not allowed)
- an absolute path without disk: "\dir\file", ...
- UNC name: "\\server", "\\server\dir", ...
- relative path: "dir", "dir\dir\file", ...
##- a relative path starting with the current directory component ".": ".\", ".\dir", ...

A reference to the parent of the root directory is considered an error.
For example, these paths are considered invalid: "c:\..\dir1\file", 
"\\..\path\file", "\..\dir". 
If the path is relative, this function may leave references to the parent directory 
if they cannot be resolved in the path given. 
For example, "dir\..\..\..\dir2\dir3" is converted to "..\..\dir2\dir3".
Strip "." components.

If after all transformations we get an empty string,
replace it with the current directory reference, that is '.'.
Empty string is not a valid path.
For example, we get an empty path for "abcd\.." and "abcd\..\".
If the original path is not blank and it ends on backslash, 
append backslash to the '.'. That is:
"abcd\.." -> "."
"abcd\..\' -> ".\"

Return value: EXIT_FAILURE or EXIT_SUCCESS. */
int norm_path_win(const std::string& path, std::string& result)
{
	result.clear();
	/* std::vector will free the allocated memory block
	when this function returns.
	+ 3 - make sure that the buffer contains at least 3 chars,
	that prevents buffer overread in the some checks.
	+ 1 - terminating '\0' */
	std::vector<char> buf(path.length() + 3 + 1);
	char* str = &buf[0];
	// end of string  - terminating '\0'
	char* str_end = g_stpcpy(str, path.c_str());
	char* root_end = path_root_end_win(str);
	if(!root_end)
		return EXIT_FAILURE;
	/*
	if(root_end == str && str[0] == '.' && (str[1] == '\\' || str[1] == '\0')) {
		if(str[1] == '\0')
			str += 1;
		else
			str += 2;
		root_end = str;
	}
	*/
	// if(str == root_end) - relative path
	/*p1 and p2 points to the first char of a path component,
	the previous char is normally '\\'.
	In each step p2 moves to the next path component.
	p1 normally moves forward as well, unless a parent directory reference
	is encontered, then p1 moves back. */
	char * p1 = root_end;
	char * p2 = root_end;
	while(p2 < str_end) {
		char *p = strchr(p2, '\\');
		if(!p)
			p = str_end;
		// [p2, p) - path component
		if(p == p2) // empty path component - error
			return EXIT_FAILURE;
		if(p2[0] == '.' && p2[1] == '.' && p2 + 2 == p) { // parent directory
			if(p1 == root_end) { // no component to strip
				if(str == root_end) { // relative path
					if(p1 != p2) {
						p1[0] = '.';
						p1[1] = '.';
						p1[2] = *p;
					}
					size_t len = p + 1 - p2;
					p1 += len;
					p2 += len;
				} else { // absolute path
					return EXIT_FAILURE; // error
				}
			} else { // search a component to strip
				char *p3 = strrchr_len(root_end, p1 - 1 - root_end, '\\');
				if(!p3)
					p3 = root_end;
				else
					++p3;
				// p3 - beginning of the privious to p1 path component
				if(p3[0] == '.' && p3[1] == '.' && p3[2] == '\\') {
					g_assert(str == root_end);
					// the previous component is "..", it cannot be stripped
					if(p1 != p2) {
						p1[0] = '.';
						p1[1] = '.';
						p1[2] = *p;
					}
					size_t len = p + 1 - p2;
					p1 += len;
					p2 += len;
				} else {
					p1 = p3;
					p2 = p + 1;
				}
			}
		} else if(p2[0] == '.' && p2 + 1 == p) { // strip "." component
			p2 = p + 1;
		} else { // normal directory
			if(p1 == p2) {
				p1 = p2 = p + 1;
			} else {
				size_t len = p + 1 - p2;
				strncpy(p1, p2, len);
				p1 += len;
				p2 += len;
			}
		}
	}
	/* p1[-1] == '\0' if the last char of the path is not '\\' */
	*p1 = '\0';
	if(str[0] == '\0') { // blank path
		str[0] = '.';
		if(!path.empty() && path[path.length()-1] == '\\') {
			str[1] = '\\';
			str[2] = '\0';
		} else
			str[1] = '\0';
	}
	result = str;
	return EXIT_SUCCESS;
}


/* returns true if the path is absolute and false otherwise,
This function does not check that the path is valid
The following paths are accepted:
- an absolute path starting with disk name: "c:\", "c:\file", "c:\dir\file", ... 
	("c:" is not allowed)
- an "absolute" path without disk: "\dir\file", ... - this path is considered relative!
- UNC name: "\\server", "\\server\dir", ...
-*/
bool is_absolute_path_win(const std::string& path)
{
	const char* str = path.c_str();
	if(g_ascii_isalpha(str[0]) && str[1] == ':' && str[2] == '\\')
		return true;
	if(str[0] == '\\' && str[1] == '\\')
		return true;
	return false;
}

/* applies a number of tests to the path 
Returns true if all tests passed and false otherwise. */
bool is_valid_path_win(const std::string& path)
{
	const char* str = path.c_str();
	/* End of the path prefix.
	if "c:\abcd" then after "c:\"
	if "\\abcd" then after "\\" 
	if "\abd" the after "\" 
	otherwise this the first char of the string. */
	const char* prefix_end = str;
	if(g_ascii_isalpha(str[0]) && str[1] == ':' && str[2] == '\\')
		prefix_end = str + 3;
	else if(str[0] == '\\' && str[1] == '\\')
		prefix_end = str + 2;
	else if(str[0] == '\\')
		prefix_end = str + 1;
	if(prefix_end[0] == '\\')
		return false;
	if(strstr(prefix_end, "\\\\"))
		return false;
	if(strlen(prefix_end) != strcspn(prefix_end, "<>:\"/|?*"))
		return false;
	for(const char* p = prefix_end; *p; ++p)
		if((unsigned char)*p < 32)
			return false;
	return true;
}

/* create a relative path from directory base_dir to file or dir path 
base_dir and path must have a common prefix, for example,
"c:\dir1\dir2" and "c:\dir1\dir3\dir4" -> "..\dir3\dir4" 
Return value: EXIT_FAILURE or EXIT_SUCCESS. 

PathRelativePathTo fuction gives strange results:
"c:\\dir", "c:\\dir", "..\\dir",
"c:\\dir\\", "c:\\dir", "..\\dir"
"c:\\dir\\", "c:\\dir\\", "",
"\\", "\\a\\", - fails!

That is why I've decided to provide a custom implementation.
base_dir and path must be absolute paths!
*/
#if 0
int build_relative_path(const std::string& base_dir, const std::string& path, std::string& rel_path)
{
	rel_path.clear();
	std_win_string base_dir_win;
	std_win_string path_win;
	if(!utf8_to_windows(base_dir, base_dir_win))
		return EXIT_FAILURE;
	if(!utf8_to_windows(path, path_win))
		return EXIT_FAILURE;
	if(base_dir_win.length() >= MAX_PATH)
		return EXIT_FAILURE;
	if(path_win.length() >= MAX_PATH)
		return EXIT_FAILURE;
	/* The output buffer must be at least MAX_PATH chars.
	How much space do we actually need? */
	wchar_t buf[MAX_PATH * 10];
	bool is_file = !path.empty() && path[path.length()-1] != '\\';
	if(!PathRelativePathToW(buf, base_dir_win.c_str(), FILE_ATTRIBUTE_DIRECTORY,
		path_win.c_str(), is_file ? 0 : FILE_ATTRIBUTE_DIRECTORY))
		return EXIT_FAILURE;
	wchar_t * buf2 = buf;
	if(buf[0] == L'.' && buf[1] == L'\\')
		buf2 = buf + 2;
	else if(buf[0] == L'.' && buf[1] == L'\0')
		buf2 = buf + 1;
	if(!windows_to_utf8(buf2, rel_path))
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
#endif

int build_relative_path(const std::string& base_dir, const std::string& path, std::string& rel_path)
{
	rel_path.clear();
	std_win_string base_dir_win;
	std_win_string path_win;
	if(!utf8_to_windows(base_dir, base_dir_win))
		return EXIT_FAILURE;
	if(!utf8_to_windows(path, path_win))
		return EXIT_FAILURE;
	if(base_dir_win.empty())
		return EXIT_FAILURE;
	if(path_win.empty())
		return EXIT_FAILURE;
	/* Make sure that both paths end with a backslash, that simplifies further processing.
	base_dir must be a directory, so adding a backslash won't hurt.
	path may be either a file or a directory */
	if(base_dir_win[base_dir_win.length()-1] != L'\\')
		base_dir_win += L'\\';
	if(path_win[path_win.length()-1] != L'\\')
		path_win += L'\\';
	const wchar_t* c_base_dir_win = base_dir_win.c_str();
	const wchar_t* c_path_win = path_win.c_str();
	const wchar_t* base_dir_win_root_end = path_root_end_winW(c_base_dir_win);
	const wchar_t* path_win_root_end = path_root_end_winW(c_path_win);
	if(!base_dir_win_root_end || base_dir_win_root_end == c_base_dir_win)
		return EXIT_FAILURE;
	if(!path_win_root_end || path_win_root_end == c_path_win)
		return EXIT_FAILURE;
	if(base_dir_win_root_end - c_base_dir_win != path_win_root_end - c_path_win)
		return EXIT_FAILURE; // different roots
	if(StrCmpNI(c_base_dir_win, c_path_win, base_dir_win_root_end - c_base_dir_win))
		return EXIT_FAILURE; // different roots
	/* p and q points to the end of the common part in base_dir_win and path_win respectively. */
	const wchar_t* p = base_dir_win_root_end;
	const wchar_t* q = path_win_root_end;
	while(true)
	{
		const wchar_t* p2 = StrChr(p, L'\\');
		const wchar_t* q2 = StrChr(q, L'\\');
		if(!p2 || !q2)
			break;
		p2++;
		q2++;
		if(p2 - p != q2 - q)
			break;
		if(StrCmpNI(p, q, p2-p))
			break;
		p = p2;
		q = q2;
	}
	// found the longest common part
	/* calculate how many directories to strip from the base_dir 
	== number of backslashes after p */
	int parent_cnt = 0;
	for(const wchar_t* r = StrChr(p, L'\\'); r; r = StrChr(r+1, L'\\'))
		++parent_cnt;
	std_win_string rel_path_win;
	rel_path_win.reserve(3 * parent_cnt + wcslen(q));
	for(int i=0; i<parent_cnt; ++i)
		rel_path_win.append(L"..\\");
	rel_path_win.append(q);
	/* What to do with the terminating backslash?
	Let make the relative path have the same ending as the path has.
	If rel_path is empty (this may occur if base_dir and path are indentical 
	or differ in the terminating backslash only), leave it as is. */
	if(!rel_path_win.empty()) {
		if(rel_path_win[rel_path_win.length()-1] == L'\\' && path[path.length() - 1] != '\\')
			rel_path_win.resize(rel_path_win.length()-1);
		else if(rel_path_win[rel_path_win.length()-1] != L'\\' && path[path.length() - 1] == '\\')
			rel_path_win += L'\\';
	}
	if(!windows_to_utf8(rel_path_win, rel_path))
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}

#endif // #ifdef _WIN32

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

/* concatenate path1 and path2 inserting a path separator in between if needed. */
std::string build_path(const std::string& path1, const std::string& path2)
{
	std::string res;
	res.reserve(path1.length() + 1 + path2.length());
	res = path1;
	if(!res.empty() && res[res.length()-1] != G_DIR_SEPARATOR)
		res += G_DIR_SEPARATOR_S;
	if(!path2.empty() && path2[0] == G_DIR_SEPARATOR)
		res.append(path2, 1, std::string::npos);
	else
		res.append(path2);
	return res;
}

static const char* html_entrs[] =     { "lt;", "gt;", "amp;", "apos;", "quot;", 0 };
static const int html_entrs_len[] =   { 3,     3,     4,      5,       5,       0 };
static const char html_raw_entrs[] =  { '<',   '>',   '&',    '\'',    '\"',    0 };

static const char* html_tags[] =     { "br>", 0 };
static const int html_tags_len[] =   { 3,     0 };
static const char html_raw_tags[] =  { '\n',  0 };

void html_decode(const char *str, std::string& decoded)
{
	decoded.clear();
	decoded.reserve(strlen(str));
	int ind;

	const char* p = str;
	while (*p)
		if (*p == '&') {
			if(*(p+1) == '#') {
				const char *q = strchr(p+2, ';');
				if(q) {
					long code = atol(p+2);
					char buf[10]; // must be at least 6 bytes long
					buf[g_unichar_to_utf8(gunichar(code), buf)] = '\0';
					decoded += buf;
					p = q + 1;
				} else {
					g_debug("unknown entry %s", p);
					break;
				}
			} else {
				for(ind = 0; html_entrs[ind] != 0; ++ind)
					if(strncmp(p + 1, html_entrs[ind], html_entrs_len[ind]) == 0) {
						decoded += html_raw_entrs[ind];
						p += html_entrs_len[ind]+1;
						break;
					}
				if (html_entrs[ind] == 0) { // unrecognized sequence
					const char *q = strchr(p+1, ';');
					if(q) {
						++q;
						g_debug("unknown entry %s", std::string(p, q-p).c_str());
						p = q;
					} else {
						g_debug("unknown entry %s", p);
						break;
					}
				}
			}
		} else if(*p == '<') {
			for(ind = 0; html_tags[ind] != 0; ++ind)
				if(strncmp(p + 1, html_tags[ind], html_tags_len[ind]) == 0) {
					decoded += html_raw_tags[ind];
					p += html_tags_len[ind]+1;
					break;
				}
			if (html_tags[ind] == 0) { // unrecognized sequence
				const char *q = strchr(p+1, '>');
				if(q) {
					++q;
					g_debug("unknown tag %s", std::string(p, q-p).c_str());
					p = q;
				} else {
					g_debug("unknown tag %s", p);
					break;
				}
			}
		} else {
			decoded += *p++;
		}
}

/* extract first pure english word (or a sequence of words separated by spaces)
	src in utf-8,
	dst must point to a buffer of size enough to hold the src string */
void GetPureEnglishAlpha(char *dst, const char *src)
{
	while(*src && (uchar(*src) >= 128 || !isalpha(*src)))
		src++;
	while(*src && uchar(*src) < 128 && (isalpha(*src) || *src == ' '))
		*dst++ = *src++;
	*dst = '\0';
}

bool IsASCII(const char *str)
{
	// works with UTF-8 strings (bytes of a multi-byte character has the highest 
	// bit set)
	for(; *str; ++str)
		if(uchar(*str) >= 128)
			return false;
	return true;
}

/* returns pointer to the first non-space unichar in the string */
const char* skip_spaces(const char *str)
{
	while(*str && g_unichar_isspace(g_utf8_get_char(str)))
		str = g_utf8_next_char(str);
	return str;
}

/* copy src to dst converting adjacent unicode spaces into one ASCII space ' ' 
	return value - new '\0' character */
char* copy_normalize_spaces(char *dst, const char *src)
{
	while(*src) {
		if (g_unichar_isspace(g_utf8_get_char(src))) {
			*dst++=' ';
			src = g_utf8_next_char(src);
			while(g_unichar_isspace(g_utf8_get_char(src)))
				src = g_utf8_next_char(src);
		} else {
			g_utf8_strncpy(dst,src,1);
			src = g_utf8_next_char(src);
			dst = g_utf8_next_char(dst);
		}
	}
	*dst='\0';
	return dst;
}

/* copy src to dst converting adjacent unicode spaces into one ASCII space ' ',
	remove leading and trailing unicode spaces */
void copy_normalize_trim_spaces(char *dst, const char *src)
{
	src = skip_spaces(src);
	char* end = copy_normalize_spaces(dst, src);
	delete_trailing_spaces_ASCII(dst, end);
}

/* delete trailing ASCII spaces. 
begin - beginning of the string, end - '\0' character 
return value - new '\0' character */
char* delete_trailing_spaces_ASCII(const char *begin, char *end)
{
	while(begin < end && *(end - 1) == ' ')
		--end;
	*end = '\0';
	return end;
}

/* delete the last word separated by ASCII space
In other word we replace the last ASCII space with '\0'.
begin - beginning of the string, end - '\0' character 
return value - new '\0' character */
char* delete_trailing_word_ASCII(const char *begin, char *end)
{
	while(begin < end && *(end-1) != ' ')
		--end;
	if(begin < end)
		--end;
	*end = '\0';
	return end;
}

/* return value - new '\0' character */
char* delete_trailing_char(char *begin, char *end)
{
	char* p = g_utf8_find_prev_char(begin, end);
	if(p) {
		*p = '\0';
		return p;
	} else {
		*begin = '\0';
		return begin;
	}
}

/* like extract_word but does not copy the extracted word
instead assignes begin and end to the first and next to last chars in the source string */
void extract_word_in_place(const char **begin, const char **end, const char* src, 
	int BeginPos, gboolean (*is_splitter)(gunichar c))
{
	g_assert(begin);
	g_assert(end);
	g_assert(BeginPos >= 0);
	*begin = *end = NULL;
	const char* const pointer = src + BeginPos;
	const char *word_begin = NULL;
	while(true) {
		while(*src && is_splitter(g_utf8_get_char(src)))
			src = g_utf8_next_char(src);
		if(!*src)
			break;
		if(src <= pointer || !word_begin)
			word_begin = src;
		if(pointer <= src)
			break;
		while(*src && !is_splitter(g_utf8_get_char(src)))
			src = g_utf8_next_char(src);
		if(!*src)
			break;
	}
	if(!word_begin)
		return;
	src = word_begin;
	while(*src && !is_splitter(g_utf8_get_char(src))) {
		src = g_utf8_next_char(src);
	}
	*begin = word_begin;
	*end = src;
}

/* extract the word at position BeginPos
word separator chars are identified by is_splitter function
If BeginPos points to a splitter, get the word to the left of the point.
If there is no word to the left, get the word to the right of the point.
copy result into dst */
void extract_word(char *dst, const char* src, int BeginPos, gboolean (*is_splitter)(gunichar c))
{
	g_assert(BeginPos >= 0);
	const char *begin, *end;
	extract_word_in_place(&begin, &end, src, BeginPos, is_splitter);
	if(!begin || !end) {
		*dst = '\0';
		return;
	}
	const int num = end - begin;
	strncpy(dst, begin, num);
	dst[num] = '\0';
}

/* Extract a capitalized word
	the word must look like Word 
	where the first letter satisfies the is_first_letter function,
	the first letter must be followed by at least one letter satisfing the is_second_letter function */
void extract_capitalized_word_in_place(const char **begin, const char **end,
	const char* src, int BeginPos, 
	gboolean (*is_first_letter)(gunichar c), gboolean (*is_second_letter)(gunichar c))
{
	g_assert(begin);
	g_assert(end);
	g_assert(BeginPos >= 0);
	*begin = *end = NULL;
	const char* const pointer = src + BeginPos;
	const char *word_begin = NULL;
	while(true) {
		while(*src && !is_first_letter(g_utf8_get_char(src)))
			src = g_utf8_next_char(src);
		if(!*src)
			break;
		const char *cur_word_begin = src;
		src = g_utf8_next_char(src);
		if(!is_second_letter(g_utf8_get_char(src)))
			continue;
		if(cur_word_begin <= pointer || !word_begin)
			word_begin = cur_word_begin;
		if(pointer <= cur_word_begin)
			break;
		while(*src && is_second_letter(g_utf8_get_char(src)))
			src = g_utf8_next_char(src);
		if(!*src)
			break;
	}
	if(!word_begin)
		return;
	// skip the first letter
	src = g_utf8_next_char(word_begin);
	while(*src && is_second_letter(g_utf8_get_char(src))) {
		src = g_utf8_next_char(src);
	}
	*begin = word_begin;
	*end = src;
}

void extract_capitalized_word(char *dst, const char* src, int BeginPos, 
	gboolean (*is_first_letter)(gunichar c), gboolean (*is_second_letter)(gunichar c))
{
	g_assert(BeginPos >= 0);
	const char *begin, *end;
	extract_capitalized_word_in_place(&begin, &end, src, BeginPos, is_first_letter, is_second_letter);
	if(!begin || !end) {
		*dst = '\0';
		return;
	}
	const int num = end - begin;
	strncpy(dst, begin, num);
	dst[num] = '\0';
}

const char* find_first(const char* src, gboolean (*isfunc)(gunichar c))
{
	while(*src && !isfunc(g_utf8_get_char(src)))
		src = g_utf8_next_char(src);
	if(isfunc(g_utf8_get_char(src)))
		return src;
	else
		return NULL;
}

const char* find_first_not(const char* src, gboolean (*isfunc)(gunichar c))
{
	while(*src && isfunc(g_utf8_get_char(src)))
		src = g_utf8_next_char(src);
	if(isfunc(g_utf8_get_char(src)))
		return NULL;
	else
		return src;
}

gboolean is_space_or_punct(gunichar c)
{
	return g_unichar_isspace(c) || g_unichar_ispunct(c);
}

gboolean is_not_alpha(gunichar c)
{
	return !g_unichar_isalpha(c);
}

gboolean is_not_upper(gunichar c)
{
	return !g_unichar_isupper(c);
}

gboolean is_not_lower(gunichar c)
{
	return !g_unichar_islower(c);
}

/* convert str into a valid utf8 string
We assume that str is a utf8-encoded string possibly containing invalid chars.
We replace invalid chars with '?' */
std::string fix_utf8_str(const std::string& str)
{
	std::string out;
	// an utf8 encoded char occupies at most 6 bytes + 1 byte for terminating '\0'
	char buf[7];
	out.reserve(str.length());
	const char* p = str.c_str();
	gunichar uch;
	while(*p) {
		uch = g_utf8_get_char_validated(p, -1);
		if(uch == (gunichar)-1 || uch == (gunichar)-2 || !g_unichar_validate(uch))
			out += '?';
		else {
			buf[g_unichar_to_utf8(uch, buf)] = '\0';
			out += buf;
		}
		++p;
		// locate the first byte of a character
		while((((unsigned char)*p) & 0xC0) == 0x80)
			++p;
	}
	return out;
}

char* strrchr_len(char* str, size_t size, char c)
{
	for(char *p = str + size - 1; str <= p; --p)
		if(*p == c)
			return p;
	return NULL;
}

bool is_ascii_alpha(wchar_t ch)
{
	static const wchar_t alphabet[] = 
		L"abcdefghijklmnopqrstuvwxyz"
		L"ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	for(int i=0; i<sizeof(alphabet)/sizeof(alphabet[0])-1; ++i)
		if(ch == alphabet[i])
			return true;
	return false;
}
