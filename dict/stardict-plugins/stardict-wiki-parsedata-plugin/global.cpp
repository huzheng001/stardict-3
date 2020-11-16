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

#include "global.h"

// *****************************************************************************
// *****************************************************************************
//
// global string functions
//
// *****************************************************************************
// *****************************************************************************

// The following functions should be language specific
bool is_text_char ( chart ch )
	{
    if ( ch >= 'a' && ch <= 'z' ) return true ;
    if ( ch >= 'A' && ch <= 'Z' ) return true ;
    return false ;
	}    


// These are not :
    	
string left ( string &s , size_t num )
	{
	if ( num <= 0 ) return "" ;
	if ( num >= s.length() ) return s ;
	return s.substr ( 0 , num ) ;
	}
     
string right ( string &s , int num )
{
	if ( num <= 0 ) return "" ;
	size_t s_len = s.length();
    int from = s_len - num ;
    string ret ;
    if ( from <= 0 )
		ret = s ;
    else
		ret = s.substr ( from , s_len ) ;
    return ret ;
}

string upper ( string s ) // For internal purposes, will do...
{
	size_t a ;
	size_t s_len;
	s_len = s.length();
	for ( a = 0 ; a < s_len ; a++ )
	{
        if ( s[a] >= 'a' && s[a] <= 'z' ) s[a] = s[a] - 'a' + 'A' ;
	}    
    return s ;
}    

void explode ( chart ch , string &l , vector <string> &parts )
{
    parts.clear () ;
    size_t a , b , l_len;
	l_len = l.length();
    for ( a = b = 0 ; a < l_len ; a++ )
    	{
	    if ( l[a] == ch )
	       {
           parts.push_back ( l.substr ( b , a - b ) ) ;
           b = a+1 ;
	       }    
    	}    
   	parts.push_back ( l.substr ( b , a - b ) ) ;

   	if ( debug ) cout << "Explode : " << l << endl ;
	size_t p_size = parts.size();
   	for ( a = 0 ; a < p_size ; a++ )
   		if ( debug ) cout << a << " " << parts[a] << endl ;
    if ( debug ) cout << endl ;   	
}    
	
string implode ( string mid , vector <string> &parts )
{
	size_t p_size = parts.size();
    if ( p_size == 0 ) return "" ;
    if ( p_size == 1 ) return parts[0] ;
    string ret = parts[0] ;
    for ( size_t a = 1 ; a < p_size ; a++ ) {
    	ret += mid + parts[a] ;
	}
   	return ret ;
}    

string unquote ( chart quote , string &s )
{
	size_t a ;
	size_t s_len = s.length();
	for ( a = 0 ; a < s_len ; a++ )
		{
		if ( s[a] == quote && ( a == 0 || ( a > 0 && s[a-1] != '\\' ) ) )
		   {
		   s.insert ( a , "\\" ) ;
		   a++ ;
		   }    
		}    
    return s ;
}    
	
bool submatch ( string &main , string &sub , int from )
{
	size_t sub_len = sub.length();
	if ( from + sub_len > main.length() ) return false ;
	size_t a ;
	for ( a = 0 ; a < sub_len ; a++ )
		{
		if ( sub[a] != main[a+from] ) return false ;
		}    
	return true ;
}
     
int find_first ( chart c , string &s )
{
	size_t a ;
	size_t s_len = s.length();
	for ( a = 0 ; a < s_len && s[a] != c ; a++ ) ;
	if ( a == s_len ) return -1 ;
    return a ;
}    
     
int find_last ( chart c , string &s )
{
	size_t a;
	int b = -1 ;
	size_t s_len = s.length();
	for ( a = 0 ; a < s_len ; a++ )
		{
		if ( s[a] == c ) b = a ;
		}    
	return b ;
}    
     
string before_first ( chart c , string s )
	{
	int pos = find_first ( c , s ) ;
	if ( pos == -1 ) return s ;
	return s.substr ( 0 , pos ) ;
	}

string before_last ( chart c , string s )
	{
	int pos = find_last ( c , s ) ;
	if ( pos == -1 ) return "" ;
	return s.substr ( 0 , pos ) ;
	}

string after_first ( chart c , string s )
	{
	int pos = find_first ( c , s ) ;
	if ( pos == -1 ) return "" ;
	return s.substr ( pos+1 , s.length() ) ;
	}

string after_last ( chart c , string s )
	{
	int pos = find_last ( c , s ) ;
	if ( pos == -1 ) return s ;
	return s.substr ( pos+1 , s.length() ) ;
	}
     
string trim ( string &s )
{
	size_t s_len = s.length();
	if ( s_len == 0 ) return s ;
	if ( s[0] != ' ' && s[s_len-1] != ' ' ) return s ;
	size_t a;
	int b ;
	for ( a = 0 ; a < s_len && s[a] == ' ' ; a++ ) ;
	for ( b = s_len-1 ; b >= 0 && s[b] == ' ' ; b-- ) ;
	return s.substr ( a , b - a + 1 ) ;
}

int find_next_unquoted ( chart c , string &s , int start )
{
	size_t a ;
	chart lastquote = ' ' ;
	size_t s_len = s.length();
	for ( a = start ; a < s_len ; a++ )
		{
		if ( s[a] == c && lastquote == ' ' ) return a ; // Success!
		if ( s[a] != SINGLE_QUOTE && s[a] != DOUBLE_QUOTE ) continue ; // No quotes, next
		if ( a > 0 && s[a-1] == '\\' ) continue ; // Ignore \' and \"
		if ( lastquote == ' ' ) lastquote = s[a] ; // Remember opening quote, text now quoted
		else if ( lastquote == s[a] ) lastquote = ' ' ; // Close quote, not quoted anymore
		}
	return -1 ;
}
    
string val ( int a )
    {
    char t[20] ;
    sprintf ( t , "%d" , a ) ;
    return string ( t ) ;
    }

string xml_embed ( string inside , string tag , string param )
    {
    string ret ;
    ret = "<" + tag ;
    if ( param != "" ) ret += " " + param ;
    if ( inside == "" ) return ret + "/>" ;
    return ret + ">" + trim ( inside ) + "</" + tag + ">" ;
    }

string xml_params ( string l ) // Yes, this function is thin...
	{
	string ret ;
	vector <string> params ;
	while ( l != "" )
		{
 		int p = find_next_unquoted ( ' ' , l ) ;
 		string first ;
 		if ( p == -1 )
 			{
  			first = l ;
  			l = "" ;
 			}
 		else
   			{
		    first = left ( l , p ) ;
		    l = l.substr ( p , l.length() - p ) ;
   			}        
		first = trim ( first ) ;
		l = trim ( l ) ;
		if ( first == "" ) continue ;
		
		p = find_next_unquoted ( '=' , first ) ;
		if ( p == -1 ) first = xml_embed ( first , "value" ) ;
		else
			{
			first = xml_embed ( left ( first , p ) , "key" ) +
					xml_embed ( first.substr ( p + 1 , first.length() - p ) , "value" ) ;
			}    
		first = xml_embed ( first , "wikiparameter" ) ;
		ret += first ;
		}    
	return ret ;
	}
    
