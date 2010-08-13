#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <cstring>
#include <cstdlib>
#include <vector>
#include <glib.h>
#include <glib/gstdio.h>
#include "libcommon.h"
#include "resourcewrap.hpp"

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
#endif

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

int unpack_zlib(const char* arch_file_name, const char* out_file_name, print_info_t print_info)
{
	zip::gzFile in(gzopen(arch_file_name, "rb"));
	if(!in) {
		print_info("Unable to open archive file %s\n", arch_file_name);
		return EXIT_FAILURE;
	}
	const size_t buffer_size = 1024*1024;
	std::vector<char> buffer(buffer_size);
	char* buf = &buffer[0];
	gulong len;
	clib::File out_file(g_fopen(out_file_name, "wb"));
	if(!out_file) {
		print_info("Unable to open file %s for writing\n", out_file_name);
		return EXIT_FAILURE;
	}
	while(true) {
		len = gzread(get_impl(in), buf, buffer_size);
		if(len < 0) {
			print_info(read_file_err, arch_file_name);
			return EXIT_FAILURE;
		}
		if(len == 0)
			break;
		if(1 != fwrite(buf, len, 1, get_impl(out_file))) {
			print_info(write_file_err, out_file_name);
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
		print_info("Unable to create a temporary file.\n");
	return file_name;
}

void TempFile::clear(void)
{
	if(!file_name.empty()) {
		if(g_remove(file_name.c_str()))
			print_info("Unable to remove temp file %s\n", file_name.c_str());
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
		write(fd, " ", 1);
		close(fd);
		return tmp_url;
	}
#endif
}

