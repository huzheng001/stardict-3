#ifndef _WIKI2XML_H_
#define _WIKI2XML_H_

#include "global.h"
#include "TXML.h"

class TTableInfo
	{
	public :
	TTableInfo () ;
	virtual string new_cell ( string type ) ;
	virtual string new_row () ;
	virtual string close () ;
	bool tr_open , td_open ;
	string td_type ;
	} ;    

class WIKI2XML
	{
	public :
	WIKI2XML () {} ;
	WIKI2XML ( string &s ) { init ( s ) ; }
	WIKI2XML ( vector <string> &l ) { init ( l ) ; }
	virtual void init ( string s ) ;
	virtual void init ( vector <string> &l ) { init ( implode ( "\n" , l ) ) ; }
	virtual void parse () { parse_lines ( lines ) ; }
	virtual string get_xml () ;
	
	private :
	virtual void make_tag_list ( string &s , vector <TXML> &list ) ;
	virtual void parse_symmetric ( string &l , int &from , 
 						string s1 , string s2 , 
                        string r1 , string r2 , bool extend = false ) ;
	virtual void parse_link ( string &l , int &from , char mode = 'L' ) ;
	virtual void parse_line_sub ( string &l ) ;
	virtual void parse_line ( string &l ) ;
	virtual void parse_lines ( vector <string> &lines ) ;
	virtual string fix_list ( string &l ) ;
	virtual string get_list_tag ( chart c , bool open ) ;
	virtual bool is_list_char ( chart c ) ;
	virtual void remove_evil_html ( string &s , vector <TXML> &taglist ) ;
	virtual void replace_part ( string &s , int from , int to , string with ) ;
	virtual void replace_part_sync ( string &s , int from , int to , string with , vector <TXML> &list ) ;
	virtual void parse_external_freelink ( string &l , int &from ) ;
	virtual void parse_external_link ( string &l , int &from ) ;
	virtual bool is_external_link_protocol ( string protocol ) ;
	virtual int scan_url ( string &l , int from ) ;
	virtual string table_markup ( string &l ) ;
		
	// Variables
	vector <string> lines , allowed_html ;
	vector <TTableInfo> tables ;
	string list ;
    } ;             

#endif
