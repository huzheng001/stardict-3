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

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <glib.h>
#include <string>
#include <cstring>

void convert(const char *filename)
{
	struct stat stats;
	if (stat (filename, &stats) == -1) {
		printf("File not find.\n");
		return;
	}
	FILE *mbfile;
	mbfile = fopen(filename, "r");
	char *buffer = (char *)malloc (stats.st_size + 1);
	size_t fread_size;
	fread_size = fread (buffer, 1, stats.st_size, mbfile);
	if (fread_size != (size_t)stats.st_size) {
		g_print("fread error!\n");
	}
	fclose (mbfile);
	buffer[stats.st_size] = '\0';
	std::string newfilename = filename;
	newfilename += ".babylon";
	FILE *babylonfile = fopen(newfilename.c_str(), "w");
	char *p = strstr(buffer, "[Text]\r\n");
	p += sizeof("[Text]\r\n") -1;
	char *p1, *p2;
	std::string hanzi, codes, synonyms;
	while (true) {
		p1 = strstr(p, "\r\n");
		if (!p1)
			break;
		*p1 = '\0';
		p2 = g_utf8_next_char(p);
		hanzi.assign(p, p2 -p);
		codes.assign(p2, p1 - p2);
		synonyms.clear();
		int len = p1 - p2;
		for (int i = 0; i <len; i++) {
			if (codes[i] == ' ')
				synonyms += '|';
			else
				synonyms += codes[i];
		}
		fprintf(babylonfile, "%s|%s\n%s\n\n", hanzi.c_str(), synonyms.c_str(), codes.c_str());
		p = p1 + 2;
	}
	fclose(babylonfile);
	free(buffer);
	printf("Write %s\n", newfilename.c_str());
}

int main(int argc, char *argv[])
{
	if (argc<2) {
		printf("please type this:\n./wubi Sun98.utf\n");
		return FALSE;
	}
	convert(argv[1]);
	return FALSE;
}
