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

//oxford.txt is come from http://igloo.its.unimelb.edu.au/Blowfish/
//but we have fixed some GB2312->UTF-8 problem and some other small fix.if you need this file, mail to huzheng_001@163.com

//notice, after the oxford.idxdata is generated,then do:
//cat oxford.idxhead oxford.idxdata > oxford.idx

#include "stdio.h"
#include "stdlib.h"
#include <locale.h>
#include <string.h>
#include <sys/stat.h>

#include <string>

#include <glib.h>

struct _worditem
{
	gchar *word;
	gchar *data;
	glong datasize;
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

/*
// It is really hard to determine which is phonetic just by "/ " and "/", i.e. the meaning of "to".
gchar *to_utf8_phonetic(gchar *text, gchar *orin_word)
{
	std::string return_text;
	gchar *old_start = text;
	gchar *start, *end;
	while ((start = strchr(old_start, '/')) != NULL) {
		if ((*(start+1) == ' ') || (*(start+1) == '-')) {
		}
		else {
			for (int i= 0; i<= start - old_start; i++) {
				return_text += old_start[i];
			}
			old_start = start+1;
			continue;
		}
		for (int i= 0; i< start - old_start; i++) {
			return_text += old_start[i];
		}
		return_text += '/';
		return_text += *(start+1);
		end = strchr(start + 1, '/');
		if (!end) {
			g_print("wrong2: %s | %s\n", orin_word, text);
			return g_strdup("");
		}
		*end = '\0';	
	
		const gchar *s, *old_s;
		s = start+1;
		gchar tmpstr[16];
		gint i = 0;
		gint len = strlen(s);
		gint str_len;
		gboolean processed;	
		while (i < len) {
			processed = true;
			switch (*s) {			
			case 'A':
				return_text+="æ"; break;
			case 'B':
				return_text+="ɑ"; break;
			case 'C':
				return_text+="ɒ"; break;
			case 'Q':
				return_text+="ʌ"; break;
			case 'E':
				return_text+="ә"; break;
			case 'e':
				return_text+="є"; break;
			case 'N':
				return_text+="ŋ"; break;
			case 'W':
				return_text+="θ"; break;
			case 'T':
				return_text+="ð"; break;
			case 'F':
				return_text+="ʃ"; break;
			case 'V':
				return_text+="ʒ"; break;						
			//case 0x:
				//return_text+="ː"; break;						
			//case 0x:
				//return_text+="ɡ"; break;
			case '9':
				return_text+="ˏ"; break;
			case '5':
				return_text+="ˋ"; break;
			//case 0x:
				//return_text+="ˊ"; break;
			default:
				processed = false;
			}
			old_s = s;
			s = g_utf8_next_char(s);
			str_len = s - old_s;
			i+= str_len;
			if (!processed) {
				strncpy(tmpstr, old_s, str_len);
				tmpstr[str_len] = '\0';
				return_text += tmpstr;
			}
		}
		return_text+= "/";
		
		old_start = end +1;
	}
	while (*old_start) {
		return_text += *old_start;
		old_start++;
	}

	return g_strdup(return_text.c_str());
}*/

void convert(char *filename)
{	
	struct stat stats;
	FILE *oxfile,*idxfile,*dicfile;
	gchar *buffer;
	GArray *array= NULL;
	struct _worditem worditem;
	gchar *p1,*p2;
	glong line_num = 0;
	gchar *utf_str;
	gsize write_size;
	glong tmpglong=0;
	long offset_old;	
	glong wordcount;
	glong thedatasize, flag, wordlength;
	const gchar *previous_word = "";
	const gchar *insert_word = "\n\n";
	struct _worditem *pworditem;
	
	if (stat (filename, &stats) == -1)
	{
		printf("file not exist!\n");
		return;
	}
	oxfile = fopen(filename,"r");

	buffer = (gchar *)g_malloc (stats.st_size + 2);
	size_t fread_size;
	fread_size = fread (buffer, 1, stats.st_size, oxfile);
	if (fread_size != (size_t)stats.st_size) {
		g_print("fread error!\n");
	}
	fclose (oxfile);
	buffer[stats.st_size] = '\n';	
	buffer[stats.st_size+1] = '\0';	
	
	array = g_array_sized_new(FALSE,FALSE, sizeof(struct _worditem),39000); //oxford.txt have 77304 lines.
	
	
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
		if (line_num % 2 == 0) {			
			if (!g_utf8_validate(p1,p2-p1,NULL))
				g_print("%s convert to utf8 error!\n", p1);
			gchar *pa;
			//process "generalize, generalise"
			if (((pa = strchr(p1, ','))!=NULL)&&(!strstr(p1,"(also"))) {								
				//g_print("Processing %s\n", p1);
				*pa='\0';
				g_strstrip(p1);
				pa++;
				gchar *pb;
				while (pa) {
					if ((pb = strchr(pa, ','))!= NULL) {
						*pb = '\0';						
					}
					worditem.word = pa;
					g_strstrip(worditem.word);
					worditem.data = g_strdup_printf("=> %s", p1);
					worditem.datasize = strlen(worditem.data);
					g_array_append_val(array, worditem);
					if (pb)
						pa = pb+1;
					else
						pa = NULL;
				}
			}
			worditem.word = p1;			
			g_strstrip(worditem.word);
		}
		else {						
			g_strstrip(p1);
			utf_str = g_locale_to_utf8(p1, -1,NULL,&write_size,NULL);
			if (!utf_str) {
				g_print("%s 's meaning convert to utf8 error!\n",worditem.word);
				printf("%s\n",p1);
				write_size=1;
				utf_str = g_strdup("");
			}
			//worditem.data = to_utf8_phonetic(utf_str, worditem.word);
			//g_free(utf_str);
			worditem.data = utf_str;
			worditem.datasize = strlen(worditem.data);
			g_array_append_val(array, worditem);						
		}
		line_num++;
		p1= p2+1;				
	}		
	g_array_sort(array,comparefunc);
	
