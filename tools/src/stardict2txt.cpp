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

#include <locale.h>
#include <glib.h>

#include "libstardict2txt.h"

int main(int argc,char * argv [])
{
	setlocale(LC_ALL, "");

	if (argc!=3) {
		g_print("Usage:\n./stardict2txt somedict.ifo output.txt\n");
	} else {
		convert_stardict2txt(argv[1], argv[2]);
	}
	return FALSE;
}
