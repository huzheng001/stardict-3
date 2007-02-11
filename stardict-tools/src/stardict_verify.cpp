#include <stdlib.h>
#include <gtk/gtk.h>

#include "libstardictverify.h"

void print_info(const char *info)
{
        g_print("%s", info);
}

static void verify_dir(gchar *dirname)
{
	GDir *dir = g_dir_open(dirname, 0, NULL);	
	if (dir) {
		const gchar *filename;	
		gchar fullfilename[256];
		while ((filename = g_dir_read_name(dir))!=NULL) {	
			sprintf(fullfilename, "%s/%s", dirname, filename);
			if (g_file_test(fullfilename, G_FILE_TEST_IS_DIR)) {
				verify_dir(fullfilename);
			}
			else if (g_str_has_suffix(filename,".ifo")) {
				stardict_verify(fullfilename, print_info);
			}
		}
		g_dir_close(dir);
	}
}

int main(int argc,char * argv [])
{
	gtk_set_locale ();
	g_type_init ();
	struct Info {
		Info() { g_print("Verifing dictionary files...\n"); }
		~Info() { g_print("Done!\n"); }
	} info;

	if (argc==2)
		return stardict_verify(argv[1], print_info);
	else {
		verify_dir ("/usr/share/stardict/dic");
		gchar home_dir[256];
		sprintf(home_dir, "%s/.stardict/dic", g_get_home_dir());
		verify_dir(home_dir);
	}

	return EXIT_SUCCESS;
}
