/*
 * This file is part of StarDict.
 *
 * StarDict is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * StarDict is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with StarDict.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "stdio.h"
#include "stdlib.h"
#include <locale.h>
#include <string.h>
#include <sys/stat.h>
#include <ctype.h>

#include <glib.h>

void convert(char *filename)
{
	struct stat stats;
        if (stat (filename, &stats) == -1)
        {
                printf("file not exist!\n");
                return;
        }
        gchar *basefilename = g_path_get_basename(filename);
        FILE *jpgbfile;
        jpgbfile = fopen(filename,"r");

	gchar *buffer = (gchar *)g_malloc (stats.st_size + 1);
	size_t fread_size;
        fread_size = fread (buffer, 1, stats.st_size, jpgbfile);
	if (fread_size != (size_t)stats.st_size) {
		g_print("fread error!\n");
	}
        fclose (jpgbfile);
        buffer[stats.st_size] = '\0';

	gchar utffilename[256];
        sprintf(utffilename, "%s.utf", basefilename);
        FILE *utffile = fopen(utffilename,"w");

        GIConv gb2utf = g_iconv_open("UTF-8", "GBK");
        GIConv jp2utf = g_iconv_open("UTF-8", "SHIFT_JIS");

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
                p2 = strchr(p, '\t');
                if (!p2) {
                        g_print("Error %d: no tab\n", linenum);
			return;
		}
		*p2='\0';
                str = g_convert_with_iconv(p, -1, jp2utf, NULL, NULL, NULL);
                if (str) {
                        fwrite(str, strlen(str), 1, utffile);
                        g_free(str);
                } else {
                        g_print("Error1: %d\n", linenum);
                }
                fwrite("\t", 1, 1, utffile);
                p2++;
                str = g_convert_with_iconv(p2, -1, gb2utf, NULL, NULL, NULL);
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
        g_iconv_close(jp2utf);
        fclose(utffile);

        g_free(buffer);
        g_free(basefilename);	
}

int main(int argc,char * argv [])
{
        if (argc<2) {
                printf("please type this:\n./jpgb2utf JC_KDic.pdb.tab\n");
                return FALSE;
        }
	setlocale(LC_ALL, "");
        for (int i=1; i< argc; i++)
                convert (argv[i]);
        return FALSE;
}

