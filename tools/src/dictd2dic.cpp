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

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "stdio.h"
#include "stdlib.h"
#include <locale.h>
#include <string.h>
#include <sys/stat.h>


#include <gtk/gtk.h>
#include <glib.h>

#define DICTD_WEBSITE "freedict2"
//#define DICTD_WEBSITE "www.dict.org"
//#define DICTD_WEBSITE "www.freedict.de"
//#define DICTD_WEBSITE "www.mova.org"

struct _worditem
{
	gchar *word;
	gchar *definition;
	glong size;
};

static unsigned char b64_list[] =
"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

#define XX 100

static int b64_index[256] = {
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,62, XX,XX,XX,63,
    52,53,54,55, 56,57,58,59, 60,61,XX,XX, XX,XX,XX,XX,
    XX, 0, 1, 2,  3, 4, 5, 6,  7, 8, 9,10, 11,12,13,14,
    15,16,17,18, 19,20,21,22, 23,24,25,XX, XX,XX,XX,XX,
    XX,26,27,28, 29,30,31,32, 33,34,35,36, 37,38,39,40,
    41,42,43,44, 45,46,47,48, 49,50,51,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
};

const char *b64_encode( unsigned long val )
{
   static char   result[7];
   int    i;

   result[0] = b64_list[ (val & 0xc0000000) >> 30 ];
   result[1] = b64_list[ (val & 0x3f000000) >> 24 ];
   result[2] = b64_list[ (val & 0x00fc0000) >> 18 ];
   result[3] = b64_list[ (val & 0x0003f000) >> 12 ];
   result[4] = b64_list[ (val & 0x00000fc0) >>  6 ];
   result[5] = b64_list[ (val & 0x0000003f)       ];
   result[6] = 0;

   for (i = 0; i < 5; i++) if (result[i] != b64_list[0]) return result + i;
   return result + 5;
}

unsigned long b64_decode( const char *val )
{
   unsigned long v = 0;
   int           i;
   int           offset = 0;
   int           len = strlen( val );

   for (i = len - 1; i >= 0; i--) {
      int tmp = b64_index[ (unsigned char)val[i] ];

      /*if (tmp == XX)
	 err_internal( __FUNCTION__,
		       "Illegal character in base64 value: `%c'\n", val[i] );*/
      
      v |= tmp << offset;
      offset += 6;
   }

   return v;
}

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

