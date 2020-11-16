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

#include <gtk/gtk.h>
#include <glib.h>

#include <list>
#include <string>

struct _worditem
{
	gchar *word;
	gchar *definition;
};

gint stardict_strcmp(const gchar *s1, const gchar *s2)
{
        gint a;
        a = g_ascii_strcasecmp(s1, s2);
        if (a == 0)
                return strcmp(s1, s2);
        else
                return a;
}

gint comparefunc(gconstpointer a,gconstpointer b)
{
        gint x;
        x = stardict_strcmp(((struct _worditem *)a)->word,((struct _worditem *)b)->word);
        if (x == 0)
                return ((struct _worditem *)a)->definition - ((struct _worditem *)b)->definition;
        else
                return x;
}

gchar *get_cdata(gchar *data)
{
	if (!g_str_has_prefix(data, "<![CDATA[")) {
		if (!g_str_has_prefix(data, "[CDATA[")) {
			g_print("Error, unknow cdata: %s\n", data);
			return NULL;
		} else {
			g_print("Warning: %s\n", data);
			data += strlen("[CDATA[");
		}
	} else {
		data += strlen("<![CDATA[");
	}
	gchar *p = strstr(data, "]]");
	if (!p) {
		g_print("Error, unknow cdata end: %s\n", data);
		return NULL;
	}
	*p='\0';
	/*if (strcmp(data, "被朝鲜")==0) {
		strcpy(data, "北朝鲜");
	}*/
	g_strstrip(data);
	return data;
}

struct _synworditem
{
        gchar *synword;
        gchar *origword;
        gchar *definition;
};

gint comparefunc2(gconstpointer a,gconstpointer b)
{
        gint x;
        x = stardict_strcmp(((struct _synworditem *)a)->synword,((struct _synworditem *)b)->synword);
        if (x == 0)
                return ((struct _worditem *)a)->definition - ((struct _worditem *)b)->definition;
        else
                return x;
}

typedef struct _ParseUserData {
	gchar *word;
	gchar *definition;
	std::list<std::string> *WordList;
	GArray *array;
	std::list<std::string> *TagList;
	std::list<std::string> *ElementList;
} ParseUserData;

static void func_parse_passthrough(GMarkupParseContext *context, const gchar *passthrough_text, gsize text_len, gpointer user_data, GError **error)
{
	gchar *text = g_strndup(passthrough_text, text_len);
	if (!(g_str_has_prefix(text, "<![CDATA[") && (g_str_has_suffix(text, "]]>")))) {
		g_print("Wrong, not CDATA: %s\n", text);
		g_free(text);
                return;
	}
	const gchar *element = g_markup_parse_context_get_element(context);
        if (!element) {
		g_print("Wrong, no element: %s\n", text);
		g_free(text);
                return;
	}
	ParseUserData *Data = (ParseUserData *)user_data;
	gchar *p = strchr(text, '&');
	if (p && *(p+1)!='\0') {
		p++;
		gchar *n = g_utf8_next_char(p);
		if (*n == '{') {
			std::string temp(p, n-p);
			bool find = false;
			for (std::list<std::string>::const_iterator it=Data->TagList->begin(); it!=Data->TagList->end(); ++it) {
				if (*it == temp) {
					find = true;
					break;
				}
			}
			if (!find) {
				g_print("Find mark tag: %s - %s\n", temp.c_str(), Data->word);
				Data->TagList->push_back(temp);
			}
		}
	}
	//if (strcmp(element, "单词原型")==0) {
	if (strcmp(element, "YX")==0) {
		gchar *str = g_strdup(text);
		gchar *oword = get_cdata(str);
		if (strcmp(Data->word, oword)) {
			bool find = false;
			for (std::list<std::string>::const_iterator it=Data->WordList->begin(); it!=Data->WordList->end(); ++it) {
				if (*it == oword) {
					//g_print("Same word: %s\n", oword);
					find= true;
					break;
				}
			}
			if (!find) {
				Data->WordList->push_back(oword);
				struct _synworditem synworditem;
				synworditem.synword = g_strdup(oword);
				synworditem.origword = Data->word;
				synworditem.definition = Data->definition;
				g_array_append_val(Data->array, synworditem);
			}
		}
		g_free(str);
	//} else if (strcmp(element, "单词")==0) {
	} else if (strcmp(element, "DC")==0) {
	//} else if (strcmp(element, "词典音标")==0) {
	} else if (strcmp(element, "CB")==0) {
	//} else if (strcmp(element, "单词词性")==0) {
	} else if (strcmp(element, "DX")==0) {
	//} else if (strcmp(element, "解释项")==0) {
	} else if (strcmp(element, "JX")==0) {
	//} else if (strcmp(element, "跟随解释")==0) {
	} else if (strcmp(element, "GZ")==0) {
	} else if (strcmp(element, "相关词")==0) {
	} else if (strcmp(element, "预解释")==0) {
	} else if (strcmp(element, "繁体写法")==0) {
	//} else if (strcmp(element, "汉语拼音")==0) {
	} else if (strcmp(element, "PY")==0) {
	} else if (strcmp(element, "台湾音标")==0) {
	//} else if (strcmp(element, "例句原型")==0) {
	} else if (strcmp(element, "LY")==0) {
	//} else if (strcmp(element, "例句解释")==0) {
	} else if (strcmp(element, "LS")==0) {
	} else if (strcmp(element, "图片名称")==0) {
	} else if (strcmp(element, "跟随注释")==0) {
	//} else if (strcmp(element, "音节分段")==0) {
	} else if (strcmp(element, "YD")==0) {
	} else if (strcmp(element, "AHD音标")==0) {
	} else if (strcmp(element, "国际音标")==0) {
	} else if (strcmp(element, "美国音标")==0) {
	} else if (strcmp(element, "子解释项")==0) {
	} else if (strcmp(element, "同义词")==0) {
	} else if (strcmp(element, "日文发音")==0) {
	} else if (strcmp(element, "惯用型原型")==0) {
	} else if (strcmp(element, "惯用型解释")==0) {
	} else if (strcmp(element, "另见")==0) {
	} else {
		g_print("Warning: Unknow tag: %s - %s - %s\n", element, Data->word, text);
	}
	g_free(text);
}

