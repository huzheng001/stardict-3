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

#include <string>

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
	size_t fread_size;
        fread_size = fread (buffer, 1, stats.st_size, gmxfile);
	if (fread_size != (size_t)stats.st_size) {
		g_print("fread error!\n");
	}
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
	setlocale(LC_ALL, "");
	for (int i=1; i< argc; i++)
                convert (argv[i]);
        return FALSE;
}

