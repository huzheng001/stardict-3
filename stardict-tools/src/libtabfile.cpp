#include <string.h>
#include <stdlib.h>
#include <glib/gstdio.h>
#include <glib.h>

#include "libtabfile.h"

struct _worditem
{
	gchar *word;
	gchar *definition;
};

static gint comparefunc(gconstpointer a,gconstpointer b)
{
	gint x;
	x = stardict_strcmp(((struct _worditem *)a)->word,((struct _worditem *)b)->word);
	if (x == 0)
                return ((struct _worditem *)a)->definition - ((struct _worditem *)b)->definition;
        else
                return x;
}

static void my_strstrip(char *str, glong linenum, print_info_t print_info)
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
				gchar *str = g_strdup_printf("Warining: line %ld \\r\n", linenum);
				print_info(str);
				g_free(str);
				p1++;
				continue;
			}
			else if (*p1 == '\\') {
				gchar *str = g_strdup_printf("Warining: line %ld \\\\\n", linenum);
				print_info(str);
				g_free(str);
                                *p2='\\';
                                p2++;
                                p1++;
                                continue;
                        }
			else if (*p1 == '\0') {
				gchar *str = g_strdup_printf("Big warining: line %ld end by \\\n", linenum);
				print_info(str);
				g_free(str);
                                *p2='\\';
                                p2++;
                                continue;
			}
			else {
				gchar *str = g_strdup_printf("Warining: line %ld \\%c\n", linenum, *p1);
				print_info(str);
				g_free(str);
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

void convert_tabfile(const char *filename, print_info_t print_info)
{			
	struct stat stats;
	if (g_stat (filename, &stats) == -1)
	{
		print_info("File not exist!\n");
		return;
	}
	gchar *basefilename = g_path_get_basename(filename);
	gchar *ch = strrchr(basefilename, '.');
	if (ch)
		*ch = '\0';
	gchar *dirname = g_path_get_dirname(filename);
	FILE *tabfile;
	tabfile = g_fopen(filename,"r");

	gchar *buffer = (gchar *)g_malloc (stats.st_size + 1);
	size_t readsize = fread (buffer, 1, stats.st_size, tabfile);
	fclose (tabfile);
	buffer[readsize] = '\0';	
	
	GArray *array = g_array_sized_new(FALSE,FALSE, sizeof(struct _worditem),20000);
		
	gchar *p, *p1, *p2;
	p = buffer;
	if ((guchar)*p==0xEF && (guchar)*(p+1)==0xBB && (guchar)*(p+2)==0xBF) // UTF-8 order characters.
		p+=3;
	struct _worditem worditem;
	glong linenum=1;
	while (1) {
		if (*p == '\0') {
                        print_info("Convert over.\n");
                        break;
                }
		p1 = strchr(p,'\n');
		if (!p1) {
			print_info("Error, no new line at the end\n");
			return;
		}
		*p1 = '\0';
		p1++;
		p2 = strchr(p,'\t');
		if (!p2) {
			gchar *str = g_strdup_printf("Warning, no tab, %ld\n", linenum);
			print_info(str);
			g_free(str);
			p= p1;
			linenum++;
			continue;
		}
		*p2 = '\0';
		p2++;
		worditem.word = p;
		worditem.definition = p2;
		my_strstrip(worditem.definition, linenum, print_info);
		g_strstrip(worditem.word);
		g_strstrip(worditem.definition);
		if (!worditem.word[0]) {
			gchar *str = g_strdup_printf("Warning: line %ld, bad word!\n", linenum);
			print_info(str);
			g_free(str);
			p= p1;
                	linenum++;
			continue;
		}
		if (!worditem.definition[0]) {
			gchar *str = g_strdup_printf("Warning: line %ld, bad definition!\n", linenum);
			print_info(str);
			g_free(str);
			p= p1;
                        linenum++;
                        continue;
		}
		g_array_append_val(array, worditem);			
		p= p1;				
		linenum++;
	}		
	g_array_sort(array,comparefunc);
		
	gchar ifofilename[256];
	gchar idxfilename[256];
	gchar dicfilename[256];
	sprintf(ifofilename, "%s" G_DIR_SEPARATOR_S "%s.ifo", dirname, basefilename);
	sprintf(idxfilename, "%s" G_DIR_SEPARATOR_S "%s.idx", dirname, basefilename);
	sprintf(dicfilename, "%s" G_DIR_SEPARATOR_S "%s.dict", dirname, basefilename);
	FILE *ifofile = g_fopen(ifofilename,"wb");
	if (!ifofile) {
		print_info("Write to ifo file failed!\n");
		return;
	}
	FILE *idxfile = g_fopen(idxfilename,"wb");
	if (!idxfile) {
		print_info("Write to idx file failed!\n");
		return;
	}
	FILE *dicfile = g_fopen(dicfilename,"wb");
	if (!dicfile) {
		print_info("Write to dict file failed!\n");
		return;
	}

	guint32 offset_old;
	guint32 tmpglong;
	struct _worditem *pworditem;
	gint definition_len;
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
	fclose(idxfile);
	fclose(dicfile);

	gchar *str = g_strdup_printf("%s wordcount: %d\n", basefilename, array->len);
	print_info(str);
	g_free(str);

#ifndef _WIN32
	gchar command[256];
        sprintf(command, "dictzip %s", dicfilename);
        system(command);
#endif

	g_stat(idxfilename, &stats);
	fprintf(ifofile, "StarDict's dict ifo file\nversion=2.4.2\nwordcount=%d\nidxfilesize=%ld\nbookname=%s\nsametypesequence=m\n", array->len, stats.st_size, basefilename);
	fclose(ifofile);

	g_free(buffer);
	g_array_free(array,TRUE);

	g_free(basefilename);
	g_free(dirname);
}

