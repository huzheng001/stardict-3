#include "WIKI2XML.h"

TTableInfo::TTableInfo ()
	{
	tr_open = false ;
 	td_open = false ;
	}    

string TTableInfo::close ()
	{
	string ret ;
	if ( td_open ) ret += "</wikitablecell>" ;
	if ( tr_open ) ret += "</wikitablerow>" ;
	ret += "</wikitable>" ;
	return ret ;
	}    
	
string TTableInfo::new_row ()
	{
	string ret ;
	if ( td_open ) ret += "</wikitablecell>" ;
	if ( tr_open ) ret += "</wikitablerow>" ;
	ret += "<wikitablerow>" ;
	td_open = false ;
	tr_open = true ;
	return ret ;
	}    

string TTableInfo::new_cell ( string type )
	{
	string ret ;
	if ( !tr_open ) ret += new_row () ;
	if ( td_open ) ret += "</wikitablecell>" ;
	ret += "<wikitablecell type=\"" + upper ( type ) + "\">" ;
	td_type = type ;
	td_open = true ;
	return ret ;
	}    

// *****************************************************************************
// *****************************************************************************
//
// WIKI2XML
//
// *****************************************************************************
// *****************************************************************************

void WIKI2XML::parse_symmetric ( string &l , int &from , 
         							string s1 , string s2 ,
         							string r1 , string r2 ,
                                    bool extend )
	{
	int a , b ;
	if ( !submatch ( l , s1 , from ) ) return ; // Left does not match
	for ( a = from + s1.length() ; a + s2.length() <= l.length() ; a++ )
		{
		if ( !submatch ( l , s2 , a ) ) continue ;
		for ( b = a+1 ; extend && submatch ( l , s2 , b ) ; b++ ) ;
		b-- ;
		l = l.substr ( 0 , from ) +
			r1 +
			l.substr ( from + s1.length() , b - from - s1.length() )  +
			r2 +
			l.substr ( b + s2.length() , l.length() ) ;
		if ( debug ) cout << "newl : " << l << endl ;
		break ;
		}    
	}
     
void WIKI2XML::parse_link ( string &l , int &from , char mode )
	{
    from += 1 ;
    int a , cnt = 1 ;
    chart par_open = '[' ; // mode 'L'
    chart par_close = ']' ; // mode 'L'
    if ( mode == 'T' ) { par_open = '{' ; par_close = '}' ; }
    for ( a = from ; cnt > 0 && a+1 < l.length() ; a++ )
    	{
	    if ( l[a] == par_open && l[a+1] == par_open )
	    	parse_link ( l , a ) ;
    	else if ( l[a] == par_close && l[a+1] == par_close )
    		cnt-- ;
    	}    
   	if ( cnt > 0 ) return ; // Not a valid link
   	
   	int to = a-1 ; // Without "]]"
   	string link = l.substr ( from+1 , to-from-1 ) ;
   	
   	TXML x ;
   	vector <string> parts ;
   	explode ( '|' , link , parts ) ;
   	if ( mode == 'L' )
        {
        x.name = "wikilink" ;
        x.add_key_value ( "type" , "internal" ) ;
        }
   	else if ( mode == 'T' ) x.name = "wikitemplate" ;
   	
   	for ( a = 0 ; a < parts.size() ; a++ )
   	    {
   	    bool last = ( a + 1 == parts.size() ) ;
   	    string p = parts[a] ;
   	    parse_line_sub ( p ) ;

   	    if ( a > 0 && ( mode != 'L' || !last ) )
           {
           string key , value ;
           vector <string> subparts ;
           explode ( '=' , p , subparts ) ;
           if ( subparts.size() == 1 )
              {
              value = xml_embed ( p , "value" ) ;
              }
           else
              {
              key = xml_embed ( subparts[0] , "key" ) ;
              subparts.erase ( subparts.begin() ) ;
              value = xml_embed ( implode ( "=" , subparts ) , "value" ) ;
              }
           p = key + value ;
           }
        else p = xml_embed ( p , "value" ) ;

   	    string param = "number=\"" + val ( a ) + "\"" ;
   	    if ( last ) param += " last=\"1\"" ;
   	    x.text += xml_embed ( p , "wikiparameter" , param ) ;
   	    }

	if ( mode == 'L' ) // Try link trail
	   {
	   string trail ;
	   for ( a = to+2 ; a < l.length() && is_text_char ( l[a] ) ; a++ )
	       trail += l[a] ;
       to = a-2 ;
       if ( trail != "" ) x.text += xml_embed ( trail , "trail" ) ;
       }
   	
   	x.add_key_value ( "parameters" , val ( parts.size() ) ) ;
	string replacement = x.get_string () ;
	parse_line_sub ( replacement ) ;
	
   	l.erase ( from-1 , to-from+3 ) ;
   	l.insert ( from-1 , replacement ) ;
   	if ( debug ) cout << "Link : " << link << endl << "Replacement : " << replacement << endl ;
   	if ( debug ) cout << "Result : " << l << endl << endl ;
   	from = from + replacement.length() - 2 ;
	}    
	