void convert(char *basefilename)
{			
	gchar indexfilename[256];
	gchar dictfilename[256];
	sprintf(indexfilename, "%s.index", basefilename);
	sprintf(dictfilename, "%s.dict", basefilename);
	
	struct stat stats;
	if (stat (indexfilename, &stats) == -1)
	{
		printf("index file not exist!\n");
		return;
	}
	FILE *indexfile;
	indexfile = fopen(indexfilename,"r");
	gchar *buffer = (gchar *)g_malloc (stats.st_size + 1);
	size_t fread_size;
	fread_size = fread (buffer, 1, stats.st_size, indexfile);
	if (fread_size != (size_t)stats.st_size) {
		g_print("fread error!\n");
	}
	fclose (indexfile);
	buffer[stats.st_size] = '\0';

	gchar dictdzfilename[1024];
	sprintf(dictdzfilename, "%s.dz", dictfilename);
	if (stat (dictdzfilename, &stats) == -1) {
	} else {
		gchar dictgzfilename[1024];
		sprintf(dictgzfilename, "%s.gz", dictfilename);
		gchar command[3000];
		sprintf(command, "mv %s %s", dictdzfilename, dictgzfilename);
		int result;
		result = system(command);
		if (result == -1) {
			g_print("system() error!\n");
		}
		sprintf(command, "gunzip %s", dictgzfilename);
		result = system(command);
		if (result == -1) {
			g_print("system() error!\n");
		}
	}

	if (stat (dictfilename, &stats) == -1) {
		printf("dict file not exist!\n");
		return;
	}

	FILE *dictfile;
	dictfile = fopen(dictfilename,"r");
	gchar *buffer1 = (gchar *)g_malloc (stats.st_size + 1);
	fread_size = fread (buffer1, 1, stats.st_size, dictfile);
	if (fread_size != (size_t)stats.st_size) {
		g_print("fread error!\n");
	}
	fclose (dictfile);
	buffer1[stats.st_size] = '\0';

	
	GArray *array = g_array_sized_new(FALSE,FALSE, sizeof(struct _worditem),20000);
		
	gchar *p, *p1, *p2, *p3, *p4;
	p = buffer;
	struct _worditem worditem;
	glong linenum=1;
	gulong d_start,d_size;
	gint word_len;
	while (1) {
		if (*p == '\0') {
			g_print("over\n");
			break;
		}
		p1 = strchr(p,'\t');
		if (p1) {
			*p1 = '\0';
		}
		else {
			g_print("Error! No tab char 1 found! %ld\n", linenum);
			return;
		}
		p2 = strchr(p1+1,'\t');
		if (p2) {
			*p2 = '\0';
		}
		else {
			g_print("Error! No tab char 2 found! %ld\n", linenum);
			return;
		}
		p3 = strchr(p2+1,'\n');
		if (p3) {
			*p3 = '\0';
			p4 = strchr(p2+1,'\t');
			if (p4) {
				//Maybe need to export it to .syn file!
				*p4 = '\0';
			}
		}
		else {
			g_print("Error! No end up new line found %ld\n", linenum);
			return;
		}
		worditem.word = p;
		g_strstrip(worditem.word);
		if (g_str_has_prefix(p, "00-database-")) {
			p= p3+1;
			linenum++;
			continue;
		}
		if (!g_utf8_validate(worditem.word, -1,NULL)) {
			g_print("word %s convert to utf8 error!\n",worditem.word);			
			p= p3+1;
			linenum++;
			continue;
		}
		d_start = b64_decode(p1+1);
		d_size = b64_decode(p2+1);
		
		worditem.definition = buffer1 + d_start;
		word_len = strlen(worditem.word);
		if (strncmp(worditem.word, worditem.definition, word_len)==0 && worditem.definition[word_len]=='\n') {
			worditem.definition = worditem.definition + word_len + 1;
			d_size = d_size - word_len - 1;
		}
		while (d_size && g_ascii_isspace(*(worditem.definition+d_size-1))) {
			d_size--; // remove end up new-line.
		}
		worditem.size = d_size;
		if (!g_utf8_validate(worditem.definition, d_size,NULL)) {
			g_print("word definition %s convert to utf8 error!\n",worditem.word);	
			p= p3+1;
                	linenum++;
			continue;
		}
		if ((!worditem.word[0]) || (d_size==0)) {
			p= p3+1;
                        linenum++;
                        continue;
		}
		g_array_append_val(array, worditem);			
		p= p3+1;				
		linenum++;
	}	
	g_array_sort(array,comparefunc);
	
	gchar ifofilename[256];
	sprintf(ifofilename, "dictd_" DICTD_WEBSITE "_%s.ifo", basefilename);
	g_print("File: %s\n", ifofilename);
	FILE *ifofile = fopen(ifofilename,"w");
	gchar idxfilename[256];
	sprintf(idxfilename, "dictd_" DICTD_WEBSITE "_%s.idx", basefilename);
	g_print("File: %s\n", idxfilename);
	FILE *idxfile = fopen(idxfilename,"w");
	gchar dicfilename[256];
	sprintf(dicfilename, "dictd_" DICTD_WEBSITE "_%s.dict", basefilename);
	g_print("File: %s\n", dicfilename);
	FILE *dicfile = fopen(dicfilename,"w");

	glong tmpglong = 0;
#if 0
	fwrite(&(tmpglong),sizeof(glong),1,idxfile);		
#endif	
	glong wordcount = array->len;
	long offset_old;
	const gchar *previous_word = "";
	struct _worditem *pworditem;
	gulong i=0;
	glong thedatasize;
	const gchar *insert_word = "\n\n";
	gboolean flag;
	pworditem = &g_array_index(array, struct _worditem, i);
	while (i<array->len)
	{
		thedatasize = 0; 
		offset_old = ftell(dicfile);
		flag = true;
		while (flag == true)
		{	
			fwrite(pworditem->definition, 1 ,pworditem->size,dicfile);
			thedatasize += pworditem->size;
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
		//g_print("size: %ld\n", thedatasize);
		fwrite(&(tmpglong),sizeof(glong),1,idxfile);	
	}
#if 0	
	fseek(idxfile,0,SEEK_SET);
	tmpglong = g_htonl(wordcount);
	fwrite(&(tmpglong),sizeof(glong),1,idxfile);
#endif	
	g_print("wordcount: %ld\n",wordcount);

	g_free(buffer);
	g_free(buffer1);
	g_array_free(array,TRUE);
	
	fclose(idxfile);
	fclose(dicfile);	
	
	stat(idxfilename, &stats);
	fprintf(ifofile, "StarDict's dict ifo file\nversion=2.4.2\nwordcount=%ld\nidxfilesize=%ld\nbookname=%s\nsametypesequence=m\n", wordcount, stats.st_size, basefilename);
	fclose(ifofile);

	gchar command[1024];
	sprintf(command, "dictzip dictd_" DICTD_WEBSITE "_%s.dict", basefilename);
	int result;
	result = system(command);
	if (result == -1) {
		g_print("system() error!\n");
	}
}

int
main(int argc,char * argv [])
{
	if (argc!=2) {
		printf("please type this:\n./dictd2dic eng-fra\n");
		return FALSE;
	}

	setlocale(LC_ALL, "");
	convert (argv[1]);
	return FALSE;
	
}