static void func_parse_start_element(GMarkupParseContext *context, const gchar *element_name, const gchar **attribute_names, const gchar **attribute_values, gpointer user_data, GError **error)
{
	ParseUserData *Data = (ParseUserData *)user_data;
	bool find = false;
	for (std::list<std::string>::const_iterator it=Data->ElementList->begin(); it!=Data->ElementList->end(); ++it) {
		if (*it == element_name) {
			find = true;
			break;
		}
	}
	if (!find) {
		g_print("Find element: %s - %s\n", element_name, Data->word);
		Data->ElementList->push_back(element_name);
	}
}

static void func_parse_error(GMarkupParseContext *context, GError *error, gpointer user_data)
{
	ParseUserData *Data = (ParseUserData *)user_data;
	g_print("Parse error: %s\n%s\n", Data->word, error->message);
}

void parse_definition(gchar *p, ParseUserData *Data)
{
	GMarkupParser parser;
        parser.start_element = func_parse_start_element;
        parser.end_element = NULL;
        parser.text = NULL;
        parser.passthrough = func_parse_passthrough;
        parser.error = func_parse_error;
        GMarkupParseContext* context = g_markup_parse_context_new(&parser, (GMarkupParseFlags)0, Data, NULL);
        g_markup_parse_context_parse(context, p, strlen(p), NULL);
        g_markup_parse_context_end_parse(context, NULL);
        g_markup_parse_context_free(context);
}

