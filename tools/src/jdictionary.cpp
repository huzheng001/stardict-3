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

// This program was written for jdictionary, http://www.jdictionary.info/
// But i find its data is too chaos, the GerEng which i want is almost the same as dictd_www.freedict.de_deu-eng.
// So i have droped this program.

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

void save_file(gboolean isFirst, GArray *array, gchar *basefilename)
{
	gchar ifofilename[256];
	gchar idxfilename[256];
	gchar dicfilename[256];
	if (isFirst) {
		sprintf(ifofilename, "jdictionary_%s.ifo", basefilename);
		sprintf(idxfilename, "jdictionary_%s.idx", basefilename);
		sprintf(dicfilename, "jdictionary_%s.dict", basefilename);
	}
	else {
		sprintf(ifofilename, "jdictionary_%s_2.ifo", basefilename);
		sprintf(idxfilename, "jdictionary_%s_2.idx", basefilename);
		sprintf(dicfilename, "jdictionary_%s_2.dict", basefilename);
	}
	FILE *ifofile = fopen(ifofilename,"w");
	FILE *idxfile = fopen(idxfilename,"w");
	FILE *dicfile = fopen(dicfilename,"w");

	
	glong wordcount = array->len;

	long offset_old;
	const gchar *previous_word = "";
	struct _worditem *pworditem;
	gulong i=0;
	glong thedatasize;
	glong tmpglong;
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
	fclose(idxfile);
	fclose(dicfile);
		
	struct stat stats;
	stat (idxfilename, &stats);
	gchar *idxheadbuffer;
	idxheadbuffer = g_strdup_printf("StarDict's dict ifo file\nversion=2.4.2\n"
		"wordcount=%ld\nidxfilesize=%ld\nbookname=jdictionary_%s\n"
		"date=2003.11.14\nsametypesequence=m\n",
		wordcount, (long) stats.st_size, basefilename);
	fwrite(idxheadbuffer, 1, strlen(idxheadbuffer), ifofile);
	g_free(idxheadbuffer);
	fclose(ifofile);
	
	g_print("%s wordcount: %ld\n", basefilename, wordcount);
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
	gchar separater;
	if (strcmp(basefilename, "EngHun") == 0 || strcmp(basefilename, "GerEng") == 0 || strcmp(basefilename, "GerHun") == 0)
		separater = ':';
	else
		separater = '|';
	gboolean isGerEng = !strcmp(basefilename, "GerEng");
	
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
	
	gchar *tmp_str;
	
	GArray *array = g_array_sized_new(FALSE,FALSE, sizeof(struct _worditem),20000);
	GArray *array2 = g_array_sized_new(FALSE,FALSE, sizeof(struct _worditem),20000);
		
	gchar *p, *p1, *p2;
	p = buffer;
	struct _worditem worditem;
	glong linenum=1;
	while (1) {
		if (*p == '\0') {
			//g_print("over\n");
			break;
		}
		p1 = strchr(p,separater);
		if (p1) {
			*p1 = '\0';
		}
		else {
			g_print("error! not tab char found! %ld\n", linenum);
			return;
		}
		p2 = strchr(p1+1,'\n');
		if (p2) {
			*p2 = '\0';
		}
		else {
			g_print("error! not end up new line found %ld\n", linenum);
			return;
		}
		worditem.word = p;
		worditem.definition = p1+1;
		p1 = strrchr(worditem.definition,separater);
		if (p1) {			
			*p1 = '\0';
			if (strchr(worditem.definition,separater)) {
				g_print("error! too much separater char found! %ld %s\n", linenum, worditem.definition);
				//return;
			}
		}
		else {
			g_print("error! not second separater char found! %ld\n", linenum);
			return;
		}
		worditem.word_need_free = false;
		worditem.definition_need_free = false;
		g_strstrip(worditem.word);
		g_strstrip(worditem.definition);
		if (!worditem.word[0]) {
			g_print("%s, %ld, bad word!!!\n", basefilename, linenum);
			return;
		}
		if (!worditem.definition[0]) {
			g_print("%s, %ld, bad definition!!!\n", basefilename, linenum);
			return;
		}
		if (!g_utf8_validate(worditem.word, -1,NULL)) {
			g_print("%s, %ld, word %s is not utf8!\n", basefilename, linenum, worditem.word);			
			return;
		}
		if (!g_utf8_validate(worditem.definition, -1,NULL)) {
			g_print("%s, %ld, definition %s is not utf8!\n", basefilename, linenum, worditem.definition);	
			return;
		}
		if (isGerEng) {
			g_array_append_val(array, worditem);
			tmp_str = worditem.word;
			worditem.word = worditem.definition;
			worditem.definition = tmp_str;
			g_array_append_val(array2, worditem);			
		}
		else {
			g_array_append_val(array, worditem);
			tmp_str = worditem.word;
			worditem.word = worditem.definition;
			worditem.definition = tmp_str;
			g_array_append_val(array2, worditem);
		}
		p= p2+1;				
		linenum++;
	}		
	g_array_sort(array,comparefunc);
	g_array_sort(array2,comparefunc);
	
	save_file(true, array, basefilename);	
	save_file(false, array2, basefilename);

	g_free(buffer);
    g_array_free(array,TRUE);
	g_array_free(array2,TRUE);

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