bool WIKI2XML::is_list_char ( chart c ) // For now...
	{
	if ( c == '*' ) return true ;
	if ( c == '#' ) return true ;
	if ( c == ':' ) return true ;
	return false ;
	}    

string WIKI2XML::get_list_tag ( chart c , bool open )
    {
    string ret ;
    if ( debug ) cout << "get_list_tag : " << c << endl ;
    if ( c == '*' ) ret = "ul" ;
    if ( c == '#' ) ret = "ol" ;
    if ( c == ':' ) ret = "dl" ;
    if ( ret != "" )
    	{
	    string itemname = "li" ;
	    if ( c == ':' ) itemname = "dd" ;
	    if ( open ) ret = "<" + ret + "><" + itemname + ">" ;
	    else ret = "</" + itemname + "></" + ret + ">" ;
    	}    
   	return ret ;
    }
        
string WIKI2XML::fix_list ( string &l )
    {
    int a , b ;
    for ( a = 0 ; a < l.length() && is_list_char ( l[a] ) ; a++ ) ;
    string newlist , pre ;
    if ( a > 0 )
    	{
        newlist = left ( l , a ) ;
        while ( a < l.length() && l[a] == ' ' ) a++ ; // Removing leading blanks
        l = l.substr ( a , l.length() ) ;
        }
    if ( debug ) cout << "fix_list : " << l << endl ;
    if ( list == "" && newlist == "" ) return "" ;
    for ( a = 0 ; a < list.length() && 
    				a < newlist.length() && 
        			list[a] == newlist[a] ; a++ ) ; // The common part, if any
        			
    for ( b = a ; b < list.length() ; b++ )
    	pre = get_list_tag ( list[b] , false ) + pre ; // Close old list tags
    for ( b = a ; b < newlist.length() ; b++ )
    	pre += get_list_tag ( newlist[b] , true ) ; // Open new ones
   	
    if ( debug ) cout << "pre : " << pre << endl ;
    if ( debug ) cout << "newlist : " << newlist << endl ;
    list = newlist ;
    return pre ;
    }    

void WIKI2XML::parse_line ( string &l )
    {
    int a , b ;
    if ( debug ) cout << l << endl ;
    string pre ;
    string oldlist = list ;
    pre += fix_list ( l ) ;
    if ( list != "" && list == oldlist )
    	{
	    string itemname = "li" ;
	    if ( right ( list , 1 ) == ":" ) itemname = "dd" ;
        pre = "</" + itemname + "><" + itemname + ">" + pre ;
        }
    
    if ( l == "" ) // Paragraph
    	{
	    l = "<p/>" ;
    	}
   	else if ( left ( l , 4 ) == "----" ) // <hr>
   	    {
   	    for ( a = 0 ; a < l.length() && l[a] == l[0] ; a++ ) ;
   	    pre += "<wikiurlcounter action=\"reset\"/><hr/>" ;
   	    l = l.substr ( a , l.length() - a ) ;
   	    }
   	else if ( l != "" && l[0] == '=' ) // Heading
   		{
	    for ( a = 0 ; a < l.length() && l[a] == '=' && l[l.length()-a-1] == '=' ; a++ ) ;
	    string h = "h0" ;
	    if ( a >= l.length() ) h = "" ; // No heading
//	    else if ( l[a] != ' ' ) h = "" ;
//	    else if ( l[l.length()-a-1] != ' ' ) h = "" ;
	    else if ( a < 1 || a > 9 ) h = "" ;
	    if ( h != "" )
	    	{
	    	l = l.substr ( a , l.length() - a*2 ) ;
    	    h[1] += a ;
    	    l = xml_embed ( l , h ) ;
    	    }    
   		}    
    else if ( l != "" && l[0] == ' ' ) // Pre-formatted text
    	{
	    for ( a = 0 ; a < l.length() && l[a] == ' ' ; a++ ) ;
	    l = l.substr ( a , l.length() ) ;
	    if ( l != "" )
     		{
            pre += "<pre>" + l + "</pre>" ;
            l = "" ;
            }    
    	}
   	else if ( left ( l , 2 ) == "{|" || left ( l , 2 ) == "|}" ||
   				( tables.size() > 0 && l != "" && ( l[0] == '|' || l[0] == '!' ) ) )
        {
        pre += table_markup ( l ) ;
        l = "" ;
        }    
   		
    	
   	if ( l != "" ) parse_line_sub ( l ) ;
    
    if ( pre != "" ) l = pre + l ;   
    }    

