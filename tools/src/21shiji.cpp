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
#include <string>

#include <glib.h>

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

gchar *to_utf8_phonetic(gchar *text, glong linenum)
{
	gchar *start = strstr(text, "\n[");
	if (!start) {
		//g_print("Warinnig, no phonetic: %ld\n", linenum);
                return NULL;
	}
	*start='\0';
	gchar *end = strstr(start + 2, "]\n");
	if (!end) {
		end = strstr(start + 2, "~]");
		if (!end) {
			end=strchr(start+2, ']');
			if (end)
				*(end+1)='\0';
			if (linenum!=14638) {
                		//printf("Warnning, bad phonetic: %ld %s\n", linenum, start+1);
			}
		}
                return NULL;
        }
        *end = '\0';

	std::string return_text = text;
	return_text+="\n[";
	const gchar *s, *old_s;
	s = start+2;
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
	return_text+= "]\n";
	return_text+= (end+2);
	len = return_text.length();
	for (int i =0; i< len; i++) {
		if (return_text[i]==0x01)
			return_text[i] = ' ';
	}
	return g_strdup(return_text.c_str());
}

void my_strstrip(char *str, glong linenum)
{
	char *p1, *p2;
	p1=str;
	p2=str;
	while (*p1 != '\0') {
		if (*p1 == '\\') {
			p1++;
			if (*p1 == 'n') {
				*p2='\n';
				p2++;
				p1++;
				continue;
			}
			else if (*p1 == 'r') {
				g_print("warining, %ld \\r\n", linenum);
				p1++;
				continue;
			}
			else if (*p1 == '\\') {
				g_print("warining, %ld \\\\\n", linenum);
                                *p2='\\';
                                p2++;
                                p1++;
                                continue;
                        }
			else if (*p1 == '\0') {
                                g_print("big warining, %ld \\\n", linenum);
                                *p2='\\';
                                p2++;
                                continue;
			}
			else {
				g_print("warining, %ld \\%c\n", linenum,  *p1);
                                *p2='\\';
                                p2++;
                                *p2=*p1;
                                p2++;
				p1++;
				continue;
			}
		}
		else {
			*p2 = *p1;
			p2++;
			p1++;
			continue;
		}
	}
	*p2 = '\0';
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
		
	gchar *p, *p1, *p2;
	p = buffer;
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
		p2 = strchr(p,'\t');
		if (!p2) {
			g_print("error, no tab, %ld\n", linenum);
			return;
		}
		*p2 = '\0';
		p2++;
		worditem.word = p;
		my_strstrip(p2, linenum);
		worditem.definition = to_utf8_phonetic(p2, linenum);
		if (worditem.definition==NULL)
			worditem.definition = g_strdup(p2);
		g_strstrip(worditem.word);
		g_strstrip(worditem.definition);
		if (!worditem.word[0]) {
			g_print("%s-%ld, bad word!!!\n", basefilename, linenum);
			p= p1;
                	linenum++;
			continue;
		}
		if (!worditem.definition[0]) {
			g_print("%s-%ld, bad definition!!!\n", basefilename, linenum);
			p= p1;
                        linenum++;
                        continue;
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

	for (i= 0;i < array->len; i++) {
                pworditem = &g_array_index(array, struct _worditem, i);
                g_free(pworditem->definition);
        }
	g_free(buffer);
	g_array_free(array,TRUE);
	
	fclose(idxfile);
	fclose(dicfile);

	g_free(basefilename);
}

int main(int argc,char * argv [])
{
	if (argc<2) {
		printf("please type this:\n./tabfile Chinese-idiom-quick.pdb.tab.utf8\n");
		return FALSE;
	}

	setlocale(LC_ALL, "");
	for (int i=1; i< argc; i++)
		convert (argv[i]);
	return FALSE;	
}