void builddata(gchar *datafilename, glong &wordcount, glong &idxfilesize, glong &synwordcount, std::list<std::string> *TagList, std::list<std::string> *ElementList)
{
	struct stat stats;
	if (stat (datafilename, &stats) == -1) {
		printf("File %s not exist!\n", datafilename);
		return;
	}
	FILE *datafile;
	datafile = fopen(datafilename,"r");
	gchar *buffer = (gchar *)g_malloc (stats.st_size + 1);
	size_t fread_size;
	fread_size = fread (buffer, 1, stats.st_size, datafile);
	if (fread_size != (size_t)stats.st_size) {
		g_print("fread error!\n");
	}
	fclose (datafile);
	buffer[stats.st_size] = '\0';

	GArray *array = g_array_sized_new(FALSE,FALSE, sizeof(struct _worditem),20000);
	GArray *array2 = g_array_sized_new(FALSE,FALSE, sizeof(struct _synworditem),20000);
	gchar *p, *p1, *p2, *p3, *p4, *p5;
	p = buffer;
	struct _worditem worditem;
	while (1) {
		//p1 = strstr(p, "<单词块>"); //Powerword 2007
		p1 = strstr(p, "<CK>"); //Powerword 2012
		if (!p1) {
			g_print("over\n");
			break;
		}
		//p1 += strlen("<单词块>");
		p1 += strlen("<CK>");
		//p2 = strstr(p1, "</单词块>");
		p2 = strstr(p1, "</CK>");
		if (!p2) {
			//g_print("Error, no </单词块>\n");
			g_print("Error, no </CK>\n");
			return;
		}
		*p2='\0';
		//p2 += strlen("</单词块>");
		p2 += strlen("</CK>");
		//p3 = strstr(p1, "<单词>");
		p3 = strstr(p1, "<DC>");
		p5 = p1;
		while (g_ascii_isspace(*p5))
			p5++;
		if (p5!=p3) {
			//g_print("Warning, not begin with <单词>.\n");
			g_print("Warning, not begin with <DC>.\n");
		}
		if (!p3) {
			//g_print("Error, no <单词>\n");
			g_print("Error, no <DC>\n");
			return;
		}
		//p3 += strlen("<单词>");
		p3 += strlen("<DC>");
		//p4 = strstr(p3, "</单词>");
		p4 = strstr(p3, "</DC>");
		if (!p4) {
			//g_print("Error, no </单词>\n");
			g_print("Error, no </DC>\n");
			return;
		}
		*p4='\0';
		//p4 += strlen("</单词>");
		p4 += strlen("</DC>");
		worditem.word = get_cdata(p3);
		if (!worditem.word) {
			return;
		}
		if (!worditem.word[0]) {
			g_print("Bad word!\n");
			p = p2;
			continue;
		}
		while (g_ascii_isspace(*p4)) {
			p4++;
		}
		worditem.definition = p4;
		g_strstrip(worditem.definition);
		if (!worditem.definition[0]) {
			g_print("Bad definition!\n");
			return;
		}
		ParseUserData Data;
		Data.word = worditem.word;
		Data.definition = worditem.definition;
		std::list<std::string> WordList;
		Data.WordList = &WordList;
		Data.array = array2;
		Data.TagList = TagList;
		Data.ElementList = ElementList;
		parse_definition(worditem.definition, &Data);
		g_array_append_val(array, worditem);
		p = p2;
	}
	g_array_sort(array,comparefunc);
	g_array_sort(array2,comparefunc2);

	gchar *basefilename = g_strdup(datafilename);
	p = strchr(basefilename, '.');
	if (p)
		*p='\0';
	gchar idxfilename[256];
	gchar dicfilename[256];
	sprintf(idxfilename, "powerword2012_%s.idx", basefilename);
	sprintf(dicfilename, "powerword2012_%s.dict", basefilename);
	FILE *idxfile = fopen(idxfilename,"w");
	FILE *dicfile = fopen(dicfilename,"w");

	guint32 offset_old;
        guint32 tmpglong;
        struct _worditem *pworditem;         gint definition_len;
        gulong i;
        for (i=0; i< array->len; i++) {
                offset_old = ftell(dicfile);
                pworditem = &g_array_index(array, struct _worditem, i);
                definition_len = strlen(pworditem->definition);
                fwrite(pworditem->definition, 1 ,definition_len,dicfile);
                fwrite(pworditem->word,sizeof(gchar),strlen(pworditem->word)+1,idxfile);
                tmpglong = g_htonl(offset_old);
                fwrite(&(tmpglong),sizeof(guint32),1,idxfile);
                tmpglong = g_htonl(definition_len);
                fwrite(&(tmpglong),sizeof(guint32),1,idxfile);
        }
	idxfilesize = ftell(idxfile);
        fclose(idxfile);
	fclose(dicfile);
	g_print("%s wordcount: %d\n", datafilename, array->len);
	wordcount = array->len;

	synwordcount = array2->len;
	if (array2->len) {
                gchar synfilename[256];
                sprintf(synfilename, "powerword2012_%s.syn", basefilename);
                FILE *synfile = fopen(synfilename,"w");
                struct _synworditem *psynworditem;
                gint iFrom, iTo, iThisIndex, cmpint;
                bool bFound;
                for (i=0; i< array2->len; i++) {
                        psynworditem = &g_array_index(array2, struct _synworditem, i);
                        fwrite(psynworditem->synword, 1, strlen(psynworditem->synword)+1, synfile);
			g_free(psynworditem->synword);
                        bFound=false;
                        iFrom=0;
                        iTo=array->len-1;
                        while (iFrom<=iTo) {
                                iThisIndex=(iFrom+iTo)/2;
                                pworditem = &g_array_index(array, struct _worditem, iThisIndex);
                                cmpint = stardict_strcmp(psynworditem->origword, pworditem->word);
                                if (cmpint>0)
                                        iFrom=iThisIndex+1;
                                else if (cmpint<0)
                                        iTo=iThisIndex-1;
                                else {
                                        bFound=true;
                                        break;
                                }

                        }
                        if (!bFound) {
                                g_print("Error, %s not find.\n", psynworditem->origword);
                                return;
                        }
                        do {
                                if (iThisIndex==0)
                                        break;
                                pworditem = &g_array_index(array, struct _worditem, iThisIndex-1);
				if (strcmp(psynworditem->origword, pworditem->word)==0)
                                        iThisIndex--;
                                else
                                        break;
                        } while (true);
                        bFound=false;
                        do {
                                pworditem = &g_array_index(array, struct _worditem, iThisIndex);
                                if (strcmp(psynworditem->origword, pworditem->word)==0) {
                                        if (psynworditem->definition == pworditem->definition) {
                                                bFound=true;
                                                break;
                                        } else
                                                iThisIndex++;
                                } else
                                        break;
                        } while (true);
                        if (!bFound) {
                                g_print("Error, %s definition not find.\n", psynworditem->origword);
                                return;
                        }
                        tmpglong = g_htonl(iThisIndex);
                        fwrite(&(tmpglong),sizeof(guint32),1, synfile);
                }
                fclose(synfile);
                g_print("synwordcount: %d\n", array2->len);
        }

	g_free(basefilename);

	g_free(buffer);
	g_array_free(array,TRUE);
	g_array_free(array2,TRUE);

	gchar command[1024];
	sprintf(command, "dictzip %s", dicfilename);
	int result;
	result = system(command);
	if (result == -1) {
		g_print("system() error!\n");
	}
}

