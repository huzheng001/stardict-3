#include <gtk/gtk.h>

#include "libstardict2txt.h"

void print_info(const char *info, ...)
{
	va_list va;
	va_start(va, info);
	char *str = g_strdup_vprintf(info, va);
	g_print("%s", str);
	g_free(str);
	va_end(va);
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
