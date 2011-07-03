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

#ifndef _TXML_H_
#define _TXML_H_

#include "global.h"

class TXML
	{
    public :
    TXML () {} ;
    virtual ~TXML() {};
	TXML ( int f , int t , string &s , bool fix_comments = true ) ;
	virtual void remove_at ( int pos ) ;
	virtual void insert_at ( int pos ) ;
	
    virtual void add_key_value ( string k , string v = "" ) ;
    virtual string get_string () ;
    
	// Variables
	int from , to ;
	bool closing , selfclosing ;
    string name , text ;
    vector <string> key , value ;
	} ;    
	
#endif