void buildifo(gchar *xmlfilename, glong wordcount, glong idxfilesize, glong synwordcount)
{
	struct stat stats;
        if (stat (xmlfilename, &stats) == -1) {
                printf("File %s not exist!\n", xmlfilename);
                return;
        }
        FILE *xmlfile;
        xmlfile = fopen(xmlfilename,"r");
        gchar *buffer = (gchar *)g_malloc (stats.st_size + 1);
	size_t fread_size;
        fread_size = fread (buffer, 1, stats.st_size, xmlfile);
	if (fread_size != (size_t)stats.st_size) {
		g_print("fread error!\n");
	}
        fclose (xmlfile);
        buffer[stats.st_size] = '\0';

	const gchar *bookname="";
	gchar *p = buffer;
	gchar *p1, *p2, *p3;
	p1 = strstr(p, "<cp>936</cp>"); //Powerword 2012
	//p1 = strstr(p, "<cp_936>"); //Powerword 2007
	if (!p1) {
		g_print("Error, <cp>936</cp> not found!\n"); //Powerword 2012
		//g_print("Error, <cp_936> not found!\n"); //Powerword 2007
	} else {
		p1+=strlen("<cp>936</cp>"); //Powerword 2012
		//p1+=strlen("<cp_936>"); //Powerword 2007
		p2 = strstr(p1, "<name>");
		if (!p2) {
			g_print("Error, <name> not found!\n");
		} else {
			p2+=strlen("<name>");
			p3 = strstr(p2, "</name>");
			if (!p3) {
				g_print("Error, </name> not found!\n");
			} else {
				*p3 = '\0';
				bookname = p2;
			}
		}
	}
	gchar *synstr;
	if (synwordcount)
		synstr = g_strdup_printf("synwordcount=%ld\n", synwordcount);
	else
		synstr = g_strdup("");
	gchar *basefilename = g_strdup(xmlfilename);
        p = strchr(basefilename, '.');
        if (p)
                *p='\0';
        gchar ifofilename[256];
        sprintf(ifofilename, "powerword2012_%s.ifo", basefilename);
        FILE *ifofile = fopen(ifofilename,"w");
	gchar *content = g_strdup_printf("StarDict's dict ifo file\nversion=2.4.2\nwordcount=%ld\n%sidxfilesize=%ld\nbookname=%s\nauthor=金山软件股份公司\ndescription=Enjoy!\ndate=2015.1.26\nsametypesequence=k\n", wordcount, synstr, idxfilesize, bookname);
	g_free(synstr);
	fwrite(content, strlen(content), 1, ifofile);
	g_free(content);
	fclose(ifofile);
	g_free(buffer);

	gchar command[256];
        sprintf(command, "mkdir stardict-powerword2012_%s-2.4.2", basefilename);
	int result;
        result = system(command);
	if (result == -1) {
		g_print("system() error!\n");
	}
	sprintf(command, "mv powerword2012_%s.idx stardict-powerword2012_%s-2.4.2", basefilename, basefilename);
	result = system(command);
	if (result == -1) {
		g_print("system() error!\n");
	}
	if (synwordcount) {
		sprintf(command, "mv powerword2012_%s.syn stardict-powerword2012_%s-2.4.2", basefilename, basefilename);
		result = system(command);
		if (result == -1) {
			g_print("system() error!\n");
		}
	}
	sprintf(command, "mv powerword2012_%s.dict.dz stardict-powerword2012_%s-2.4.2", basefilename, basefilename);
        result = system(command);
	if (result == -1) {
		g_print("system() error!\n");
	}
	sprintf(command, "mv powerword2012_%s.ifo stardict-powerword2012_%s-2.4.2", basefilename, basefilename);
	result = system(command);
	if (result == -1) {
		g_print("system() error!\n");
	}
	sprintf(command, "chmod 644 stardict-powerword2012_%s-2.4.2/*", basefilename);
	result = system(command);
	if (result == -1) {
		g_print("system() error!\n");
	}
	sprintf(command, "chown root.root -R stardict-powerword2012_%s-2.4.2", basefilename);
	result = system(command);
	if (result == -1) {
		g_print("system() error!\n");
	}
	sprintf(command, "tar -cjvf stardict-powerword2012_%s-2.4.2.tar.bz2 stardict-powerword2012_%s-2.4.2", basefilename, basefilename);
	result = system(command);
	if (result == -1) {
		g_print("system() error!\n");
	}
	sprintf(command, "rm -rf stardict-powerword2012_%s-2.4.2", basefilename);
	result = system(command);
	if (result == -1) {
		g_print("system() error!\n");
	}

        g_free(basefilename);
}