	idxfile = fopen("oxford.idxdata","w");
	dicfile = fopen("oxford.dict","w");

	fwrite(&(tmpglong),sizeof(glong),1,idxfile);
	

	wordcount = array->len;
	
	gulong i=0;
	pworditem = &g_array_index(array, struct _worditem, i);
	while (i<array->len)
	{
		thedatasize = 0; 
		offset_old = ftell(dicfile);
		wordlength = strlen(pworditem->word);
		flag = 1;
		while (flag == 1)
		{	
			fwrite(pworditem->data,sizeof(gchar),pworditem->datasize,dicfile);
			g_free(pworditem->data);
			thedatasize += pworditem->datasize;
			previous_word = pworditem->word;
						
			i++;
			if (i<array->len)
			{
				pworditem = &g_array_index(array, struct _worditem, i);
				if (strcmp(previous_word,pworditem->word)==0)
				{
					flag = 1;
					wordcount--;
					fwrite(insert_word,sizeof(gchar),strlen(insert_word),dicfile);
					thedatasize += strlen(insert_word);
				}
				else 
				{
					flag = 0;
				}
			}
			else
				flag = 0;
		}
		fwrite(previous_word,sizeof(gchar),wordlength+1,idxfile);
		tmpglong = g_htonl(offset_old);
		fwrite(&(tmpglong),sizeof(glong),1,idxfile);
		tmpglong = g_htonl(thedatasize);
		fwrite(&(tmpglong),sizeof(glong),1,idxfile);	
	}

	fseek(idxfile,0,SEEK_SET);
	tmpglong = g_htonl(wordcount);
	fwrite(&(tmpglong),sizeof(glong),1,idxfile);
	
	g_print("wordcount: %ld\n",wordcount);

	g_free(buffer);
    g_array_free(array,TRUE);
	
	fclose(idxfile);
	fclose(dicfile);	
}

int
main(int argc,char * argv [])
{
	if (argc!=2) {
		printf("please type this:\n./oxford2dict oxford.txt\n");
		return FALSE;
	}

	setlocale(LC_ALL, "");
	convert (argv[1]);
	return FALSE;
	
}
