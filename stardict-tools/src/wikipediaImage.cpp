#include <stdio.h>
#include <stdlib.h>

#include "mysql.h"

#define CONN_HOST "localhost"
#define CONN_USER "wikiuser"
#define CONN_PASS "123"
#define CONN_DB   "wikidb"


void dump_file()
{
	MYSQL mysql;
	if (!mysql_init(&mysql))
		return;
	if (!mysql_real_connect(&mysql, CONN_HOST , CONN_USER , CONN_PASS, NULL , MYSQL_PORT, NULL, 0))
		return;
	if (mysql_select_db(&mysql, CONN_DB))
		return;
	char *sqlstr = "SELECT img_name img_metadata FROM image";
	if (mysql_query(&mysql, sqlstr))
		return;
	MYSQL_RES *res;
	if (!(res = mysql_use_result(&mysql)))
		return;
	printf("Writing to wiki-images/.\n");
	unsigned int num_fields;
	num_fields = mysql_num_fields(res);
	unsigned int i;
	MYSQL_FIELD *fields;
	MYSQL_ROW row;
	while((row = mysql_fetch_row(res))) {
		for (i=0 ; i < num_fields; i++) {
			fields = mysql_fetch_field_direct(res, i);
			
		}
	}
	mysql_free_result(res);
	mysql_close(&mysql);
}

int main(int argc,char * argv [])
{
	if (mysql_library_init(argc, argv, NULL))
		return 0;
	dump_file();
	mysql_library_end();
	return 0;
}