bool WIKI2XML::is_external_link_protocol ( string protocol )
    {
    if ( protocol == "HTTP" ) return true ;
    if ( protocol == "FTP" ) return true ;
    if ( protocol == "MAILTO" ) return true ;
    return false ;
    }
    
int WIKI2XML::scan_url ( string &l , int from )
    {
    int a ;
    for ( a = from ; a < l.length() ; a++ )
        {
        if ( l[a] == ':' || l[a] == '/' || l[a] == '.' ) continue ;
        if ( l[a] >= '0' && l[a] <= '9' ) continue ;
        if ( is_text_char ( l[a] ) ) continue ;
        break ; // End of URL
        }
    return a ;
    }
	
void WIKI2XML::parse_external_freelink ( string &l , int &from )
	{
	int a ;
	for ( a = from - 1 ; a >= 0 && is_text_char ( l[a] ) ; a-- ) ;
	if ( a == -1 ) return ;
	a++ ;
	string protocol = upper ( l.substr ( a , from - a ) ) ;
	if ( debug ) cout << "protocol : " << protocol << endl ;
	if ( !is_external_link_protocol ( protocol ) ) return ;	
	int to = scan_url ( l , a ) ;
	string url = l.substr ( a , to - a ) ;
	string replacement ;
    replacement += xml_embed ( url , "url" ) ;
    replacement += xml_embed ( url , "title" ) ;
	l = left ( l , a ) + replacement + l.substr ( to , l.length() - to ) ;
	from = a + replacement.length() - 1 ;
	}
	
void WIKI2XML::parse_external_link ( string &l , int &from )
	{
	string protocol = upper ( before_first ( ':' , l.substr ( from + 1 , l.length() - from ) ) ) ;
	if ( !is_external_link_protocol ( protocol ) ) return ;
    int to ;
    for ( to = from + 1 ; to < l.length() && l[to] != ']' ; to++ ) ;
    if ( to == l.length() ) return ;
    string url = l.substr ( from + 1 , to - from - 1 ) ;
    string title = after_first ( ' ' , url ) ;
    url = before_first ( ' ' , url ) ;
    string replacement ;
    replacement += xml_embed ( url , "url" ) ;
    if ( title == "" )
        replacement += xml_embed ( "<wikiurlcounter action=\"add\"/>" , "title" ) ;
    else replacement += xml_embed ( title , "title" ) ;
    replacement = xml_embed ( replacement , "wikilink" , "type='external' protocol='" + protocol + "'" ) ;
    l = left ( l , from ) + replacement + l.substr ( to + 1 , l.length() - to ) ;
    from = from + replacement.length() - 1 ;
	}
	
void WIKI2XML::parse_line_sub ( string &l )
	{
	int a ;
    for ( a = 0 ; a < l.length() ; a++ )
        {
        if ( l[a] == '[' && a+1 < l.length() && l[a+1] == '[' ) // [[Link]]
        	parse_link ( l , a , 'L' ) ;
        else if ( l[a] == '{' && a+1 < l.length() && l[a+1] == '{' ) // {{Template}}
        	parse_link ( l , a , 'T' ) ;
       	else if ( l[a] == '[' ) // External link
            parse_external_link ( l , a ) ;
        else if ( a+2 < l.length() && l[a] == ':' && l[a+1] == '/' && l[a+2] == '/' ) // External freelink
            parse_external_freelink ( l , a ) ;
      	else if ( l[a] == SINGLE_QUOTE ) // Bold and italics
       		{
        	parse_symmetric ( l , a , "'''" , "'''" , "<b>" , "</b>" , true ) ; 
        	parse_symmetric ( l , a , "''" , "''" , "<i>" , "</i>" ) ; 
         	}
        } 
	}
     
