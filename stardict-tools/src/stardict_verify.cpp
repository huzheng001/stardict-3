#include <stdlib.h>
#include <gtk/gtk.h>
#include <string>

#include "libstardictverify.h"

static void verify_dir(const std::string& dirname)
{
	GDir *dir = g_dir_open(dirname.c_str(), 0, NULL);
	if (dir) {
		const gchar *filename;
		std::string fullfilename;
		while ((filename = g_dir_read_name(dir))!=NULL) {
			fullfilename = dirname + "/" + filename;
			if (g_file_test(fullfilename.c_str(), G_FILE_TEST_IS_DIR)) {
				verify_dir(fullfilename);
			}
			else if (g_str_has_suffix(filename,".ifo")) {
				stardict_verify(fullfilename.c_str(), g_print);
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
		Info() { g_print("Verifying dictionary files...\n"); }
		~Info() { g_print("Done!\n"); }
	} info;

	if (argc==2)
		return stardict_verify(argv[1], g_print);
	else {
		verify_dir("/usr/share/stardict/dic");
		const std::string home_dir = std::string(g_get_home_dir()) + "/.stardict/dic";
		verify_dir(home_dir);
	}

	return EXIT_SUCCESS;
}
