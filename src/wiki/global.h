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
string left ( string &s , int num ) ;
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

