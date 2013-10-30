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

#include <cstring>
#include <cstdlib>
#include <vector>
#include <sstream>
#include <glib.h>
#include <glib/gstdio.h>
#include "libcommon.h"
#ifdef _WIN32
#  include <Shlwapi.h>
#endif

const char* known_resource_types[] = {
	"img",
	"snd",
	"vdo",
	"att",
	NULL
};

gint stardict_strcmp(const gchar *s1, const gchar *s2)
{
	gint a;
	a = g_ascii_strcasecmp(s1, s2);
	if (a == 0)
		return strcmp(s1, s2);
	else
		return a;
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

/* performs case-insensitive comparision of two paths 
returns true if paths are equal */
bool is_equal_paths_win(const std::string& path1, const std::string& path2)
{
	std_win_string path1_win;
	std_win_string path2_win;
	if(!utf8_to_windows(path1, path1_win))
		return false;
	if(!utf8_to_windows(path2, path2_win))
		return false;
	return StrCmpI(path1_win.c_str(), path2_win.c_str()) == 0;
}

bool is_path_end_with_win(const std::string& path, const std::string& suff)
{
	std_win_string path_win;
	std_win_string suff_win;
	if(!utf8_to_windows(path, path_win))
		return false;
	if(!utf8_to_windows(suff, suff_win))
		return false;
	if(path_win.length() < suff_win.length())
		return false;
	size_t path_len = path_win.length();
	size_t suff_len = suff_win.length();
	return StrCmpI(path_win.substr(path_len - suff_len).c_str(), suff_win.c_str()) == 0;
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

int unpack_zlib(const char* arch_file_name, const char* out_file_name)
{
	zip::gzFile in(gzopen(arch_file_name, "rb"));
	if(!in) {
		g_critical("Unable to open archive file: %s.", arch_file_name);
		return EXIT_FAILURE;
	}
	const size_t buffer_size = 1024*1024;
	std::vector<char> buffer(buffer_size);
	char* buf = &buffer[0];
	gulong len;
	clib::File out_file(g_fopen(out_file_name, "wb"));
	if(!out_file) {
		g_critical(open_write_file_err, out_file_name);
		return EXIT_FAILURE;
	}
	while(true) {
		len = gzread(get_impl(in), buf, buffer_size);
		if(len < 0) {
			g_critical(read_file_err, arch_file_name, "");
			return EXIT_FAILURE;
		}
		if(len == 0)
			break;
		if(1 != fwrite(buf, len, 1, get_impl(out_file))) {
			g_critical(write_file_err, out_file_name);
			return EXIT_FAILURE;
		}
	}
	return EXIT_SUCCESS;
}

const std::string& TempFile::create_temp_file(void)
{
	clear();
	file_name = ::create_temp_file();
	if(file_name.empty())
		g_critical(create_temp_file_no_name_err);
	return file_name;
}

void TempFile::clear(void)
{
	if(!file_name.empty()) {
		if(g_remove(file_name.c_str()))
			g_warning(remove_temp_file_err, file_name.c_str());
		file_name.clear();
	}
}

std::string create_temp_file(void)
{
#ifdef _WIN32
	/* g_file_open_tmp does not work reliably on Windows
	Use platform specific API here. */
	{
		UINT uRetVal   = 0;
		DWORD dwRetVal = 0;
		TCHAR szTempFileName[MAX_PATH];
		TCHAR lpTempPathBuffer[MAX_PATH];
		dwRetVal = GetTempPath(MAX_PATH, lpTempPathBuffer);
		if (dwRetVal > MAX_PATH || (dwRetVal == 0))
			return "";

		uRetVal = GetTempFileName(lpTempPathBuffer, // directory for tmp files
			TEXT("temp"),     // temp file name prefix
			0,                // create unique name
			szTempFileName);  // buffer for name
		if (uRetVal == 0)
			return "";
		std::string tmp_url_utf8;
		std::string tmp_url;
		if(!windows_to_utf8(szTempFileName, tmp_url_utf8)
			|| !utf8_to_file_name(tmp_url_utf8, tmp_url))
			return "";
		FILE * f = g_fopen(tmp_url.c_str(), "wb");
		if(!f)
			return "";
		fwrite(" ", 1, 1, f);
		fclose(f);
		return tmp_url;
	}
#else
	{
		std::string tmp_url;
		gchar * buf = NULL;
		gint fd = g_file_open_tmp(NULL, &buf, NULL);
		if(fd == -1)
			return "";
		tmp_url = buf;
		g_free(buf);
		ssize_t write_size;
		write_size = write(fd, " ", 1);
		if (write_size == -1) {
			g_print("write error!\n");
		}
		close(fd);
		return tmp_url;
	}
#endif
}

bool is_known_resource_type(const char* str)
{
	for(size_t i=0; known_resource_types[i]; ++i)
		if(strcmp(str, known_resource_types[i]) == 0)
			return true;
	return false;
}

/* trim string src
 * new_beg is set to new beginning of the string
 * new_len length of the new string in bytes
 * The source string is not modified. */
void trim_spaces(const char* const src, const char*& new_beg, size_t& new_len)
{
	new_beg = src;
	while(*new_beg && g_unichar_isspace(g_utf8_get_char(new_beg))) {
		new_beg = g_utf8_next_char(new_beg);
	}
	const char* p = new_beg;
	const char* end = p;
	while(*p) {
		if(!g_unichar_isspace(g_utf8_get_char(p)))
			end = p;
		p = g_utf8_next_char(p);
	}
	if(*end)
		end = g_utf8_next_char(end);
	new_len = end - new_beg;
}

/* truncate utf8 string on char boundary (string content is not changed,
 * instead desired new length is returned)
 * new string length must be <= max_len
 * beg - first char of the string,
 * str_len - string length in bytes
 * return value: length of the truncated string */
size_t truncate_utf8_string(const char* const beg, const size_t str_len, const size_t max_len)
{
	if(str_len <= max_len)
		return str_len;
	if(max_len == 0)
		return 0;
	const char* char_end = beg+max_len;
	const char* p = beg+max_len-1;
	while(true) {
		// find the first byte of a utf8 char
		for(; beg <= p && (*p & 0xC0) == 0x80; --p)
			;
		if(p<beg)
			return 0;
		const gunichar guch = g_utf8_get_char_validated(p, char_end-p);
		if(guch != (gunichar)-1 && guch != (gunichar)-2)
			return char_end - beg;
		char_end = p;
		--p;
		if(p<beg)
			return 0;
	}
}


/* convert str into a valid utf8 string
We assume that str is a utf8-encoded string possibly containing invalid chars.
Replace invalid chars with replacement_char, or strip them if replacement_char == 0. */
std::string fix_utf8_str(const std::string& str, char replacement_char)
{
	std::string out;
	// an utf8 encoded char occupies at most 6 bytes + 1 byte for terminating '\0'
	char buf[7];
	out.reserve(str.length());
	const char* p = str.c_str();
	gunichar uch;
	while(p && *p) {
		uch = g_utf8_get_char_validated(p, -1);
		if(uch == (gunichar)-1 || uch == (gunichar)-2 || !g_unichar_validate(uch) || uch == 0) {
			if(replacement_char)
				out += replacement_char;
		} else {
			buf[g_unichar_to_utf8(uch, buf)] = '\0';
			out += buf;
		}
		p = g_utf8_find_next_char(p+1, NULL);
	}
	return out;
}

/* print a comma-separated list of Unicode character codes
 * chars - a list of Unicode utf-8 encoded characters to print */
std::string print_char_codes(const std::list<const char*>& chars) {
	std::stringstream buf;
	bool add_splitter = false;
	for(std::list<const char*>::const_iterator it = chars.begin(); it != chars.end(); ++it) {
		if(add_splitter)
			buf << ", ";
		buf << static_cast<unsigned long>(g_utf8_get_char(*it));
		add_splitter = true;
	}
	return buf.str();
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
	for(size_t i=0; i<sizeof(alphabet)/sizeof(alphabet[0])-1; ++i)
		if(ch == alphabet[i])
			return true;
	return false;
}

std::string get_basename_without_extension(const std::string& filepath)
{
	std::string::size_type pos = filepath.find_last_of(G_DIR_SEPARATOR);
	if(pos == std::string::npos)
		pos = 0;
	else
		pos += 1;
	if(pos >= filepath.length())
		return "";
	std::string::size_type pos2 = filepath.find_last_of('.');
	if(pos2 == std::string::npos || pos2 < pos)
		return filepath.substr(pos);
	return filepath.substr(pos, pos2-pos);
}

/* remove the item at path
 * if this is a regular file, removed the file;
 * if this is a symbolic line, remove the link;
 * if this is a directory, remove the directory recursively.
 * Return value: EXIT_SUCCESS or EXIT_FAILURE
 * */
int remove_recursive(const std::string& path)
{
	int res = EXIT_SUCCESS;
	if(g_file_test(path.c_str(),G_FILE_TEST_IS_DIR)) {
		// change file mode so we can read directory and remove items from it
		// If we cannot read mode or change it, go on, maybe we can remove the dir anyway.
		stardict_stat_t stats;
		if(!g_stat(path.c_str(), &stats)) {
			// full access for everyone
			g_chmod(path.c_str(), stats.st_mode | (S_IRWXU|S_IRWXG|S_IRWXO));
		}
		glib::Dir dir(g_dir_open(path.c_str(), 0, NULL));
		if(!dir)
			res = EXIT_FAILURE;
		else {
			std::string dirpath(path); // directory path ending with a dir separator
			if(dirpath[dirpath.length()-1] != G_DIR_SEPARATOR)
				dirpath += G_DIR_SEPARATOR;
			const gchar * filename;
			while((filename = g_dir_read_name(get_impl(dir)))) {
				if (strcmp(filename, ".") == 0 || strcmp(filename, "..") == 0)
					continue;
				const std::string itempath(dirpath + filename);
				if(remove_recursive(itempath.c_str()))
					res = EXIT_FAILURE;
			}
		}
		if(g_rmdir(path.c_str()))
			res = EXIT_FAILURE;
		return res;
	} else {
		if(g_remove(path.c_str()))
			res = EXIT_FAILURE;
		return res;
	}
}
