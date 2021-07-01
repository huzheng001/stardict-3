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
#include <utility>
#include <iostream>

#include "utils.h"

static inline bool 
test_analyse_query(const char *s, const std::pair<query_t, std::string>& res)
{
	std::string resstr;
	query_t qt=analyse_query(s, resstr);
	if (qt!=res.first || resstr!=res.second) {
		std::cerr<<"query: "<<s<<std::endl
						 <<"expected: type="<<res.first<<", result="<<res.second<<std::endl
						 <<"get: type="<<qt<<", result="<<resstr<<std::endl;
		return false;
	}

	return true;
}

static inline void
add_analyse_query_test(std::vector<std::pair<std::string, std::pair<query_t, std::string> > >& v, const std::string& s, query_t r, const std::string& rs)
{
	v.push_back(std::pair<std::string, std::pair<query_t, std::string> >(s, std::pair<query_t, std::string>(r, rs)));
}

int main()
{
	setlocale(LC_ALL, "");
	std::vector<std::pair<std::string, std::pair<query_t, std::string> > > aq_tests;

	add_analyse_query_test(aq_tests, "/fuzzy", qtFUZZY, "fuzzy");
	add_analyse_query_test(aq_tests, "|data", qtFULLTEXT, "data");
	add_analyse_query_test(aq_tests, "\\|data", qtSIMPLE, "|data");
	add_analyse_query_test(aq_tests, "\\/fuzzy", qtSIMPLE, "/fuzzy");
	add_analyse_query_test(aq_tests, "re?ex*", qtPATTERN, "re?ex*");
	add_analyse_query_test(aq_tests, "re\\?ex\\*", qtSIMPLE, "re?ex*");
	add_analyse_query_test(aq_tests, "re?ex\\*", qtPATTERN, "re?ex*");
	add_analyse_query_test(aq_tests, "re\\?ex*", qtPATTERN, "re?ex*");
	add_analyse_query_test(aq_tests, "\\|data\\?", qtSIMPLE, "|data?");
	add_analyse_query_test(aq_tests, "", qtSIMPLE, "");
	add_analyse_query_test(aq_tests, "\\/", qtSIMPLE, "/");
	add_analyse_query_test(aq_tests, "/", qtFUZZY, "");


	for (std::vector<std::pair<std::string, std::pair<query_t, std::string> > >::const_iterator p=aq_tests.begin();
			 p!=aq_tests.end(); ++p)
		if (!test_analyse_query(p->first.c_str(), p->second))		
			return EXIT_FAILURE;
		
	return EXIT_SUCCESS;
}
