// WoAiFaYu dictionary's data files are encoded both in French and Chinese, the French encoding sees to to created by himself, this program try to convert these data files to UTF-8 format.

#include "stdio.h"
#include "stdlib.h"
#include <string.h>
#include <sys/stat.h>
#include <ctype.h>

#include <string>

#include <gtk/gtk.h>
#include <glib.h>

/*
bool isfr(guchar c1, guchar c2)
{
	if ((c1==0xA8 && c2==0xA4) || (c1==0xE2 && c2==0x20) || (c1==0xA8 && c2==0xA6) || (c1==0xA8 && c2==0xA8) || (c1==0xA8 && c2==0xBA) || (c1==0xEB && c2==0x20) || (c1==0xE7 && c2==0x20) || (c1==0xEE && c2==0x20) || (c1==0xEF && c2==0x20) || (c1==0xF4 && c2==0x20) || (c1==0x9C && c2==0x20) || (c1==0xFB && c2==0x20) || (c1==0xA8 && c2==0xB4) || (c1==0xA8 && c2==0xB9))
		return true;
	return false;
}
*/

gchar *convert_fr(gchar *p, GIConv *gb2utf, GIConv *fr2utf, gint linenum)
{
	gchar *str;
	std::string string;
	while (*p) {
		if (isascii(*p)) {
			string+=*p;
		} else {
			if ((guchar)*p==0xA8 && (guchar)*(p+1)==0xA4) {
				string+="à";
			} else if ((guchar)*p==0xE2 && (guchar)*(p+1)==0x20) {
				string+="â";
			} else if ((guchar)*p==0xA8 && (guchar)*(p+1)==0xA6) {
				string+="é";
			} else if ((guchar)*p==0xA8 && (guchar)*(p+1)==0xA8) {
				string+="è";
			} else if ((guchar)*p==0xA8 && (guchar)*(p+1)==0xBA) {
				string+="ê";
			} else if ((guchar)*p==0xEB && (guchar)*(p+1)==0x20) {
				string+="ë";
			} else if ((guchar)*p==0xE7 && (guchar)*(p+1)==0x20) {
				string+="ç";
			} else if ((guchar)*p==0xEE && (guchar)*(p+1)==0x20) {
                                string+="î";
			} else if ((guchar)*p==0xEF && (guchar)*(p+1)==0x20) {
                                string+="ï";
			} else if ((guchar)*p==0xF4 && (guchar)*(p+1)==0x20) {
                                string+="ô";
			} else if ((guchar)*p==0x9C && (guchar)*(p+1)==0x20) {
                                string+="œ";
			} else if ((guchar)*p==0xFB && (guchar)*(p+1)==0x20) {
                                string+="û";
			} else if ((guchar)*p==0xA8 && (guchar)*(p+1)==0xB4) {
                                string+="ù";
			} else if ((guchar)*p==0xA8 && (guchar)*(p+1)==0xB9) {
                                string+="ü";
			} else if ((guchar)*p>=0x81 && (guchar)*p<=0xFE && (guchar)*(p+1)>=0x40 && (guchar)*(p+1)<=0xFE) {
				str = g_convert_with_iconv(p, 2, *gb2utf, NULL, NULL, NULL);
				if (str) {
					string+=str;
				} else {
					return NULL;
				}
			} else {
				str = g_convert_with_iconv(p, 1, *fr2utf, NULL, NULL, NULL);
				if (str) {
					string+=str;
				} else {
					g_print("Unkown %d : %c\n", linenum, *p);
				}
				p--;
			}
			p++;
		}
		p++;
	}
	return g_strdup(string.c_str());
}

void convert(char *filename)
{
	struct stat stats;
        if (stat (filename, &stats) == -1)
        {
                printf("file not exist!\n");
                return;
        }
        gchar *basefilename = g_path_get_basename(filename);
        FILE *degbfile;
        degbfile = fopen(filename,"r");

        gchar *buffer = (gchar *)g_malloc (stats.st_size + 1);
        fread (buffer, 1, stats.st_size, degbfile);
        fclose (degbfile);
        buffer[stats.st_size] = '\0';

	gchar utffilename[256];
	sprintf(utffilename, "%s.utf", basefilename);
	FILE *utffile = fopen(utffilename,"w");

	GIConv gb2utf = g_iconv_open("UTF-8", "GBK");
	GIConv fr2utf = g_iconv_open("UTF-8", "ISO-8859-15");

	gchar *p, *p1, *p2;
	p=buffer;
	gchar *str;
	int linenum = 1;
	while (true) {
		if (*p=='\0')
			break;
		p1 = strchr(p, '\n');
		if (!p1) {
			g_print("Error, no ending new line.\n");
			return;
		}
		*p1='\0';
		p2 = strchr(p, '=');
		if (!p2) {
			g_print("Error %d: no =\n", linenum);
			return;
		}
		*p2='\0';
		str = convert_fr(p, &gb2utf, &fr2utf, linenum);
		if (str) {
			fwrite(str, strlen(str), 1, utffile);
			g_free(str);
		} else {
			g_print("Error1: %d\n", linenum);
		}
		fwrite("=", 1, 1, utffile);
		p2++;
		str = convert_fr(p2, &gb2utf, &fr2utf, linenum);
		if (str) {
			fwrite(str, strlen(str), 1, utffile);
			g_free(str);
		} else {
                	g_print("Error2: %d\n", linenum);
		}
		fwrite("\n", 1, 1, utffile);
		p=p1+1;
		linenum++;
	} 

	g_iconv_close(gb2utf);
	g_iconv_close(fr2utf);
	fclose(utffile);

	g_free(buffer);
	g_free(basefilename);
}

int main(int argc,char * argv [])
{
        if (argc<2) {
                printf("please type this:\n./frgb2utf fundset.txt\n");
                return FALSE;
        }
	gtk_set_locale ();
        g_type_init ();
	for (int i=1; i< argc; i++)
                convert (argv[i]);
        return FALSE;
}

