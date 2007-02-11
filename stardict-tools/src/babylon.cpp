#include <gtk/gtk.h>

#include "libbabylonfile.h"

void print_info(const char *info)
{
	g_print("%s", info);
}

int main(int argc,char * argv [])
{
	if (argc<2) {
		printf("please type this:\n./babylon fundset.utf\n");
		return FALSE;
	}

	gtk_set_locale ();
	g_type_init ();
	for (int i=1; i< argc; i++)
		convert_babylonfile (argv[i], print_info);
	return FALSE;	
}

