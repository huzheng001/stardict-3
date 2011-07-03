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

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <glib.h>
#include <cstring>

#include <mysql.h>

#include <string>

void print_info(const char *info)
{
	g_print("%s", info);
}

typedef void (*print_info_t)(const char *info);

static void my_strstrip(char *str, glong linenum, print_info_t print_info)
{
	char *p1, *p2;
	p1=str;
	p2=str;
	while (*p1 != '\0') {
		if (*p1 == '\\') {
			p1++;
			if (*p1 == 'n') {
				*p2='\n';
				p2++;
				p1++;
				continue;
			}
			else if (*p1 == 'r') {
				gchar *str = g_strdup_printf("Warining: line %ld \\r\n", linenum);
				print_info(str);
				g_free(str);
				p1++;
				continue;
			}
			else if (*p1 == '\\') {
                                *p2='\\';
                                p2++;
                                p1++;
                                continue;
                        }
			else if (*p1 == '\0') {
				gchar *str = g_strdup_printf("Big warining: line %ld end by \\\n", linenum);
				print_info(str);
				g_free(str);
                                *p2='\\';
                                p2++;
                                continue;
			}
			else {
				gchar *str = g_strdup_printf("Warining: line %ld \\%c\n", linenum, *p1);
				print_info(str);
				g_free(str);
                                *p2='\\';
                                p2++;
                                *p2=*p1;
                                p2++;
				p1++;
				continue;
			}
		}
		else {
			*p2 = *p1;
			p2++;
			p1++;
			continue;
		}
	}
	*p2 = '\0';
}

static void convert_line(FILE *sqlfile, char *word, char *meaning)
{
	my_strstrip(meaning, 0, print_info);
	int word_len = strlen(word);
	char word_buf[word_len*2+3];
	int meaning_len = strlen(meaning);
	char meaning_buf[meaning_len*2+3];
	mysql_escape_string(word_buf, word, word_len);
	mysql_escape_string(meaning_buf, meaning, meaning_len);
	fputs("INSERT INTO dict (keyword, definition) VALUES('", sqlfile);
	fputs(word_buf, sqlfile);
	fputs("','", sqlfile);
	fputs(meaning_buf, sqlfile);
	fputs("');\n", sqlfile);
}

static void convert(const char *filename)
{
	struct stat stats;
	if (stat (filename, &stats) == -1) {
		print_info("File not exist!\n");
		return;
	}
	FILE *tabfile;
	tabfile = fopen(filename,"r");
	gchar *buffer = (gchar *)g_malloc (stats.st_size + 1);
	size_t readsize = fread (buffer, 1, stats.st_size, tabfile);
	fclose (tabfile);
	buffer[readsize] = '\0';
	FILE *sqlfile;
	std::string sqlfilename = filename;
	sqlfilename += ".sql";
	sqlfile = fopen(sqlfilename.c_str(), "wb");
	char *p, *p1, *p2;
	p = buffer;
	while (TRUE) {
		p1 = strchr(p, '\n');
		if (!p1)
			break;
		*p1 = '\0';
		p2 = strchr(p, '\t');
		if (!p2) {
			g_print("Error: no Tab.\n");
			break;
		}
		*p2 = '\0';
		p2++;
		convert_line(sqlfile, p, p2);
		p = p1 + 1;
	}
	fclose(sqlfile);
}

int main(int argc, char * argv[])
{
	if (argc<2) {
		printf("please type this:\n./tabfile2sql filename.txt\n");
		return FALSE;
	}
	convert(argv[1]);
	return FALSE;
}
