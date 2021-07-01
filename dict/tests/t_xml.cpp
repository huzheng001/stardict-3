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

#include <glib.h>
#include <cstring>
#include <cstdlib>
#include <string>
#include <string.h>
#include "xml_str.h"


static bool decode(const char *probe, const char *exp)
{
	std::string res;
	xml_utf8_decode(probe, res);
	if (res != exp) {
		g_warning("want: %s, got %s", exp, res.c_str());
		return false;
	}
	return true;
}

int main()
{
	setlocale(LC_ALL, "");
	const char *probe[] = {
		"aa &lt; &gt;&amp; &apos; &quot; bb",
		"",
		"aaa bbb",
		"aa &lt; &gt;&amp; &apos; &quot; bb, &b; &quot;",
		NULL
	};
	const char *expect[] = {
		"aa < >& ' \" bb",
		"",
		"aaa bbb",
		"aa < >& ' \" bb, &b; \"",
	};

	for (int i = 0; probe[i]; ++i) 
		if (!decode(probe[i], expect[i]))
			return EXIT_FAILURE;

	return EXIT_SUCCESS;
}
