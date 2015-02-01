/*
 * This file is part of makedict - convertor from any
 * dictionary format to any http://xdxf.sourceforge.net
 *
 * Copyright (C) Evgeniy Dushistov, 2005-2006
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

#include <cerrno>
#include <cstring>
#include <glib/gi18n.h>

#include "charset_conv.h"

//#define DEBUG

/* Checking WIN32 macro is not enough to determine type of the second argument
 * of the iconv function. Here I try to provide a generic solution to this
 * problem... */
namespace {
typedef char RT1;
typedef struct { char a[2]; } RT2;

template<typename RT, typename P1, typename P3, typename P4, typename P5>
RT1 iconv_func(RT (*)(P1, const char* *, P3, P4, P5));
template<typename RT, typename P1, typename P3, typename P4, typename P5>
RT2 iconv_func(RT (*)(P1,       char* *, P3, P4, P5));

template<bool s> struct iconv_traits;

template<>
struct iconv_traits<true>
{
	typedef const char** type;
};

template<>
struct iconv_traits<false>
{
	typedef char** type;
};

typedef iconv_traits<sizeof(iconv_func(iconv)) == 1>::type iconv_P2_type;
} // anonymous namespace

void CharsetConv::workwith(const char *from, const char *to)
{
	close();

	if ((cd_ = iconv_open(to, from)) == iconv_t(-1))
		g_critical("Can not convert from %s to %s: %s\n",
			      from, to, strerror(errno));
#ifdef DEBUG
	StdErr.printf("CharsetConv::workwith: %s to %s\n", from ,to);
#endif
}

bool CharsetConv::convert(const char *str, std::string::size_type len,
			  std::string& res) const
{
	res = "";
	if (cd_ == iconv_t(-1)) {
		g_critical("Can not convert from one encoding to another:"
			    " wrong iconv descriptor\n");
		return false;
	}

	size_t err;

	res.resize(len);
	const char *p = str;
	size_t inbytesleft = len;
	size_t outbytesleft = res.length();
//TODO: res may be not contiguous
	char *outp = &res[0];
again:
	err = iconv(cd_,
//this need because win32 version of iconv and from glibc is different
		    (iconv_P2_type)&p,
		    &inbytesleft, &outp, &outbytesleft);
	if (err == size_t(-1)) {
		if (errno == E2BIG) {
			size_t used = outp - &(res[0]);
			res.resize(res.length() * 2);
			outp = &res[0] + used;
			outbytesleft = res.length() - used;
			goto again;
		} else {
			g_critical("Can not convert from one encoding to another: %s\n",
				      strerror(errno));
			return false;
		}
	}	

	res.resize(outp - &res[0]);
	return true;
}
