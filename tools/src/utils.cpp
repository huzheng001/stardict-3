/*
 * This file part of makedict - convertor from any dictionary format to any
 * http://xdxf.sourceforge.net
 * Copyright (C) 2005-2006 Evgeniy <dushistov@mail.ru>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <cstring>
#include <cstdarg>
#include <cerrno>
#include <glib.h>
#include <glib/gi18n.h>
#include <glib/gstdio.h>

#include "utils.h"

bool make_directory(const std::string& dir)
{
	if (g_mkdir(dir.c_str(), 0700) && errno != EEXIST) {
		g_critical("Can not create %s: %s\n",
			      dir.c_str(), strerror(errno));
		return false;
	}
	return true;
}

void replace(const ReplaceStrTable& replace_table,
	     const char *str, std::string& res)
{
	res.resize(0);//may be this helps to clear string without freeing memory
	const char *p=str;

	while (*p) {
		const char *beg=p;
		ReplaceStrTable::const_iterator i;
		for (i=replace_table.begin(); i!=replace_table.end(); ++i) {
			p=beg;
			const char *q=i->first;
			while (*p && *q && *p==*q)
				++p, ++q;

			if (*q=='\0') {
				res+=i->second;
				break;
			}
		}
		if (i==replace_table.end()) {
			p=beg;
			res+=*p++;
		}
	}
}

StringList split(const std::string& str, char sep)
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

void strip(std::string& str)
{
	std::string::size_type i;
	for (i=str.length(); i>0 && g_ascii_isspace(str[i-1]); --i)
		;
	str.resize(i);
	for (i=0; i<str.length() && g_ascii_isspace(str[i]); ++i)
		;
	str.erase(0, i);
}

#define XX 100

static int b64_index[256] = {
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,62, XX,XX,XX,63,
    52,53,54,55, 56,57,58,59, 60,61,XX,XX, XX,XX,XX,XX,
    XX, 0, 1, 2,  3, 4, 5, 6,  7, 8, 9,10, 11,12,13,14,
    15,16,17,18, 19,20,21,22, 23,24,25,XX, XX,XX,XX,XX,
    XX,26,27,28, 29,30,31,32, 33,34,35,36, 37,38,39,40,
    41,42,43,44, 45,46,47,48, 49,50,51,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
};

guint32 b64_decode(const char *val)
{
  guint32 v = 0;
  int i;
  int offset = 0;
  int len = strlen( val );

  for (i = len - 1; i >= 0; i--) {
    int tmp = b64_index[ (unsigned char)val[i] ];
#if 0
    if (tmp == XX)
      err_internal( __FUNCTION__,
      "Illegal character in base64 value: `%c'\n", val[i] );
#endif    
    v |= tmp << offset;
    offset += 6;
  }

  return v;
}

static unsigned char b64_list[] =
"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";


/* |b64_encode| encodes |val| in a printable base 64 format.  A MSB-first
   encoding is generated. */

const char *b64_encode(guint32 val, char result[7])
{
//   static char   result[7];
   int    i;

   result[0] = b64_list[ (val & 0xc0000000) >> 30 ];
   result[1] = b64_list[ (val & 0x3f000000) >> 24 ];
   result[2] = b64_list[ (val & 0x00fc0000) >> 18 ];
   result[3] = b64_list[ (val & 0x0003f000) >> 12 ];
   result[4] = b64_list[ (val & 0x00000fc0) >>  6 ];
   result[5] = b64_list[ (val & 0x0000003f)       ];
   result[6] = 0;

   for (i = 0; i < 5; i++)
	   if (result[i] != b64_list[0])
		   return result + i;
   return result + 5;
}

bool copy_file(const std::string& from, const std::string& to)
{
	FILE* in = fopen(from.c_str(), "rb");
	if (!in) {
		g_critical("Can not open %s for reading\n", from.c_str());
		return false;
	}
	FILE* out = fopen(to.c_str(), "wb");
	if (!out) {
		g_critical("Can not open %s for writing\n", to.c_str());
		return false;
	}
	
	char buf[2048];
	size_t readb;

	do {
		readb = fread(buf, 1, sizeof(buf), in);
		if (ferror(in)) {
			g_critical("I/O error: can not read from stream\n");
			return false;
		}

		if (fwrite(buf, 1, readb, out) != readb) {
			g_critical("I/O error: can not write to stream\n");
			return false;
		}
	} while (readb == sizeof(buf));

	fclose(in);

	fclose(out);
	
	return true;
}
