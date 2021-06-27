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

//this file is not completed....as now i know pydict use xdict in fact...i already have the xdict convert tool....

#include "stdio.h"
#include "stdlib.h"
#include <locale.h>
#include <string.h>
#include <sys/stat.h>

#include <glib.h>

struct _worditem
{
	gchar *word;
	gchar *data;
	glong datasize;
};

gint comparefunc(gconstpointer a,gconstpointer b)
{
	return g_ascii_strcasecmp(((struct _worditem *)a)->word,((struct _worditem *)b)->word);
}

void convert(char *filename)
{	
	struct stat stats;
	FILE *pyfile,*ecidxfile,*ecdicfile,*ceidxfile,*cedicfile;
	gchar *buffer;
	GArray *ecarray;
	struct _worditem ecworditem;
	GArray *cearray;
	struct _worditem ceworditem;
	gchar *p1,*p2,*p3,*p4;
	gchar *utf_str;
	gsize write_size;
	glong word_len;
	long offset_old;
	glong wordcount;
	glong i;
	gchar *previous_word = "";
	struct _worditem *pworditem;
	
	if (stat (filename, &stats) == -1)
	{
		printf("file not exist!\n");
		return;
	}
	pyfile = fopen(filename,"r");

	buffer = (gchar *)g_malloc (stats.st_size + 2);
	size_t fread_size;
	fread_size = fread (buffer, 1, stats.st_size, pyfile);
	if (fread_size != (size_t)stats.st_size) {
		g_print("fread error!\n");
	}
	fclose (pyfile);
	buffer[stats.st_size] = '\n';	
	buffer[stats.st_size+1] = '\0';	
	
	ecarray = g_array_sized_new(FALSE,FALSE, sizeof(struct _worditem),177785); //pydict.lib have 177785 lines.
	cearray = g_array_sized_new(FALSE,FALSE, sizeof(struct _worditem),177785);
	
	p1 = buffer;

	while (1) {
		if (*p1=='\0') {
			g_print("over\n");
			break;
		}
		p2 = strchr(p1,'\n');
		if (p2) {
			*p2 = '\0';
		}
		else {
			g_print("error!\n");
			return;
		}
		p3 = strchr(p1,'=');
		if (p3) {
			*p3 = '\0';
		}
		else {
			g_print("error! no meaning\n");
			return;
		}
		p3++;
		p4 = strchr(p3,'=');
		if (p4) {
			*p4 = '\0';
		}
		else {
			g_print("error! meaning no end\n");
			return;
		}
		g_strstrip(p1);
		if (*p1=='\0') {
			p1 = p2+1;
			g_print("bad word\n");
			continue;
		}
		word_len = strlen(p1);
		if (!g_utf8_validate(p1,word_len,NULL)) {
			p1 = p2+1;
			g_print("%s convert to utf8 error!\n",p1);
			continue;
		}		
		g_strstrip(p3);
		if (*p3=='\0') {
			p1 = p2+1;
			g_print("bad meaning\n");
			continue;
		}					
		utf_str = g_locale_to_utf8(p3,-1,NULL,&write_size,NULL);
		if (!utf_str) {
				g_print("%s 's meaning convert to utf8 error!\n",p1);
				p1 = p2+1;
				continue;
		}
		ecworditem.word = p1;
		ecworditem.datasize = sizeof(gchar)+sizeof(glong)+ write_size;
		ecworditem.data = g_malloc(ecworditem.datasize);
		memcpy(ecworditem.data,"M",sizeof(gchar));
		memcpy(ecworditem.data+sizeof(gchar),&(write_size),sizeof(glong));
		memcpy(ecworditem.data+sizeof(gchar)+sizeof(glong),utf_str,write_size);			
		g_array_append_val(ecarray, ecworditem);			
		
		ceworditem.word = utf_str;
		ceworditem.datasize = sizeof(gchar)+sizeof(glong)+ word_len;
		ceworditem.data = g_malloc(ceworditem.datasize);
		memcpy(ceworditem.data,"M",sizeof(gchar));
		memcpy(ceworditem.data+sizeof(gchar),&(word_len),sizeof(glong));
		memcpy(ceworditem.data+sizeof(gchar)+sizeof(glong),p1,word_len);
		g_array_append_val(cearray, ceworditem);		

		//need to read mark!!!!		
		
		p1= p2+1;				
	}		
	g_array_sort(ecarray,comparefunc);
	g_array_sort(cearray,comparefunc);
	
	ecidxfile = fopen("pydict-gb-ec.idx","w");
	ecdicfile = fopen("pydict-gb-ec.dict","w");
	ceidxfile = fopen("pydict-gb-ce.idx","w");
	cedicfile = fopen("pydict-gb-ce.dict","w");	
	fprintf(ecidxfile,"stardict's idx file\nbookname=pydict-gb-ec\nbeigin:\n");
	fprintf(ceidxfile,"stardict's idx file\nbookname=pydict-gb-ce\nbeigin:\n");

	/*ecidxfile = fopen("pydict-big5-ec.idx","w");
	ecdicfile = fopen("pydict-big5-ec.dict","w");
	ceidxfile = fopen("pydict-big5-ce.idx","w");
	cedicfile = fopen("pydict-big5-ce.dict","w");	
	fprintf(ecidxfile,"stardict's idx file\ndictfile=pydict-big5-ec.dict\nbookname=pydict-big5-ec\nbeigin:\n");
	fprintf(ceidxfile,"stardict's idx file\ndictfile=pydict-big5-ce.dict\nbookname=pydict-big5-ce\nbeigin:\n");
	*/
	
	
	wordcount = ecarray->len;
	for (i=0;i<ecarray->len;i++)
	{
		pworditem = &g_array_index(ecarray, struct _worditem, i);
		
		if (strcmp(previous_word,pworditem->word)==0) {
			g_print("word %s is duplicated! droped!\n" ,previous_word);
			wordcount--;
			continue;
		}
		previous_word = pworditem->word;
		
		offset_old = ftell(ecdicfile);
		fwrite(pworditem->data,sizeof(gchar),pworditem->datasize,ecdicfile);
		
		fwrite(pworditem->word,sizeof(gchar),strlen(pworditem->word)+1,ecidxfile);
		fwrite(&(offset_old),sizeof(glong),1,ecidxfile);
		fwrite(&(pworditem->datasize),sizeof(glong),1,ecidxfile);	
		
		g_free(pworditem->data);		
	}	
	fwrite(&(wordcount),sizeof(glong),1,ecidxfile);
	g_print("ec wordcount: %ld\n\n",wordcount);

	wordcount = cearray->len;
	for (i=0;i<cearray->len;i++)
	{
		pworditem = &g_array_index(cearray, struct _worditem, i);
		
		if (strcmp(previous_word,pworditem->word)==0) {
			g_print("word %s is duplicated! droped!\n" ,previous_word);
			wordcount--;
			continue;
		}
		previous_word = pworditem->word;
		
		offset_old = ftell(cedicfile);
		fwrite(pworditem->data,sizeof(gchar),pworditem->datasize,cedicfile);
		
		fwrite(pworditem->word,sizeof(gchar),strlen(pworditem->word)+1,ceidxfile);
		fwrite(&(offset_old),sizeof(glong),1,ceidxfile);
		fwrite(&(pworditem->datasize),sizeof(glong),1,ceidxfile);	
		
		g_free(pworditem->word);
		g_free(pworditem->data);		
	}	
	fwrite(&(wordcount),sizeof(glong),1,ceidxfile);
	g_print("ce wordcount: %ld\n",wordcount);

	g_free(buffer);
    g_array_free(ecarray,TRUE);
	g_array_free(cearray,TRUE);
	
	fclose(ecidxfile);
	fclose(ecdicfile);	
	fclose(ceidxfile);
	fclose(cedicfile);	
}

int
main(int argc,char * argv [])
{
	if (argc!=2) {
		printf("please type this:\n./pydict2dic pydict.lib\n");
		return FALSE;
	}

	setlocale(LC_ALL, "");
	convert (argv[1]);
	return FALSE;
	
}
