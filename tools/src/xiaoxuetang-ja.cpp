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

#include <glib.h>
#include <stdio.h>
#include <locale.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <string>

typedef struct _ParseUserData {
	FILE *tabfile;
	bool genbabylon;
	int cell_index;

	std::string word;
	std::string line;

	std::string oword;
	std::string pinyin;
	std::string jiazi;
	std::string lines;
} ParseUserData;

static void func_parse_start_element(GMarkupParseContext *context, const gchar *element_name, const gchar **attribute_names, const gchar **attribute_values, gpointer user_data, GError **error)
{
	if (strcmp(element_name, "Row")==0) {
		ParseUserData *Data = (ParseUserData *)user_data;
		Data->cell_index = 0;
		if (Data->genbabylon) {
			Data->oword.clear();
			Data->pinyin.clear();
			Data->jiazi.clear();
			Data->lines.clear();
		} else {
			Data->line.clear();
		}
	}
}

static void func_parse_end_element(GMarkupParseContext *context,
                          const gchar         *element_name,
                          gpointer             user_data,
                          GError             **error)
{
	if (strcmp(element_name, "Row")==0) {
		ParseUserData *Data = (ParseUserData *)user_data;
		if (Data->genbabylon) {
			std::string str;
			str = Data->oword;
			str += "|";
			str += Data->pinyin;
			if (Data->jiazi != Data->oword) {
				str += "|";
				str += Data->jiazi;
			}
			str += "\n";
			str += Data->pinyin;
			if (Data->jiazi != Data->oword) {
				str += " ";
				str += Data->jiazi;
			}
			str += "\\n";
			str += Data->lines;
			str += "\n\n";
			fputs(str.c_str(), Data->tabfile);
		} else {
			Data->line += '\n';
			fputs(Data->line.c_str(), Data->tabfile);
		}
	}
}

static void func_parse_text(GMarkupParseContext *context, const gchar *text, gsize text_len, gpointer user_data, GError **error)
{
	const gchar *element = g_markup_parse_context_get_element(context);
	if (!element) {
		return;
	}
	if (strcmp(element, "Data")==0) {
		ParseUserData *Data = (ParseUserData *)user_data;
		Data->cell_index++;
		if (Data->genbabylon) {
			if (Data->cell_index == 1) {
				std::string word;
				word.assign(text, text_len);
				const char *p = word.c_str();
				const char *p1 = strstr(word.c_str(), "【");
				if (!p1) {
					g_print("Warning cell 1: %s\n", word.c_str());
					return;
				}
				Data->pinyin.assign(p, p1-p);
				p1 += sizeof("【") -1;
				const char *p2 = strstr(p1, "】");
				if (!p2) {
					g_print("Warning cell 1: %s\n", word.c_str());
					return;
				}
				Data->oword.assign(p1, p2-p1);
			} else if (Data->cell_index == 2) {
				std::string word;
				word.assign(text, text_len);
				const char *p = word.c_str();
				const char *p1 = strstr(word.c_str(), "【");
				if (!p1) {
					Data->jiazi.assign(text, text_len);
				} else {
					Data->jiazi.assign(p, p1-p);
					p1 += sizeof("【") -1;
					const char *p2 = strstr(p1, "】");
					if (!p2) {
						g_print("Warning cell 2: %s\n", word.c_str());
						return;
					}
					std::string oword;
					oword.assign(p1, p2-p1);
					if (oword != Data->oword) {
						g_print("Warning cell 2: %s %s\n", Data->oword.c_str(), word.c_str());
						return;
					}
				}
			} else if (Data->cell_index == 3) {
				Data->lines.append(text, text_len);
			} else {
				Data->lines.append("\\n");
				Data->lines.append(text, text_len);
			}
		} else {
			if (Data->cell_index == 1) {
				Data->word.assign(text, text_len);
				Data->line.append(text, text_len);
				Data->line.append("\t");
			} else if (Data->cell_index == 2) {
				std::string word;
				word.assign(text, text_len);
				if (word != Data->word) {
					g_print("Synonym: %s %s\n", Data->word.c_str(), word.c_str());
				}
			} else if (Data->cell_index == 3) {
				Data->line.append(text, text_len);
			} else {
				Data->line.append("\\n");
				Data->line.append(text, text_len);
			}
		}
	}
}

void convert_excelxmlfile(const char *filename)
{
	struct stat stats;
	if (stat (filename, &stats) == -1) {
		printf("file not exist!\n");
		return;
	}
	int mmap_fd;
	if ((mmap_fd = open(filename, O_RDONLY)) < 0) {
		return;
	}
	char *data;
	data = (gchar *)mmap( NULL, stats.st_size, PROT_READ, MAP_SHARED, mmap_fd, 0);
	if ((void *)data == (void *)(-1)) {
		return;
	}
	ParseUserData Data;
	std::string tabfilename = filename;
	if (strcmp(filename, "Sheetjc.xml") == 0) {
		Data.genbabylon = true;
		tabfilename += ".babylon";
	} else {
		Data.genbabylon = false;
		tabfilename += ".tab";
	}
	Data.tabfile = fopen(tabfilename.c_str(), "wb");
	GMarkupParser parser;
	parser.start_element = func_parse_start_element;
	parser.end_element = func_parse_end_element;
	parser.text = func_parse_text;
	parser.passthrough = NULL;
	parser.error = NULL;
	GMarkupParseContext* context = g_markup_parse_context_new(&parser, (GMarkupParseFlags)0, &Data, NULL);
	g_markup_parse_context_parse(context, data, stats.st_size, NULL);
	g_markup_parse_context_end_parse(context, NULL);
	g_markup_parse_context_free(context);
	munmap(data, stats.st_size);
	close(mmap_fd);
	fclose(Data.tabfile);
	g_print("Generate: %s\n", tabfilename.c_str());
}

int main(int argc, char *argv[])
{
	if (argc<2) {
		printf("please type this:\n./xiaoxuetang-ja Sheetcj.xml\n");
		return false;
	}
	setlocale(LC_ALL, "");
	convert_excelxmlfile(argv[argc-1]);
	return false;
}
