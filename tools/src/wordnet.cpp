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
#include <glib.h>
#include <string>
#include <cstring>


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

void convert(std::string &filename, std::string &content)
{
	gchar *contents;
	if (!g_file_get_contents(filename.c_str(), &contents, NULL, NULL)) {
		g_print("Open file %s failed!\n", filename.c_str());
		return;
	}
	char *p, *p1, *p2, *p3, *p4;
	p = contents;
	std::string meaning;
	std::string word;
	gchar *eword;
	unsigned int num;
	while (true) {
		p1 = strchr(p, '\n');
		if (!p1) {
			g_print("%s over.\n", filename.c_str());
			break;
		}
		*p1 = '\0';
		if (*p == ' ' && *(p+1) == ' ') {
			p = p1 + 1;
			continue;
		}
		meaning.clear();
		p2 = strchr(p, ' ');
		p2++;
		p2 = strchr(p2, ' ');
		p2++;
		meaning += "<type>";
		meaning += *p2;
		meaning += "</type>";
		p2+=2;
		num = htoi(p2);
		p3 = p2+3;
		if (num != 1) {
			meaning += "<wordgroup>";
		}
		for (unsigned int i = 0; i < num; i++) {
			if (i != 0) {
				content += '|';
			}
			p4 = strchr(p3, ' ');
			word.clear();
			int len = p4-p3;
			for (int j=0; j < len; j++) {
				if (p3[j] == '_')
					word += ' ';
				else if (p3[j] == '(')
					break;
				else
					word += p3[j];
			}
			content += word;
			if (num != 1) {
				meaning += "<word>";
				eword = g_markup_escape_text(word.c_str(), word.length());
				meaning += eword;
				g_free(eword);
				meaning += "</word>";
			}
			p3 = p4 + 3;
		}
		if (num != 1) {
			meaning += "</wordgroup>";
		}
		p4 = strchr(p3, '|');
		if (p4) {
			meaning += "<gloss>";
			p4++;
			g_strstrip(p4);
			eword = g_markup_escape_text(p4, -1);
			meaning += eword;
			g_free(eword);
			meaning += "</gloss>";
		}
		content += '\n';
		content += meaning;
		content += "\n\n";
		p = p1 + 1;
	}
	g_free(contents);
}

void convertdir(const char *dirname)
{
	std::string content;
	time_t t0;
	struct tm *t;
	time(&t0);
	t=gmtime(&t0);
	gchar *head = g_strdup_printf("\n#stripmethod=keep\n#sametypesequence=n\n#bookname=WordNet\n#website=http://wordnet.princeton.edu\n#date=%i.%02i.%02i\n#description=Made by Hu Zheng.\n\n", t->tm_year+1900, t->tm_mon+1, t->tm_mday);
	content = head;
	std::string filename;
	filename = dirname;
	filename += "/data.noun";
	convert(filename, content);
	filename = dirname;
	filename += "/data.adj";
	convert(filename, content);
	filename = dirname;
	filename += "/data.adv";
	convert(filename, content);
	filename = dirname;
	filename += "/data.verb";
	convert(filename, content);
	g_file_set_contents("wordnet.babylon", content.c_str(), content.length(), NULL);
	g_print("wordnet.babylon generated!\n");
}

int main(int argc, char *argv[])
{
	if (argc<2) {
		printf("please type this:\n./wordnet WordNet-3.0/dict\n");
		return FALSE;
	}
	convertdir(argv[1]);
	return FALSE;
}
