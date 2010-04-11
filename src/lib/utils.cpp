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
#endif

#include "utils.h"


void ProcessGtkEvent()
{
  while (gtk_events_pending())
    gtk_main_iteration();
}

std::string get_user_config_dir()
{
        const gchar *config_path_from_env = g_getenv("STARDICT_CONFIG_PATH");
        if (config_path_from_env)
            return config_path_from_env;
#ifdef _WIN32
	std::string res = g_get_user_config_dir();
	res += G_DIR_SEPARATOR_S "StarDict";
	return res;
#else
	std::string res;
	gchar *tmp = g_build_filename(g_get_home_dir(), ".stardict", NULL);
	res=tmp;
	g_free(tmp);
	return res;
#endif
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
		 GtkWidget *message_dlg = 
			 gtk_message_dialog_new(
															NULL,
															(GtkDialogFlags)(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
															GTK_MESSAGE_ERROR,
															GTK_BUTTONS_OK,
															_("Can not load image. %s"), err->message);
    
    gtk_dialog_set_default_response(GTK_DIALOG(message_dlg), GTK_RESPONSE_OK);
    
    gtk_window_set_resizable(GTK_WINDOW(message_dlg), FALSE);
    
    gtk_dialog_run(GTK_DIALOG(message_dlg));
    gtk_widget_destroy(message_dlg);
		g_error_free(err);
		exit(EXIT_FAILURE);
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