void WIKI2XML::parse_lines ( vector <string> &lines )
    {
    int a ;
    for ( a = 0 ; a < lines.size() ; a++ )
        {
        parse_line ( lines[a] ) ;
        }
        
    string end ;
    
    // Cleanup lists
    end = fix_list ( end ) ;
    if ( end != "" ) lines.push_back ( end ) ;
    
    // Cleanup tables
    end = "" ;
    while ( tables.size() )
    	{
	    end += tables[tables.size()-1].close () ;
	    tables.pop_back () ;
    	}    
   	if ( end != "" ) lines.push_back ( end ) ;
    }    

void WIKI2XML::init ( string s )
	{
	list = "" ;
	lines.clear () ;
	
	// Now we remove evil HTML
	allowed_html.clear () ;
	allowed_html.push_back ( "b" ) ;
	allowed_html.push_back ( "i" ) ;
	allowed_html.push_back ( "p" ) ;
	allowed_html.push_back ( "b" ) ;
	allowed_html.push_back ( "br" ) ;
	allowed_html.push_back ( "hr" ) ;
	allowed_html.push_back ( "tt" ) ;
	allowed_html.push_back ( "pre" ) ;
	allowed_html.push_back ( "nowiki" ) ;
	allowed_html.push_back ( "math" ) ;
	allowed_html.push_back ( "strike" ) ;
	allowed_html.push_back ( "u" ) ;
	allowed_html.push_back ( "table" ) ;
	allowed_html.push_back ( "caption" ) ;
	allowed_html.push_back ( "tr" ) ;
	allowed_html.push_back ( "td" ) ;
	allowed_html.push_back ( "th" ) ;
	allowed_html.push_back ( "li" ) ;
	allowed_html.push_back ( "ul" ) ;
	allowed_html.push_back ( "ol" ) ;
	allowed_html.push_back ( "dl" ) ;
	allowed_html.push_back ( "dd" ) ;
	allowed_html.push_back ( "dt" ) ;
	allowed_html.push_back ( "div" ) ;
	allowed_html.push_back ( "h1" ) ;
	allowed_html.push_back ( "h2" ) ;
	allowed_html.push_back ( "h3" ) ;
	allowed_html.push_back ( "h4" ) ;
	allowed_html.push_back ( "h5" ) ;
	allowed_html.push_back ( "h6" ) ;
	allowed_html.push_back ( "h7" ) ;
	allowed_html.push_back ( "h8" ) ;
	allowed_html.push_back ( "h9" ) ;
	allowed_html.push_back ( "small" ) ;
	allowed_html.push_back ( "center" ) ;
//	allowed_html.push_back ( "" ) ;
	int a ;
	for ( a = 0 ; a < allowed_html.size() ; a++ )
		allowed_html[a] = upper ( allowed_html[a] ) ;
	
	vector <TXML> taglist ;
	make_tag_list ( s , taglist ) ;
	remove_evil_html ( s , taglist ) ;
	
	// Now evaluate each line
	explode ( '\n' , s , lines ) ;
	}    

string WIKI2XML::get_xml ()
	{
	string xmlheader = "<?xml version='1.0' encoding='UTF-8'?>" ;
	string ret = xmlheader + "<text>" + implode ( " " , lines ) + "</text>" ;
	
	// Invalidating mdash
	int a = ret.find ( "&mdash;" ) ;
	while ( a >= 0 && a < ret.length() )
		{
		ret[a] = '!' ;
		a = ret.find ( "&mdash;" , a ) ;
		}    
		
	return ret ;
	}
	
void WIKI2XML::replace_part ( string &s , int from , int to , string with )
	{
	s = s.substr ( 0 , from ) + with + s.substr ( to + 1 , s.length() - to - 1 ) ;
	}    
    