gchar *encodefilename(gchar *filename)
{
	std::string code;
	while (*filename) {
		if (*filename == ' ')
			code+="\\ ";
		else
			code+=*filename;
		filename++;
	}
	return g_strdup(code.c_str());
}

bool convert_utf(gchar *filename, gchar *utffilename)
{
	if (g_file_test(utffilename, G_FILE_TEST_EXISTS))
		return false;
	FILE *datafile;
	datafile = fopen(filename, "r");
	FILE *utffile;
	utffile = fopen(utffilename, "w");
	GIConv unicode2utf = g_iconv_open("UTF-8", "UNICODE");
	guint32 len1, len2, len;
	gchar *unicode, *str;
	size_t fread_size;
	while (!feof(datafile)) {
		fread_size = fread(&len1, sizeof(guint32), 1, datafile);
		if (fread_size != 1) {
			g_print("fread error!\n");
		}
		fread_size = fread(&len2, sizeof(guint32), 1, datafile);
		if (fread_size != 1) {
			g_print("fread error 2!\n");
		}
		len = len1*len2;
		if (len==0) {
			continue;
		}
		unicode = (gchar *)g_malloc(len);
		fread_size = fread(unicode, 1, len, datafile);
		if (fread_size != len) {
			g_print("fread error!\n");
		}
		str = g_convert_with_iconv(unicode, len, unicode2utf, NULL, NULL, NULL);
                if (!str) {
                        g_print("UNICODE to UTF-8 error!\n");
                } else {
                        fwrite(str, strlen(str), 1, utffile);
			g_free(str);
                }
		g_free(unicode);
	}
	g_iconv_close(unicode2utf);
	fclose (datafile);
	fclose(utffile);
	if (strcmp(utffilename, "pwdnnjsj.da3.utf")==0) {
		g_print("Doing sed process.\n");
		gchar command[256];
		sprintf(command, "mv %s %s.bad", utffilename, utffilename);
		int result;
		result = system(command);
		if (result == -1) {
			g_print("system() error!\n");
		}
		sprintf(command, "sed -e s//\\ / -e s//\\ / -e s//\\ / -e s/]/]]/ -e s/絔]/]]/ -e s/]/]]/ -e s/╙]/]]/ -e s/輂]/]]/ -e s/]/]]/ -e s/謁]/]]/ %s.bad > %s", utffilename, utffilename);
		result = system(command);
		if (result == -1) {
			g_print("system() error!\n");
		}
		sprintf(command, "rm %s.bad", utffilename);
		result = system(command);
		if (result == -1) {
			g_print("system() error!\n");
		}
	}
	g_print("Convert over!\n");
	return false;
}

