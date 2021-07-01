/* 
 * Copyright (C) 2005 Evgeniy <dushistov@mail.ru>
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

#include <algorithm>
#include <ctime>
#include <iterator>
#include <iostream>
#include <cstdlib>
#include <numeric>
#include <sys/resource.h>
#include <vector>
#include <gtk/gtk.h>

#include "file-utils.h"
#include "utils.h"
#include "stddict.h"

static void fuzzy_lookup(Libs& libs, const gchar *s)
{
	std::vector<InstantDictIndex> dictmask;
	gchar *res[100];
	const size_t res_size=sizeof(res)/sizeof(gchar *);
	libs.LookupWithFuzzy(s, res, res_size, dictmask);
	std::for_each(res, res+res_size, g_free);
}

static double average_time(std::vector<double> &time_arr)
{
	if (time_arr.empty())
		return 0.;
	std::vector<double>::iterator it=
		max_element(time_arr.begin(), time_arr.end());
	time_arr.erase(it);
	if (time_arr.empty())
		return 0.;
	it=min_element(time_arr.begin(), time_arr.end());
	time_arr.erase(it);
	if (time_arr.empty())
		return 0.;
	return std::accumulate(time_arr.begin(), time_arr.end(), 0.)/time_arr.size();
}


int main(int argc, char *argv[])
{
	setlocale(LC_ALL, "");
	gtk_init(&argc, &argv);
	
	Libs libs(NULL, false, CollationLevel_NONE, COLLATE_FUNC_NONE);
	List dict_list;
	libs.load(dict_list);
	std::vector<double> times;

	for (int i=0; i<10; ++i) {
		clock_t t=clock();
		fuzzy_lookup(libs, "mather");
		fuzzy_lookup(libs, "try thes");
		fuzzy_lookup(libs, "wths up man?");
		fuzzy_lookup(libs, "faind fiz");
		fuzzy_lookup(libs, "u can not find?");
		fuzzy_lookup(libs, "starnge");
		t=clock()-t;
		times.push_back(double(t)/CLOCKS_PER_SEC);
	//	std::cout<<double(t)/CLOCKS_PER_SEC<<std::endl;
	}

	std::cout<<average_time(times)<<std::endl;	
	
	return EXIT_SUCCESS;
}
