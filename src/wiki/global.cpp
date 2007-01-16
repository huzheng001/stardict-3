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
    	
string left ( string &s , int num )
	{
	if ( num <= 0 ) return "" ;
	if ( num >= s.length() ) return s ;
	return s.substr ( 0 , num ) ;
	}
     
string right ( string &s , int num )
	{
	if ( num <= 0 ) return "" ;
    int from = s.length() - num ;
    string ret ;
    if ( from <= 0 ) ret = s ;
    else ret = s.substr ( from , s.length() ) ;
    return ret ;
	}    

string upper ( string s ) // For internal purposes, will do...
	{
	int a ;
	for ( a = 0 ; a < s.length() ; a++ )
		{
        if ( s[a] >= 'a' && s[a] <= 'z' ) s[a] = s[a] - 'a' + 'A' ;
		}    
    return s ;
	}    

void explode ( chart ch , string &l , vector <string> &parts )
	{
    parts.clear () ;
    int a , b ;
    for ( a = b = 0 ; a < l.length() ; a++ )
    	{
	    if ( l[a] == ch )
	       {
           parts.push_back ( l.substr ( b , a - b ) ) ;
           b = a+1 ;
	       }    
    	}    
   	parts.push_back ( l.substr ( b , a - b ) ) ;

   	if ( debug ) cout << "Explode : " << l << endl ;
   	for ( a = 0 ; a < parts.size() ; a++ )
   		if ( debug ) cout << a << " " << parts[a] << endl ;
    if ( debug ) cout << endl ;   	
	}    
	
string implode ( string mid , vector <string> &parts )
	{
    if ( parts.size() == 0 ) return "" ;
    if ( parts.size() == 1 ) return parts[0] ;
    string ret = parts[0] ;
    for ( int a = 1 ; a < parts.size() ; a++ )
    	ret += mid + parts[a] ;
   	return ret ;
	}    

string unquote ( chart quote , string &s )
	{
	int a ;
	for ( a = 0 ; a < s.length() ; a++ )
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
	if ( from + sub.length() > main.length() ) return false ;
	int a ;
	for ( a = 0 ; a < sub.length() ; a++ )
		{
		if ( sub[a] != main[a+from] ) return false ;
		}    
	return true ;
	}
     
int find_first ( chart c , string &s )
	{
	int a ;
	for ( a = 0 ; a < s.length() && s[a] != c ; a++ ) ;
	if ( a == s.length() ) return -1 ;
    return a ;
	}    
     
int find_last ( chart c , string &s )
	{
	int a , b = -1 ;
	for ( a = 0 ; a < s.length() ; a++ )
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
	if ( s.length() == 0 ) return s ;
	if ( s[0] != ' ' && s[s.length()-1] != ' ' ) return s ;
	int a , b ;
	for ( a = 0 ; a < s.length() && s[a] == ' ' ; a++ ) ;
	for ( b = s.length()-1 ; b >= 0 && s[b] == ' ' ; b-- ) ;
	return s.substr ( a , b - a + 1 ) ;
	}

int find_next_unquoted ( chart c , string &s , int start )
	{
	int a ;
	chart lastquote = ' ' ;
	for ( a = start ; a < s.length() ; a++ )
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
    
