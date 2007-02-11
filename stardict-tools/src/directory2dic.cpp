//notice, after the dir.idxdata is generated,then do:
//cat dir.idxhead dir.idxdata > dir.idx

#include "stdio.h"
#include "stdlib.h"
#include <string.h>
#include <sys/stat.h>

#include <gtk/gtk.h>

struct _worditem
{
	gchar *word;
	gchar *filename;
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

void add_dir(GArray *array, char *dirname, char *extendname)
{
	struct _worditem worditem;
	GDir* dir = g_dir_open(dirname, 0, NULL);
	const gchar *filename;
	gchar fullfilename[256];
	while ((filename = g_dir_read_name(dir))!=NULL) {
		if (filename[0]=='.') //hiden file.
			continue;
		sprintf(fullfilename, "%s/%s", dirname, filename);
		if (g_file_test(fullfilename, G_FILE_TEST_IS_DIR)) {
			add_dir(array, fullfilename, extendname);
		}
		else {
			if (g_str_has_suffix(filename, extendname)) {
				worditem.word = g_strndup(filename,strlen(filename)-4);
				worditem.filename = g_strdup(fullfilename);
				g_array_append_val(array, worditem);
			}
		}
	}
	g_dir_close(dir);
}

void convert(char *dirname, char *extendname)
{	
	struct stat stats;
	GArray *array= NULL;
	
	if (stat (dirname, &stats) == -1)
	{
		printf("directory not exist!\n");
		return;
	}	
	
	array = g_array_sized_new(FALSE,FALSE, sizeof(struct _worditem),21236); // WyabdcRealPeopleTTS have about 21236 wav files.
	
	add_dir(array, dirname,extendname);
	
	g_print("sorting\n");
	g_array_sort(array,comparefunc);
	g_print("writing\n");
	
	FILE *idxfile,*dicfile;
	idxfile = fopen("dir.idxdata","w");
	dicfile = fopen("dir.dict","w");

	glong wordcount = array->len;
	
	glong tmpglong;
	tmpglong = g_htonl(wordcount);
	fwrite(&(tmpglong),sizeof(glong),1,idxfile);
	
	glong last_filesize=0, offset_old;
	gint wordlength;
	gchar *buffer = NULL;
	FILE *wavfile;
	struct _worditem *pworditem;
	gulong i=0;
	while (i<array->len)
	{
		pworditem = &g_array_index(array, struct _worditem, i);
		
		offset_old = ftell(dicfile);
		wordlength = strlen(pworditem->word);
		fwrite(pworditem->word,sizeof(gchar),wordlength+1,idxfile);
		g_free(pworditem->word);
		tmpglong = g_htonl(offset_old);
		fwrite(&(tmpglong),sizeof(glong),1,idxfile);

		stat (pworditem->filename, &stats);
		if (last_filesize < stats.st_size)
			buffer = (gchar *)g_realloc(buffer, stats.st_size);
		last_filesize = stats.st_size;
		tmpglong = g_htonl(last_filesize);
		fwrite(&(tmpglong),sizeof(glong),1,idxfile);
		
		wavfile = fopen(pworditem->filename,"rb");
		fread (buffer, 1, last_filesize, wavfile);
		fclose(wavfile);
		fwrite(buffer, 1, last_filesize, dicfile);
		g_free(pworditem->filename);
		
		i++;
	}
	g_free(buffer);
	
	g_print("wordcount: %ld\n",wordcount);

    g_array_free(array,TRUE);
	
	fclose(idxfile);
	fclose(dicfile);	
}

int
main(int argc,char * argv [])
{
	if (argc!=3) {
		printf("please type this:\n./directory2dic ZiMaJie .wav\n");
		return FALSE;
	}

	gtk_set_locale ();
	g_type_init ();
	convert (argv[1], argv[2]);
	return FALSE;
	
}
