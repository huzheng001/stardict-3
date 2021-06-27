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
	gchar *definition;
	gboolean word_need_free;
	gboolean definition_need_free;
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
	
	GIConv latin1_converter;
	latin1_converter = g_iconv_open("UTF-8","LATIN1");
	gchar *utf8_str;
	
	GArray *array = g_array_sized_new(FALSE,FALSE, sizeof(struct _worditem),20000);
		
	gchar *p, *p1, *p2;
	p = buffer;
	struct _worditem worditem;
	glong linenum=1;
	while (1) {
		if (*p == '\0') {
			//g_print("over\n");
			break;
		}
		p1 = strchr(p,'\n');
		if (p1) {
			*p1 = '\0';
		}
		else {
			g_print("error! no end up new line found! %ld\n", linenum);
			return;
		}
		p2 = strchr(p,'\t');
		if (p2) {
			*p2 = '\0';
		}
		else {
			g_print("error! not tab char found %ld\n", linenum);
			p= p1+1;
                        linenum++;
			continue;
		}
		worditem.word = p;
		worditem.definition = p2+1;
		worditem.word_need_free = false;
		worditem.definition_need_free = false;
		g_strstrip(worditem.word);
		g_strstrip(worditem.definition);
		if (!worditem.word[0]) {
			g_print("%s, bad word!!!\n", basefilename);
			return;
		}
		if (!worditem.definition[0]) {
			g_print("%s-%ld, bad definition!!!\n", basefilename, linenum);
			p= p1+1;
	                linenum++;
			continue;
		}
		if (!g_utf8_validate(worditem.word, -1,NULL)) {
			utf8_str = g_convert_with_iconv(worditem.word, -1,latin1_converter,NULL, NULL,NULL);
			if (utf8_str) {
				worditem.word_need_free = true;		
				worditem.word = utf8_str;
			}
			else {
				g_print("%s, word %s convert to utf8 error!\n", basefilename, worditem.word);			
				return;
			}
		}
		if (!g_utf8_validate(worditem.definition, -1,NULL)) {
			utf8_str = g_convert_with_iconv(worditem.definition, -1,latin1_converter,NULL, NULL,NULL);
			if (utf8_str) {
				worditem.definition_need_free = true;
				worditem.definition = utf8_str;
			}
			else {
				g_print("%s, definition %s convert to utf8 error!\n", basefilename, worditem.definition);	
				return;
			}
		}	
		g_array_append_val(array, worditem);			
		p= p1+1;				
		linenum++;
	}		
	g_iconv_close(latin1_converter);
	g_array_sort(array,comparefunc);
		
	gchar idxfilename[256];
	gchar dicfilename[256];
	sprintf(idxfilename, "quick_%s.idx", basefilename);
	sprintf(dicfilename, "quick_%s.dict", basefilename);
	FILE *idxfile = fopen(idxfilename,"w");
	FILE *dicfile = fopen(dicfilename,"w");

	gchar *idxheadbuffer;
	idxheadbuffer = g_strdup_printf("StarDict's idx file\nversion=2.1.0\nbookname=quick_%s\ndate=2003.05.27\nsametypesequence=m\nBEGIN:\n",basefilename);
	fwrite(idxheadbuffer, 1, strlen(idxheadbuffer), idxfile);
	g_free(idxheadbuffer);
	glong old_wordcount_offset = ftell(idxfile);
	glong tmpglong = 0;
	fwrite(&(tmpglong),sizeof(glong),1,idxfile);
	
	
	
	glong wordcount = array->len;

	long offset_old;
	const gchar *previous_word = "";
	struct _worditem *pworditem;
	gulong i=0;
	glong thedatasize;
	//gchar *insert_word = "\n\n";
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
					//g_print("D! %s\n",previous_word);
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
				flag = false;
		}
		fwrite(previous_word,sizeof(gchar),strlen(previous_word)+1,idxfile);
		tmpglong = g_htonl(offset_old);
		fwrite(&(tmpglong),sizeof(glong),1,idxfile);
		tmpglong = g_htonl(thedatasize);
		fwrite(&(tmpglong),sizeof(glong),1,idxfile);	
	}	
	for (i= 0;i < array->len; i++) {
		pworditem = &g_array_index(array, struct _worditem, i);
		if (pworditem->word_need_free)
			g_free(pworditem->word);
		if (pworditem->definition_need_free)
			g_free(pworditem->definition);
	}	
		
	fseek(idxfile,old_wordcount_offset,SEEK_SET);
	tmpglong = g_htonl(wordcount);
	fwrite(&(tmpglong),sizeof(glong),1,idxfile);
	
	g_print("%s wordcount: %ld\n", basefilename, wordcount);

	g_free(buffer);
    g_array_free(array,TRUE);
	
	fclose(idxfile);
	fclose(dicfile);

	g_free(basefilename);
}

int
main(int argc,char * argv [])
{
	if (argc<2) {
		printf("please type this:\n./wquick2dict *\n");
		return FALSE;
	}

	setlocale(LC_ALL, "");
	for (int i=1; i< argc; i++)
		convert (argv[i]);
	return FALSE;
	
}
