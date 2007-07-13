#include "stdio.h"
#include "stdlib.h"
#include <string.h>
#include <sys/stat.h>

#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/mman.h>


#include <gtk/gtk.h>


struct _worditem
{
        gchar *title;
        gchar *text;
};

gint stardict_strcmp(const gchar *s1, const gchar *s2)
{
        gint a;         a = g_ascii_strcasecmp(s1, s2);
        if (a == 0)
                return strcmp(s1, s2);
        else
                return a;
}

gint comparefunc(gconstpointer a,gconstpointer b)
{
        gint x;
        x = stardict_strcmp(((struct _worditem *)a)->title,((struct _worditem *)b)->title);
        if (x == 0)
                return ((struct _worditem *)a)->text - ((struct _worditem *)b)->text;
        else
                return x;
}


typedef struct _ParseUserData {
	bool inpage;
	gchar *title;
	gchar *text;
	GArray *array;
} ParseUserData;

static void func_parse_start_element(GMarkupParseContext *context, const gchar *element_name, const gchar **attribute_names, const gchar **attribute_values, gpointer user_data, GError **error)
{
	ParseUserData *Data = (ParseUserData *)user_data;
	if (strcmp(element_name, "page")==0) {
		Data->inpage = true;
		Data->title = NULL;
		Data->text = NULL;
	}
}

static void func_parse_end_element(GMarkupParseContext *context,
                          const gchar         *element_name,
                          gpointer             user_data,
                          GError             **error)
{
	ParseUserData *Data = (ParseUserData *)user_data;
        if (strcmp(element_name, "page")==0) {
                Data->inpage = false;
		struct _worditem worditem;
		if (Data->title && Data->text) {
			worditem.title = Data->title;
			worditem.text = Data->text;
			g_array_append_val(Data->array, worditem);
		} else {
			g_free(Data->title);
			g_free(Data->text);
		}
        }

}

static void func_parse_text(GMarkupParseContext *context, const gchar *text, gsize text_len, gpointer user_data, GError **error)
{
	ParseUserData *Data = (ParseUserData *)user_data;
	if (!Data->inpage)
		return;
	const gchar *element = g_markup_parse_context_get_element(context);
        if (!element) {
		return;
	}
	if (strcmp(element, "title")==0) {
		Data->title = g_strndup(text, text_len);
	} else if (strcmp(element, "text")==0) { 
		Data->text = g_strndup(text, text_len);
	}
}

void convert(char *filename, char *wikiname, char *wikidate)
{
        struct stat stats;
        if (stat (filename, &stats) == -1)
        {
                printf("file not exist!\n");
                return;
        }
	int mmap_fd;
	if ((mmap_fd = open(filename, O_RDONLY | O_LARGEFILE)) < 0) {
		printf("open file failed!\n");
		return;
	}
	char *data;
	GArray *array = g_array_sized_new(FALSE,FALSE, sizeof(struct _worditem),20000);
	ParseUserData Data;
	Data.inpage=false;
	Data.array= array;
	GMarkupParser parser;
	parser.start_element = func_parse_start_element;
	parser.end_element = func_parse_end_element;
	parser.text = func_parse_text;
	parser.passthrough = NULL;
	parser.error = NULL;
	GMarkupParseContext* context = g_markup_parse_context_new(&parser, (GMarkupParseFlags)0, &Data, NULL);
	size_t length;
	for (size_t offset = 0; offset < stats.st_size; offset += 10240000) {
		if (offset + 10240000 > stats.st_size) {
			length = stats.st_size - offset;
		} else {
			length = 10240000;
		}
		data = (char *)mmap( NULL, length, PROT_READ, MAP_SHARED, mmap_fd, offset);
		if (data == MAP_FAILED) {
			printf("mmap failed!\n");
			return;
		}
		if (g_markup_parse_context_parse(context, data, length, NULL) == FALSE) {
			if (!g_utf8_validate(data, length, NULL))
				g_print("invalide UTF-8.\n");
			g_print("Parse error!\n");
			g_file_set_contents("error.xml", data, length, NULL);
			return;
		}
		munmap(data, length);
		g_print(".");
		fflush(stdout);
	}
	g_markup_parse_context_end_parse(context, NULL);
	g_markup_parse_context_free(context);
	close(mmap_fd);

	g_print("Parse over!\n");
	g_array_sort(array,comparefunc);
	gchar idxfilename[256];
        gchar dicfilename[256];
	gchar ifofilename[256];
        sprintf(idxfilename, "wikipedia-%s-%s.idx", wikiname, wikidate);
        sprintf(dicfilename, "wikipedia-%s-%s.dict", wikiname, wikidate);
        sprintf(ifofilename, "wikipedia-%s-%s.ifo", wikiname, wikidate);
        FILE *idxfile = fopen(idxfilename,"w");
        FILE *dicfile = fopen(dicfilename,"w");
	guint32 offset_old;
        guint32 tmpglong;
        struct _worditem *pworditem;
        gint definition_len;
        gulong i;
        for (i=0; i< array->len; i++) {
                offset_old = ftell(dicfile);
                pworditem = &g_array_index(array, struct _worditem, i);
                definition_len = strlen(pworditem->text);
                fwrite(pworditem->text, 1 ,definition_len,dicfile);
                fwrite(pworditem->title,sizeof(gchar),strlen(pworditem->title)+1,idxfile);
                tmpglong = g_htonl(offset_old);
                fwrite(&(tmpglong),sizeof(guint32),1,idxfile);
                tmpglong = g_htonl(definition_len);
                fwrite(&(tmpglong),sizeof(guint32),1,idxfile);
		g_free(pworditem->text);
		g_free(pworditem->title);
        }
	long idxfilesize = ftell(idxfile);
        fclose(idxfile);
        fclose(dicfile);

	FILE *ifofile = fopen(ifofilename,"w");
	gchar *content = g_strdup_printf("StarDict's dict ifo file\nversion=2.4.2\nwordcount=%d\nidxfilesize=%ld\nbookname=%s\ndescription=Made by Hu Zheng. WikiPedia version: %s\ndate=2006.12.18\nsametypesequence=w\n", array->len, idxfilesize, wikiname, wikidate);
	fwrite(content, strlen(content), 1, ifofile);
	g_free(content);
	fclose(ifofile);

        g_print("%s wordcount: %d\n", filename, array->len);
	g_array_free(array,TRUE);

	gchar command[256];
        sprintf(command, "dictzip %s", dicfilename);
        system(command);

	char dirname[256];
	sprintf(dirname, "stardict-wikipedia-%s-%s-2.4.2", wikiname, wikidate);
	sprintf(command, "mkdir %s", dirname);
	system(command);
	sprintf(command, "mv %s %s", idxfilename, dirname);
	system(command);
	sprintf(command, "mv %s.dz %s", dicfilename, dirname);
	system(command);
	sprintf(command, "mv %s %s", ifofilename, dirname);
	system(command);
}

int main(int argc,char * argv [])
{
	if (argc!=4) {
		printf("please type this:\n./wikipedia zhwiki-20060303-pages-articles.xml zhwiki 20060303\n");
                return FALSE;
        }

        gtk_set_locale ();
        g_type_init ();
        convert (argv[1], argv[2], argv[3]);
	return FALSE;
}