void WIKI2XML::replace_part_sync ( string &s , int from , int to , string with , vector <TXML> &list )
	{
	int a , b ;
	replace_part ( s , from , to , with ) ;
	for ( a = 0 ; a < list.size() ; a++ )
		{
		for ( b = 0 ; b < with.length() ; b++ ) list[a].insert_at ( from ) ;
		for ( b = from ; b <= to ; b++ ) list[a].remove_at ( from ) ;
		}    
	}    
    
// ATTENTION : this doesn't handle all HTML comments correctly!
void WIKI2XML::make_tag_list ( string &s , vector <TXML> &list )
	{
	list.clear () ;
	int a , b ;
	for ( a = 0 ; a < s.length() ; a++ )
		{
		if ( s[a] == '>' ) // Rouge >
			{
	    	s[a] = ';' ;
	    	s.insert ( a , "&gt" ) ;
			continue ;
			}
		else if ( s[a] != '<' ) continue ;
		b = find_next_unquoted ( '>' , s , a ) ;
		if ( b == -1 ) // Rouge <
  			{
	    	s[a] = ';' ;
	    	s.insert ( a , "&lt" ) ;
         	continue ;
         	}   	
		list.push_back ( TXML ( a , b , s ) ) ;
		a = list[list.size()-1].to ;
		}    
	}
 
void WIKI2XML::remove_evil_html ( string &s , vector <TXML> &taglist )
	{
	int a , b ;
	for ( a = 0 ; a < taglist.size() ; a++ )
		{
		string tag = upper ( taglist[a].name ) ;
		for ( b = 0 ; b < allowed_html.size() && tag != allowed_html[b] ; b++ ) ;
		if ( b < allowed_html.size() ) continue ;
		replace_part_sync ( s , taglist[a].from , taglist[a].from , "&lt;" , taglist ) ;
		replace_part_sync ( s , taglist[a].to , taglist[a].to , "&gt;" , taglist ) ;
		}    
	}

string WIKI2XML::table_markup ( string &l )
	{
	int a ;
	string ret ;
	if ( left ( l , 2 ) == "{|" ) // Open table
		{
		ret = "<wikitable>" ;
		ret += xml_embed ( l.substr ( 2 , l.length() - 2 ) , "wikiparameter" ) ;
		tables.push_back ( TTableInfo () ) ;
		}
	else if ( left ( l , 2 ) == "|}" ) 
		{
		ret = tables[tables.size()-1].close () ;
		tables.pop_back () ;
		}
	else if ( left ( l , 2 ) == "|-" ) 
		{
		ret = tables[tables.size()-1].new_row () ;
		for ( a = 1 ; a < l.length() && l[a] == '-' ; a++ ) ;
		ret += xml_params ( l.substr ( a , l.length() - a ) ) ;
		}
	else
		{
		string init ;
		if ( left ( l , 2 ) == "|+" )
			{
 			init = "caption" ;
 			l = l.substr ( 2 , l.length() - 2 ) ;
			}    
		else if ( l[0] == '!' )
			{
 			init = "header" ;
 			l = l.substr ( 1 , l.length() - 1 ) ;
			}    
		else if ( l[0] == '|' )
			{
 			init = "cell" ;
 			l = l.substr ( 1 , l.length() - 1 ) ;
			}
		vector <string> sublines ;
		for ( a = 0 ; a + 1 < l.length() ; a++ )
			{
 			if ( l[a] == '|' && l[a+1] == '|' )
 			   {
 			   sublines.push_back ( left ( l , a ) ) ;
 			   l = l.substr ( a + 2 , l.length() - a ) ;
 			   a = -1 ;
 			   }    
			}    
		if ( l != "" ) sublines.push_back ( l ) ;
		for ( a = 0 ; a < sublines.size() ; a++ )
			{
			l = sublines[a] ;
			parse_line_sub ( l ) ;
			string params ;
			int b = find_next_unquoted ( '|' , l ) ;
			if ( b != -1 )
   				{
			    params = left ( l , b ) ;
			    l = l.substr ( b + 1 , l.length() - b ) ;
       			}        
			if ( params != "" ) l = xml_params ( params ) + l ;
			ret += tables[tables.size()-1].new_cell ( init ) ;
			ret += l ;
			}    
		}    
	return ret ;
	}    
