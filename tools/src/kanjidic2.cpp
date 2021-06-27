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

#include <glib.h>

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
	return stardict_strcmp(((struct _worditem *)a)->word,((struct _worditem *)b)->word);
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
	FILE *tabfile;
	tabfile = fopen(filename,"r");

	gchar *buffer = (gchar *)g_malloc (stats.st_size + 1);
	size_t fread_size;
	fread_size = fread (buffer, 1, stats.st_size, tabfile);
	if (fread_size != (size_t)stats.st_size) {
		g_print("fread error!\n");
	}
	fclose (tabfile);
	buffer[stats.st_size] = '\0';	
	
	GArray *array = g_array_sized_new(FALSE,FALSE, sizeof(struct _worditem),20000);
		
	gchar *p, *p1, *p2, *p3, *p4, *p5;
	p = buffer;
	struct _worditem worditem;
	glong linenum=1;
	std::string def;
	while (1) {
		p1 = strstr(p,"<character>");
		if (!p1) {
			g_print("over\n");
			break;
		}
		p1+=11;
		p2 = strstr(p1,"<literal>");
		if (!p2) {
			g_print("error, no <literal>\n");
			return;
		}
		p2+=9;
		p3 = strstr(p2, "</literal>");
		if (!p3) {
			g_print("error, no </literal>\n");
			return;
		}
		*p3='\0';
		p3+=10;
		worditem.word = p2;
		p5 = strstr(p3, "</character>");
		if (!p5) {
			g_print("error:%s no </character>\n", worditem.word);
			return;
		}
		*p5='\0';
		p5+=12;
		def.clear();
		do {
			p4 = strstr(p3, "<reading");
			if (p4) {
				if (*(p4+8)=='_') {
					p3+=8;
				} else if (strcmp(p4, "<reading_meaning>")==0) {
					p3+=8;
				} else {
					if (*(p4+8)=='>' || (*(p4+8)==' ' && *(p4+9)=='r')) {
					} else {
						g_print("Wrong\n");
					}
					p4+=8;
					p1 = strchr(p4, '>');
					if (!p1) {
						g_print("error, no >\n");
						return;
					}
					p1++;
					p2 = strstr(p1, "</reading>");
					if (!p2) {
						g_print("error:%s no </reading>\n", worditem.word);
						return;
					}
					*p2='\0';
					def += p1;
					def += '\n';
					p3=p2+10;
				}
			}
		} while (p4);
		def += '\n';
		do {
			p4 = strstr(p3, "<meaning");
			if (p4) {
				p4+=8;
				p1 = strchr(p4, '>');
				if (!p1) {
					g_print("error, no >\n");
					return;
				}
				p1++;
				p2 = strstr(p1, "</meaning>");
				if (!p2) {
					g_print("error, no </meaning>\n");
					return;
				}
				*p2='\0';
				def += p1;
				def += '\n';
				p3=p2+10;
			}
		} while (p4);
		worditem.definition = g_strdup(def.c_str());
		g_strstrip(worditem.word);
		g_strstrip(worditem.definition);
		if (!worditem.word[0]) {
			g_print("%s-%ld, bad word!!!\n", basefilename, linenum);
			return;
		}
		if (!worditem.definition[0]) {
			//g_print("%s-%ld: %s, bad definition!\n", basefilename, linenum, worditem.word);
			p=p5;
			continue;
		}
		g_array_append_val(array, worditem);			
		p= p5;				
		linenum++;
	}		
	g_array_sort(array,comparefunc);
		
	gchar idxfilename[256];
	gchar dicfilename[256];
	sprintf(idxfilename, "%s.idx", basefilename);
	sprintf(dicfilename, "%s.dict", basefilename);
	FILE *idxfile = fopen(idxfilename,"w");
	FILE *dicfile = fopen(dicfilename,"w");

	
	glong wordcount = array->len;

	long offset_old;
	glong tmpglong;
	const gchar *previous_word = "";
	struct _worditem *pworditem;
	gulong i=0;
	glong thedatasize;
	const gchar *insert_word = "\n";
	gboolean flag;
	pworditem = &g_array_index(array, struct _worditem, i);
	gint definition_len;
	while (i<array->len)
	{
		thedatasize = 0; 
		offset_old = ftell(dicfile);
		flag = true;
		while (flag == true)
		{	
			definition_len = strlen(pworditem->definition);
			fwrite(pworditem->definition, 1 ,definition_len,dicfile);
			thedatasize += definition_len;
			previous_word = pworditem->word;
						
			i++;
			if (i<array->len)
			{
				pworditem = &g_array_index(array, struct _worditem, i);
				if (strcmp(previous_word,pworditem->word)==0)
				{
					g_print("D! %s\n",previous_word);
					flag = true;
					wordcount--;
					fwrite(insert_word,sizeof(gchar),strlen(insert_word),dicfile);
					thedatasize += strlen(insert_word);
				}
				else 
				{
					flag = false;
				}
			}
			else
			{
				flag = false;
			}
		}
		fwrite(previous_word,sizeof(gchar),strlen(previous_word)+1,idxfile);
		tmpglong = g_htonl(offset_old);
		fwrite(&(tmpglong),sizeof(glong),1,idxfile);
		tmpglong = g_htonl(thedatasize);
		fwrite(&(tmpglong),sizeof(glong),1,idxfile);	
	}
	for (i=0; i<array->len; i++) {
		pworditem = &g_array_index(array, struct _worditem, i);
		g_free(pworditem->definition);
	}
		
	g_print("%s wordcount: %ld\n", basefilename, wordcount);

	g_free(buffer);
	g_array_free(array,TRUE);
	
	fclose(idxfile);
	fclose(dicfile);

	g_free(basefilename);
}

int main(int argc,char * argv [])
{
	if (argc<2) {
		printf("please type this:\n./kanjidic2 kanjidic2.xml\n");
		return FALSE;
	}

	setlocale(LC_ALL, "");
	for (int i=1; i< argc; i++)
		convert (argv[i]);
	return FALSE;	
}

