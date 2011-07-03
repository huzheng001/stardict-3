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

#include <cstdlib>
#include <cstdio>
#include <string>

#include <iterator>
#include <algorithm>
#include <iostream>

#include <glib.h>
#include <glib/gstdio.h>

#include "inifile.h"

static const char *tmpfname = "/tmp/stardict.cfg";

class TmpFile {
public:
	TmpFile(const std::string& fn) : fname_(fn) {}
	~TmpFile() { remove(fname_.c_str()); }
private:
	std::string fname_;
};

int main()
{
	setlocale(LC_ALL, "");
	remove(tmpfname);
	
	{
		TmpFile tf(tmpfname);
		inifile ini;
		bool res = ini.load(tmpfname);
		g_assert(res);
		std::string s;
		g_assert(ini.read_string("stardict-private", "version", s));
		g_assert(s == "1.0");
	}
	FILE * f = g_fopen(tmpfname, "wb");
	if (!f) {
		fprintf(stderr, "Can not open: %s\n", tmpfname);
		return EXIT_FAILURE;
	}
	guchar list[] = {'a', 0x01, 'b', 0x01, 'c', '\0' };
	g_fprintf(f, "[somegroup]\nsomevalue=%s\n", list);
	fclose(f);
	{
		//TmpFile tf(tmpfname);
		inifile ini;
		bool res = ini.load(tmpfname);
		g_assert(res);
		std::string s;
		g_assert(ini.read_string("stardict-private", "version", s));
		g_assert(s == "1.0");
		std::list<std::string> slist;
		g_assert(ini.read_strlist("somegroup", "somevalue", slist));
		std::list<std::string>::iterator it = slist.begin();
		for (int i = 0; i < 3; ++i, ++it) {
			g_assert(it != slist.end());
			g_assert(*it == std::string(1, char('a' + i)));
		}
	}
	return EXIT_SUCCESS;
}
