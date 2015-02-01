/*
 * This file part of makedict - convertor from any dictionary format to any
 * http://xdxf.sourceforge.net
 * Copyright (C) 2006 Evgeniy <dushistov@mail.ru>
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

#include <cstdarg>
#include <glib/gi18n.h>

#include "file.h"

File StdIn(stdin, false);
File StdOut(stdin, false);

File& File::operator<<(const char *str)
{
	if (stream_)
		fputs(str, stream_);
	return *this;
}

File& File::operator<<(const std::string& str)
{
	if (stream_)
		fputs(str.c_str(), stream_);
	return *this;
}

void File::flush()
{
	if (stream_)
		fflush(stream_);
}

File& File::printf(const char *fmt, ...)
{	
	va_list ap;

	va_start(ap, fmt);
	if (stream_)
	{
		g_message(fmt);
	}
	va_end(ap);

	return *this;
}

File& File::getline(File& in, std::string& line)
{
	int ch;

	line = "";

	while ((ch = fgetc(in.stream_)) != EOF && ch != '\n')
		line += guchar(ch);

	return in;
}

File& operator<<(File& out, const Strip& st)
{
	const std::string& str = st.str_;
	size_t beg;
	for (beg = 0; beg < str.length() && g_ascii_isspace(str[beg]); ++beg)
		;
	size_t end;
	for (end = str.length(); end > beg && g_ascii_isspace(str[end - 1]); --end)
		;

	return out.write(str.c_str() + beg, end - beg);
}

File& File::write(const char *buf, size_t len)
{
	fwrite(buf, 1, len, stream_);
	return *this;
}
