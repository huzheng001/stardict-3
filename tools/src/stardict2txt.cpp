#include <gtk/gtk.h>

#include "libstardict2txt.h"

int main(int argc,char * argv [])
{
	gtk_set_locale ();
	g_type_init ();

	if (argc!=3) {
		g_print("Usage:\n./stardict2txt somedict.ifo output.txt\n");
	} else {
		convert_stardict2txt(argv[1], argv[2]);
	}
	return FALSE;
}
