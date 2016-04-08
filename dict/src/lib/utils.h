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

#ifndef UTILS_H
#define UTILS_H

#include <glib.h>
#include <string>
#include <vector>
#include <cstring>
#include <gdk-pixbuf/gdk-pixbuf.h>
#ifdef _WIN32
#include <windows.h>
#endif


#if defined(ARM) || defined(__sparc__)
static inline guint32 get_uint32(const gchar *addr)
{
	guint32 result;
	memcpy(&result, addr, sizeof(guint32));
	return result;
}
#else
#define get_uint32(x) *reinterpret_cast<const guint32 *>(x)
#endif

extern void ProcessGtkEvent();

extern std::string combnum2str(gint comb_code);
extern std::vector<std::string> split(const std::string& str, char sep);
extern GdkPixbuf *load_image_from_file(const std::string& filename);
extern char *common_encode_uri_string(const char *string);
extern char *common_build_dictdata(char type, const char *definition);

/* Create a new temporary file. Return file name in file name encoding.
Return an empty string if file cannot be created. */
std::string create_temp_file(void);
/* stardict_mkstemp_full, stardict_mkstemp, stardict_file_open_tmp
functions were created after g_mkstemp_full, g_mkstemp, g_file_open_tmp
respectively. Implementation is identical to glib functions besides 
on Windows platform new functions use native Windows API. 
In particular _wsopen_s is called instead of g_open on Windows. 
g_open works on Windows, but we cannot close the returned handle properly.
The handle must be closed with close() function from GTK+ CRL, 
but we have no access to it. We only have _close() from MS CRL, 
it cannot close the handler returned from other CRL. 
When StarDict is build with Visual Studio we have two copies of CRL:
one from Microsoft and one from GTK+ runtime. */
gint
stardict_mkstemp_full (gchar *tmpl,
	int    flags,
	int    mode);
gint
stardict_mkstemp (gchar *tmpl);
gint
stardict_file_open_tmp (const gchar  *tmpl,
				 gchar       **name_used,
				 GError      **error);
inline gchar* stardict_datadup(gconstpointer mem)
{
	return (gchar *)g_memdup(mem, sizeof(guint32) + *reinterpret_cast<const guint32 *>(mem));
}

typedef enum {
	qtSIMPLE, qtPATTERN, qtFUZZY, qtREGEX, qtFULLTEXT
} query_t;

extern query_t analyse_query(const char *s, std::string& res);
extern void stardict_input_escape(const char *text, std::string &res);


void html_decode(const char *str, std::string& decoded);
void GetPureEnglishAlpha(char *dst, const char *src); // not used
bool IsASCII(const char *str);
const char* skip_spaces(const char *str);
char* copy_normalize_spaces(char *dst, const char *src);
void copy_normalize_trim_spaces(char *dst, const char *src);
char* delete_trailing_spaces_ASCII(const char *begin, char *end);
char* delete_trailing_word_ASCII(const char *begin, char *end);
char* delete_trailing_char(char *begin, char *end);
void extract_word(char *dst, const char* src, int BeginPos, gboolean (*is_splitter)(gunichar c));
void extract_word_in_place(const char **begin, const char **end, 
	const char* src, int BeginPos, gboolean (*is_splitter)(gunichar c));
void extract_capitalized_word(char *dst, const char* src, int BeginPos, 
	gboolean (*is_first_letter)(gunichar c), gboolean (*is_second_letter)(gunichar c));
void extract_capitalized_word_in_place(const char **begin, const char **end,
	const char* src, int BeginPos, 
	gboolean (*is_first_letter)(gunichar c), gboolean (*is_second_letter)(gunichar c));
const char* find_first(const char* src, gboolean (*isfunc)(gunichar c));
const char* find_first_not(const char* src, gboolean (*isfunc)(gunichar c));
gboolean is_space_or_punct(gunichar c);
gboolean is_not_alpha(gunichar c);
gboolean is_not_upper(gunichar c);
gboolean is_not_lower(gunichar c);

#ifdef _WIN32
#define bzero(p, l) memset(p, 0, l)
#endif

typedef unsigned int uint;
typedef unsigned short  uint16; /* Short for unsigned integer >= 16 bits */
typedef unsigned long   ulong;            /* Short for unsigned long */
typedef unsigned long long int ulonglong; /* ulong or unsigned long long */
typedef long long int   longlong;
typedef char    pchar;          /* Mixed prototypes can take char */
typedef char    pbool;          /* Mixed prototypes can take char */
typedef unsigned char   uchar;  /* Short for unsigned char */
typedef char            my_bool; /* Small bool */

#endif/*UTILS_H*/
