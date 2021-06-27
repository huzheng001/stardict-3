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

struct _worditem
{
	gchar *word;
	gchar *pinyin;
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

void to_definition(gchar *str)
{
	while (*str) {
		if (*str=='/')
			*str='\n';
		str++;
	}
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
		
	gchar *p, *p1, *p2, *p3;
	p = buffer;
	if ((guchar)*p==0xEF && (guchar)*(p+1)==0xBB && (guchar)*(p+2)==0xBF) // UTF-8 order characters.
		p+=3;
	struct _worditem worditem;
	glong linenum=1;
	while (1) {
		if (*p == '\0') {
                        g_print("over\n");
                        break;
                }
		p1 = strchr(p,'\n');
		if (!p1) {
			g_print("error, no end line\n");
			return;
		}
		*p1 = '\0';
		p1++;
		p2 = strchr(p,'[');
		if (!p2) {
			p2 = strchr(p, '/');
			if (!p2) {
				g_print("error, no /, %ld\n", linenum);
				return;
			}
			*p2 = '\0';
			p3 = p2+1;
		} else {
			*p2 = '\0';
			p2++;
			p3 = strchr(p2, ']');
			if (!p3) {
				g_print("error, no ], %ld\n", linenum);
				return;
			}
			*p3 = '\0';
			p3++;
		}
		worditem.word = p;
		worditem.pinyin = p2;
		to_definition(p3);
		worditem.definition = p3;
		g_strstrip(worditem.word);
		g_strstrip(worditem.pinyin);
		g_strstrip(worditem.definition);
		if (!worditem.word[0]) {
			g_print("%s-%ld, bad word!!!\n", basefilename, linenum);
			p= p1;
                	linenum++;
			continue;
		}
		if (!worditem.pinyin[0]) {
			//g_print("%s-%ld, bad pinyin!!!\n", basefilename, linenum);
		}
		if (!worditem.definition[0]) {
			g_print("%s-%ld, bad definition!!!\n", basefilename, linenum);
		}
		g_array_append_val(array, worditem);			
		p= p1;				
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
	const gchar *insert_word = "\n\n";
	gboolean flag;
	pworditem = &g_array_index(array, struct _worditem, i);
	gint pinyin_len, definition_len;
	gboolean firsttime;
	while (i<array->len)
	{
		thedatasize = 0; 
		offset_old = ftell(dicfile);
		flag = true;
		firsttime = TRUE;
		while (flag == true)
		{	
			pinyin_len = strlen(pworditem->pinyin);
			if (firsttime) {
				fwrite(pworditem->pinyin, 1 , pinyin_len+1,dicfile);
			} else {
				fwrite(pworditem->pinyin, 1 , pinyin_len,dicfile);
				fwrite("\n", 1, 1, dicfile);
			}
			definition_len = strlen(pworditem->definition);
			fwrite(pworditem->definition, 1 ,definition_len,dicfile);
			thedatasize += pinyin_len+1+definition_len;
			previous_word = pworditem->word;
						
			i++;
			if (i<array->len)
			{
				pworditem = &g_array_index(array, struct _worditem, i);
				if (strcmp(previous_word,pworditem->word)==0)
				{
					//g_print("D! %s\n",previous_word);
					flag = true;
					firsttime=FALSE;
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
	
	g_print("%s wordcount: %ld\n", basefilename, wordcount);

	g_free(buffer);
	g_array_free(array,TRUE);
	
	fclose(idxfile);
	fclose(dicfile);

	gchar command[256];
        sprintf(command, "dictzip %s.dict", basefilename);
	int result;
        result = system(command);
	if (result == -1) {
		g_print("system() error!\n");
	}

	g_free(basefilename);
}

int main(int argc,char * argv [])
{
	if (argc<2) {
		printf("please type this:\n./edict edict.utf\n");
		return FALSE;
	}

	setlocale(LC_ALL, "");
	for (int i=1; i< argc; i++)
		convert (argv[i]);
	return FALSE;	
}

