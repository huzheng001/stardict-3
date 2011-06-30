#include "stdio.h"
#include "stdlib.h"
#include <string.h>
#include <sys/stat.h>
#include <ctype.h>

#include <string>

#include <gtk/gtk.h>
#include <glib.h>

void convert(char *filename)
{
	struct stat stats;
        if (stat (filename, &stats) == -1) {
                printf("file not exist!\n");
                return;
        }
        gchar *basefilename = g_path_get_basename(filename);
        FILE *gmxfile;
        gmxfile = fopen(filename,"r");

        gchar *buffer = (gchar *)g_malloc (stats.st_size + 1);
        fread (buffer, 1, stats.st_size, gmxfile);
        fclose (gmxfile);
        buffer[stats.st_size] = '\0';

	gchar utffilename[256];
	sprintf(utffilename, "%s.utf", basefilename);
	FILE *utffile = fopen(utffilename,"w");

	gchar *p;
	p=buffer;
	while (*p) {
		switch (*p) {
			case 0x06:
				fwrite("ɔ", strlen("ɔ"), 1, utffile);
				break;
			case 0x04:
                                fwrite("ɑ", strlen("ɑ"), 1, utffile);
                                break;
                        case 0x10:
                                fwrite("ә", strlen("ә"), 1, utffile);
                                break;
                        case 0x0f:
                                fwrite("ʌ", strlen("ʌ"), 1, utffile);
                                break;
                        case 0x0b:
                                fwrite("θ", strlen("θ"), 1, utffile);
                                break;
                        case 0x1c:
                                fwrite("ð", strlen("ð"), 1, utffile);
                                break;
                        case 0x1d:
                                fwrite("ʃ", strlen("ʃ"), 1, utffile);
                                break;
                        case 0x1e:
                                fwrite("ʒ", strlen("ʒ"), 1, utffile);
                                break;
                        case 0x19:
                                fwrite("ŋ", strlen("ŋ"), 1, utffile);
                                break;
                        case 0x03:
                                fwrite("æ", strlen("æ"), 1, utffile);
                                break;
                        case 0x02:
                                fwrite("є", strlen("є"), 1, utffile);
                                break;
                        case 0x01:
                                fwrite("I", strlen("I"), 1, utffile);
                                break;
			case 0x13:
				fwrite("ɚ", strlen("ɚ"), 1, utffile);
                                break;
			case 0x0e:
				fwrite("ʊ", strlen("ʊ"), 1, utffile);
				break;
			case 0x15:
				fwrite("ṃ", strlen("ṃ"), 1, utffile);
				break;
			case 0x16:
				fwrite("ṇ", strlen("ṇ"), 1, utffile);
				break;
			case 0x17:
				fwrite("ḷ", strlen("ḷ"), 1, utffile);
				break;
			case 0x11:
			case 0x1f:
				fwrite("ˏ", strlen("ˏ"), 1, utffile);
                                break;
			default:
				fwrite(p, 1, 1, utffile);
				break;
		}
		p++;
	} 
	fclose(utffile);

	g_free(buffer);
	g_free(basefilename);
}

int main(int argc,char * argv [])
{
        if (argc<2) {
                printf("please type this:\n./gmx2utf lazyworm-E2C.pdb.tab.utf\n");
                return FALSE;
        }
	gtk_set_locale ();
        g_type_init ();
	for (int i=1; i< argc; i++)
                convert (argv[i]);
        return FALSE;
}

