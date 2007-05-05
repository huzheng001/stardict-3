#include <string.h>
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
	bool strip_html = true;
	for (int i=1; i < argc; i++) {
		if (strcmp(argv[i], "-n")==0) {
			strip_html = false;
			break;
		}
	}

	gtk_set_locale ();
	g_type_init ();
	convert_babylonfile (argv[argc-1], print_info, strip_html);
	return FALSE;	
}

