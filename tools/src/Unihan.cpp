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
#include <string>
#include <list>
#include <map>

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

std::map<std::string, std::list<std::pair<std::string, std::string> > > unihan_map;

void parse_line(char *line)
{
	if (line[0] == '#')
		return;

	if (!g_str_has_prefix(line, "U+")) {
		g_print("Error: %s\n", line);
		return;
	}
	char *han = line+2;

	char *p;
	p = strchr(han, '\t');
	if (!p) {
		g_print("Error: %s\n", line);
		return;
	}
	*p = '\0';
	p++;
	char *key = p;

	p = strchr(key, '\t');
	if (!p) {
		g_print("Error: %s\n", line);
		return;
	}
	*p = '\0';
	p++;
	char *def = p;

	std::map<std::string, std::list<std::pair<std::string, std::string> > >::iterator iter = unihan_map.find(han);
	if (iter == unihan_map.end()) {
		std::list<std::pair<std::string, std::string> > han_list;
		han_list.push_back(std::pair<std::string, std::string>(key, def));
		unihan_map[han] = han_list;
	} else {
		std::list<std::pair<std::string, std::string> > &han_list = iter->second;
		han_list.push_back(std::pair<std::string, std::string>(key, def));
	}
}

std::string get_definition(std::list<std::pair<std::string, std::string> > &keylist)
{
	std::string definition;
	if (keylist.empty()) {
		g_print("Error!\n");
		return definition;
	}
	for (std::list<std::pair<std::string, std::string> >::iterator iter = keylist.begin(); iter != keylist.end(); ++iter) {
		definition += "\\n";
		definition += iter->first;
		definition += "\t";
		definition += iter->second;
	}
	return definition;
}

void gen_tabfile()
{
	FILE *file = fopen("Unihan.tab", "wb");
	gunichar uc;
	gchar utf8[7];
	for (std::map<std::string, std::list<std::pair<std::string, std::string> > >::iterator iter = unihan_map.begin(); iter != unihan_map.end(); ++iter) {
		uc = htoi(iter->first.c_str());
		gint n = g_unichar_to_utf8(uc, utf8);
		utf8[n] = '\0';
		fprintf(file, "%s\tU+%s", utf8, iter->first.c_str());
		std::string definition = get_definition(iter->second);
		fprintf(file, "%s\n", definition.c_str());
	}
	fclose(file);
	g_print("File Unihan.tab generated.\n");
}

int main(int argc, char *argv[])
{
	//Get Unihan.txt from ftp://ftp.unicode.org/Public/UNIDATA/Unihan.zip
	gchar *contents;
	if (!g_file_get_contents("Unihan.txt", &contents, NULL, NULL))
		return false;
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
	gen_tabfile();
	g_free(contents);
}
