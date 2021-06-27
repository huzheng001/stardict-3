/*
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

#include <cstdio>
#include <locale.h>
#include <string.h>
#include <glib.h>

#include "libbabylonfile.h"

int main(int argc,char * argv [])
{
	if (argc<2) {
		printf("please type this:\n./babylon fundset.utf\n");
		return FALSE;
	}
	bool strip_html = true;
	for (int i=1; i < argc; i++) {
		if (strcmp(argv[i], "-n")==0) {
			strip_html = false;
			break;
		}
	}

	setlocale(LC_ALL, "");
	convert_babylonfile (argv[argc-1], strip_html);
	return FALSE;
}