void convert_dir(gchar *dictdirname, std::list<std::string> *TagList, std::list<std::string> *ElementList)
{
	g_print("Building: %s\n", dictdirname);
	gchar *basename = g_path_get_basename(dictdirname);
	gchar *xmlfilename = g_strdup_printf("%s/%s.xml", dictdirname, basename);
	gchar *datafilename = g_strdup_printf("%s/%s.da3", dictdirname, basename);
	struct stat stats;
	if (stat (xmlfilename, &stats) == -1) {
		g_print("Error, xml file %s don't exists!", xmlfilename);
		return;
	}
	if (stat (datafilename, &stats) == -1) {
		g_print("Error, data file %s don't exists!", datafilename);
		return;
	}
	gchar *utfxmlfilename = g_strdup_printf("%s.xml.utf", basename);
	gchar *utfdatafilename = g_strdup_printf("%s.da3.utf", basename);
	gchar command[256];
	gchar *code = encodefilename(xmlfilename);
	sprintf(command, "iconv -f GBK -t UTF-8 %s -o %s", code, utfxmlfilename);
	g_free(code);
	if (system(command)!=0) {
		g_print("Error, convert %s to UTF-8 failed!", xmlfilename);
		return;
	}
	if (convert_utf(datafilename, utfdatafilename)) {
		g_print("Error, convert %s to UTF-8 failed!", datafilename);
		return;
	}
	glong wordcount, idxfilesize, synwordcount;
	builddata(utfdatafilename, wordcount, idxfilesize, synwordcount, TagList, ElementList);
	buildifo(utfxmlfilename, wordcount, idxfilesize, synwordcount);

	sprintf(command, "rm %s", utfxmlfilename);
	int result;
	result = system(command);
	if (result == -1) {
		g_print("system() error!\n");
	}
	sprintf(command, "rm %s", utfdatafilename);
	//system(command);

	g_free(xmlfilename);
	g_free(datafilename);
	g_free(basename);
}

void convert(char *dirname)
{
	std::list<std::string> TagList;
	std::list<std::string> ElementList;
	GDir *dir = g_dir_open(dirname, 0, NULL);
	if (!dir) {
		g_print("Open %s failed!", dirname);
		return;
	}
	const gchar *filename;
	gchar fullfilename[256];
	while ((filename = g_dir_read_name(dir))!=NULL) {
		sprintf(fullfilename, "%s/%s", dirname, filename);
		if (g_file_test(fullfilename, G_FILE_TEST_IS_DIR)) {
			convert_dir(fullfilename, &TagList, &ElementList);
		} else if (g_str_has_suffix(filename, ".xml")) {
			convert_dir(dirname, &TagList, &ElementList);
		}
	}
	g_dir_close(dir);
	g_print("Element:");
	for (std::list<std::string>::const_iterator it=ElementList.begin(); it!=ElementList.end(); ++it) {
		g_print(" %s", it->c_str());
	}
	g_print("\n");
	g_print("Tags:");
	for (std::list<std::string>::const_iterator it=TagList.begin(); it!=TagList.end(); ++it) {
		g_print("%s", it->c_str());
	}
	g_print("\n");
}

int main(int argc,char * argv [])
{
        if (argc!=2) {
                printf("please type this:\n./kingsoft /mnt/e/Program Files/kingsoft/PowerWord\\ 2012/dicts\n");
                return FALSE;
        }
	setlocale(LC_ALL, "");
        convert (argv[1]);
	return FALSE;
}
