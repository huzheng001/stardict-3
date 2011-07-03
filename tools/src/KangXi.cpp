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
#include <string.h>
#include <glib.h>
#include <map>
#include <list>
#include <string>

static int hexalpha_to_int(int c)
{
	char hexalpha[] = "aAbBcCdDeEfF";
	int i;
	int answer = 0;

	for(i = 0; answer == 0 && hexalpha[i] != '\0'; i++) {
		if(hexalpha[i] == c) {
			answer = 10 + (i / 2);
		}
	}
	return answer;
}

static unsigned int htoi(const char s[])
{
	unsigned int answer = 0;
	int i = 0;
	int hexit;

	while(s[i] != '\0') {
		if(s[i] >= '0' && s[i] <= '9') {
			answer = answer * 16;
			answer = answer + (s[i] - '0');
		} else {
			hexit = hexalpha_to_int(s[i]);
			if(hexit == 0) {
				break;
			} else {
				answer = answer * 16;
				answer = answer + hexit;
			}
		}
		++i;
	}
	return answer;
}

std::map<int, std::list<std::string> > char_page_map;

void parse_line(const char *line)
{
	if (!g_str_has_prefix(line, "U+")) {
		g_print("Error: %s\n", line);
		return;
	}
	gunichar uc;
	uc = htoi(line+2);
	gchar utf8[7];
	gint n = g_unichar_to_utf8(uc, utf8);
	utf8[n] = '\0';
	const char *p;
	p = strchr(line, '\t');
	if (!p) {
		g_print("Error: %s\n", line);
		return;
	}
	p++;
	p = strchr(p, '\t');
	if (!p) {
		g_print("Error: %s\n", line);
		return;
	}
	int page = atoi(p + 1);
	std::map<int, std::list<std::string> >::iterator iter;
	iter = char_page_map.find(page);
	if (iter == char_page_map.end()) {
		g_print("Error: no page %d\n", page);
		return;
	} else {
		std::list<std::string> &charlist = iter->second;
		charlist.push_back(utf8);
	}
}

void gen_pages()
{
	std::list<std::string> charlist;
	for (int i = 1; i <= 1683; i++) {
		char_page_map[i] = charlist;
	}
}

void gen_babylon()
{
	FILE *file = fopen("KangXiZiDian.babylon", "wb");
	for (std::map<int, std::list<std::string> >::iterator iter = char_page_map.begin(); iter != char_page_map.end(); ++iter) {
		fprintf(file, "%d", iter->first);
		std::list<std::string> &charlist = iter->second;
		for (std::list<std::string>::iterator i = charlist.begin(); i != charlist.end(); ++i) {
			fprintf(file, "|%s", i->c_str());
		}
		if (iter->first == 1)
			fprintf(file, "\n<rref type=\"image\">%d.tif</rref>\\n<kref k=\"%d\">下一页</kref>\n\n", iter->first, iter->first+1);
		else if (iter->first == 1683)
			fprintf(file, "\n<rref type=\"image\">%d.tif</rref>\\n<kref k=\"%d\">上一页</kref>\n\n", iter->first, iter->first-1);
		else
			fprintf(file, "\n<rref type=\"image\">%d.tif</rref>\\n<kref k=\"%d\">上一页</kref>\t<kref k=\"%d\">下一页</kref>\n\n", iter->first, iter->first-1, iter->first+1);
	}
	fclose(file);
	g_print("File KangXiZiDian.babylon generated.\n");
}

int main(int argc, char *argv[])
{
	//Get KangXi.info by "grep kIRGKangXi Unihan.txt > KangXi.info"
	gchar *contents;
	if (!g_file_get_contents("KangXi.info", &contents, NULL, NULL))
		return false;
	gen_pages();
	char *p = contents;
	char *p1;
	while (true) {
		p1 = strchr(p, '\n');
		if (!p1)
			break;
		*p1 = '\0';
		parse_line(p);
		p = p1 + 1;
	}
	gen_babylon();
	g_free(contents);
}
