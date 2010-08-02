#include <gtk/gtk.h>

#include "libtabfile.h"

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
	if (argc<2) {
		printf("please type this:\n./tabfile Chinese-idiom-quick.pdb.tab.utf8\n");
		return FALSE;
	}

	gtk_set_locale ();
	g_type_init ();
	for (int i=1; i< argc; i++)
		convert_tabfile (argv[i], print_info);
	return FALSE;
}
