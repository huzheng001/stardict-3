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

#ifndef _LIBCOMMON_H_
#define _LIBCOMMON_H_

#include <glib.h>
#include <string>
#include <vector>
#include <cstdio>
#include <list>
#include <zlib.h>
#ifdef _WIN32
#include <windows.h>
#endif

extern gint stardict_strcmp(const gchar *s1, const gchar *s2);
extern bool file_name_to_utf8(const std::string& str, std::string& out);
extern bool utf8_to_file_name(const std::string& str, std::string& out);
#ifdef _WIN32
typedef std::basic_string<TCHAR> std_win_string;
extern bool utf8_to_windows(const std::string& str_utf8, std_win_string& out);
extern bool windows_to_utf8(const std_win_string& str, std::string& out_utf8);
extern int norm_path_win(const std::string& path, std::string& result);
extern bool is_absolute_path_win(const std::string& path);
extern bool is_valid_path_win(const std::string& path);
extern int build_relative_path(const std::string& base_dir, const std::string& path, std::string& rel_path);
extern bool is_equal_paths_win(const std::string& path1, const std::string& path2);
extern bool is_path_end_with_win(const std::string& path, const std::string& suff);
#endif
inline
bool is_equal_paths(const std::string& path1, const std::string& path2)
{
#ifdef _WIN32
	return is_equal_paths_win(path1, path2);
#else
	return path1 == path2;
#endif
}
inline
bool is_path_end_with(const std::string& path, const std::string& suff)
{
#ifdef _WIN32
	return is_path_end_with_win(path, suff);
#else
	return g_str_has_suffix(path.c_str(), suff.c_str());
#endif
}
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
std::string build_path(const std::string& path1, const std::string& path2);

enum TLoadResult { lrOK, lrError, lrNotFound };

int unpack_zlib(const char* arch_file_name, const char* out_file_name);

/* allows to create a temporary file, remove the temporary file when the object is destroyed. */
class TempFile
{
public:
	TempFile(void)
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
	void clear(void);
private:
	std::string file_name;
};

template <class T>
class auto_executor_t
{
public:
	typedef void (T::*method_t)(void);
	auto_executor_t(T& obj, method_t method)
	:
		obj(obj),
		method(method)
	{

	}
	~auto_executor_t(void)
	{
		(obj.*method)();
	}
private:
	T& obj;
	method_t method;
};


template <typename T, typename unref_res_par_t, typename unref_res_ret_t,
	unref_res_ret_t (*unref_res)(unref_res_par_t)>
class ResourceWrapper {
public:
	ResourceWrapper(T *p = NULL) : p_(p) {}
	~ResourceWrapper() { free_resource(); }
	T *operator->() const { return p_; }
	bool operator!() const { return p_ == NULL; }

	void reset(T *newp) {
		if (p_ != newp) {
			free_resource();
			p_ = newp;
		}
	}

	friend inline T *get_impl(const ResourceWrapper& rw) {
		return rw.p_;
	}

	friend inline T **get_addr(ResourceWrapper& rw) {
		return &rw.p_;
	}
private:
	T *p_;

	void free_resource() { if (p_) unref_res(p_); }

// Helper for enabling 'if (sp)'
	struct Tester {
		Tester() {}
	private:
		void operator delete(void*);
	};

public:
// enable 'if (sp)'
	operator const Tester*() const
	{
		if (!*this) return 0;
		static Tester t;
		return &t;
	}
};

namespace glib {
	typedef ResourceWrapper<gchar, void*, void, g_free> CharStr;
	typedef ResourceWrapper<GError, GError*, void, g_error_free> Error;
	typedef ResourceWrapper<gchar *, gchar **, void, g_strfreev> CharStrArr;
	typedef ResourceWrapper<GOptionContext, GOptionContext*, void,
		g_option_context_free> OptionContext;
	typedef ResourceWrapper<GDir, GDir*, void, g_dir_close> Dir;
}

namespace clib {
	typedef ResourceWrapper<FILE, FILE*, int, fclose> File;
}

namespace zip {
#if ZLIB_VERNUM > 0x1250
typedef ResourceWrapper<gzFile_s, gzFile, int, gzclose> gzFile;
#else
typedef ResourceWrapper<void, void*, int, gzclose> gzFile;
#endif
}

/* Create a new temporary file. Return file name in file name encoding.
Return an empty string if file cannot be created. */
std::string create_temp_file(void);

extern const char* known_resource_types[];

bool is_known_resource_type(const char* str);

void trim_spaces(const char* const src, const char*& new_beg, size_t& new_len);
size_t truncate_utf8_string(const char* const beg, const size_t str_len, const size_t max_len);
std::string fix_utf8_str(const std::string& str, char replacement_char = '?');
std::string print_char_codes(const std::list<const char*>& chars);
char* strrchr_len(char* str, size_t size, char c);
bool is_ascii_alpha(wchar_t ch);
std::string get_basename_without_extension(const std::string& filepath);
int remove_recursive(const std::string& path);

#define UTF8_BOM "\xEF\xBB\xBF"

#define known_type_ids \
	"mtygxkwhnr"

#define file_not_found_err \
	"File does not exist: '%s'"
#define dir_not_found_err \
	"Directory does not exist: '%s'"
#define read_file_err \
	"Error reading file: '%s'. Error: %s."
#define write_file_err \
	"Error writing file: '%s'."
#define open_read_file_err \
	"Unable open file for reading: '%s'. Error: %s."
#define open_write_file_err \
	"Unable open file for writing: '%s'."
#define create_temp_file_err \
	"Unable to create a temporary file: '%s'."
#define create_temp_file_no_name_err \
	"Unable to create a temporary file."
#define remove_temp_file_err \
	"Unable to remove a temporary file: '%s'."
#define copy_file_err \
	"Error copying file from '%s' to '%s'. Error: %s"
#define create_dir_err \
	"Unable to create directory '%s'. Error: %s"
#define open_dir_err \
	"Unable to open directory '%s'. Error: %s"
#define incorrect_arg_err \
	"Incorrect argument."
#define fixed_ignore_msg \
	"The problem was fixed. Ignore the problem."
#define fixed_msg \
	"The problem was fixed."
#define fixed_msg2 \
	"The problem was fixed. "

/* Maximum size of word in index. strlen(word) < MAX_INDEX_KEY_SIZE.
 * See doc/StarDictFileFormat. */
const int MAX_INDEX_KEY_SIZE=256;

#endif

