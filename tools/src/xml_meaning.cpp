#include "stdio.h"
#include "stdlib.h"
#include <locale.h>
#include <string.h>
#include <sys/stat.h>

#include <glib.h>

typedef struct _ParseUserData {
	FILE *tabfile;
} ParseUserData;

static void func_parse_text(GMarkupParseContext *context, const gchar *text, gsize text_len, gpointer user_data, GError **error)
{
	const gchar *element = g_markup_parse_context_get_element(context);
	if (strcmp(element, "meaning")==0) {
		ParseUserData *Data = (ParseUserData *)user_data;

		fwrite(text, text_len, 1, Data->tabfile);
		fwrite("\\n", 2, 1, Data->tabfile);
		// tabfile will g_strstrip the last \\n
	}
}	

static void func_parse_start_element(GMarkupParseContext *context, const gchar *element_name, const gchar **attribute_names, const gchar **attribute_values, gpointer user_data, GError **error)
{
	ParseUserData *Data = (ParseUserData *)user_data;

	if (strcmp(element_name, "word")==0) {
		if (strcmp(*attribute_names, "name")==0) {
			gint len = strlen(*attribute_values);
			fwrite(*attribute_values, len, 1, Data->tabfile);
			fwrite("\t", 1, 1, Data->tabfile);
		}
	}
}

static void func_parse_end_element(GMarkupParseContext *context, const gchar *element_name, gpointer user_data, GError **error)
{
	ParseUserData *Data = (ParseUserData *)user_data;

	if (strcmp(element_name, "word")==0) {
		fwrite("\n", 1, 1, Data->tabfile);
	}
}

void parse_content(gchar *p, ParseUserData *Data)
{
	GMarkupParser parser;
    parser.start_element = func_parse_start_element;
    parser.end_element = func_parse_end_element;
    parser.text = func_parse_text;
    parser.passthrough = NULL;
    parser.error = NULL;
    GMarkupParseContext* context = g_markup_parse_context_new(&parser, (GMarkupParseFlags)0, Data, NULL);
    g_markup_parse_context_parse(context, p, strlen(p), NULL);
    g_markup_parse_context_end_parse(context, NULL);
    g_markup_parse_context_free(context);
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
	FILE *xmlfile;
	xmlfile = fopen(filename,"r");

	gchar *buffer = (gchar *)g_malloc (stats.st_size + 1);
	size_t fread_size;
	fread_size = fread (buffer, 1, stats.st_size, xmlfile);
	if (fread_size != (size_t)stats.st_size) {
		g_print("fread error!\n");
	}
	fclose (xmlfile);
	buffer[stats.st_size] = '\0';

	gchar *tabfilename = g_strdup_printf("%s.tab", basefilename);
	FILE *tabfile;
	tabfile = fopen(tabfilename, "w");

	ParseUserData Data;
	Data.tabfile = tabfile;
	parse_content(buffer, &Data);

	fclose(tabfile);
	g_print("Out file: %s.tab\n", basefilename);

	g_free(buffer);
	g_free(basefilename);
	g_free(tabfilename);
}

int main(int argc,char * argv [])
{
	if (argc<2) {
		printf("please type this:\n./xml_meaning english-turkish.xml\n");
		return FALSE;
	}

	setlocale(LC_ALL, "");
	for (int i=1; i< argc; i++)
		convert (argv[i]);
	return FALSE;	
}

