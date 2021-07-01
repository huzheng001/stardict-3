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
#include <vector>
#include <iostream>
#include <gtk/gtk.h>

#include "file-utils.h"
#include "utils.h"
#include "stddict.h"

int main(int argc, char *argv[])
{
	setlocale(LC_ALL, "");
	gtk_init(&argc, &argv);
	
	Libs libs(NULL, false, CollationLevel_NONE, COLLATE_FUNC_NONE);
	List dict_list;
	libs.load(dict_list);
	std::vector<InstantDictIndex> dictmask;
	size_t dictmask_size = dictmask.size();
	std::vector<gchar *> reslist[dictmask_size];
	if (libs.LookupData("letter", reslist, NULL, NULL, NULL, dictmask)) 
		return EXIT_SUCCESS;
	else
		return EXIT_FAILURE;
}
