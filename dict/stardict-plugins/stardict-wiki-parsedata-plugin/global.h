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

#ifndef _GLOBAL_FUNCTIONS_H_
#define _GLOBAL_FUNCTIONS_H_

#define debug 0

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <stdlib.h>

using namespace std;

#define SINGLE_QUOTE 39
#define DOUBLE_QUOTE '"'

typedef string::value_type chart ; // Char type

string right ( string &s , int num ) ;
string left ( string &s , size_t num ) ;
string upper ( string s ) ;
bool is_text_char ( chart ch ) ;
void explode ( chart ch , string &l , vector <string> &parts ) ;
string implode ( string mid , vector <string> &parts ) ;
string unquote ( chart quote , string &s ) ;
bool submatch ( string &main , string &sub , int from ) ;
string before_first ( chart c , string s ) ;
string before_last ( chart c , string s ) ;
string after_first ( chart c , string s ) ;
string after_last ( chart c , string s ) ;
string trim ( string &s ) ;
string val ( int a ) ;
int find_next_unquoted ( chart c , string &s , int start = 0 ) ;
string xml_embed ( string inside , string tag , string param = "" ) ;
string xml_params ( string l ) ;

#endif

