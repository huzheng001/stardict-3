#include <gtk/gtk.h>

#include "libstardict2txt.h"

void print_info(const char *info)
{
        g_print("%s", info);
}

int main(int argc,char * argv [])
{
	gtk_set_locale ();
	g_type_init ();

	if (argc!=2) {
		g_print("please type this:\n./stardict2txt somedict.ifo\n");
	} else {
		convert_stardict2txt(argv[1], print_info);
	}
	return FALSE;
}
