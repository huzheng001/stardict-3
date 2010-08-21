#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <cstring>
#include <cstdlib>
#include <string>
#include <glib/gstdio.h>
#include <glib.h>

#include "libtabfile.h"
#include "libcommon.h"
#include "resourcewrap.hpp"

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
			else if (*p1 == '\\') {
				*p2='\\';
				p2++;
				p1++;
				continue;
			}
			else if (*p1 == 't') {
				*p2='\t';
				p2++;
				p1++;
				continue;
			}
			else if (*p1 == '\0') {
				print_info("Warning: line %ld: end by \\\n", linenum);
				*p2='\\';
				p2++;
				continue;
			}
			else {
				print_info("Warning: line %ld: \\%c is unsupported escape sequence.\n", linenum, *p1);
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

/* return: true - OK, false - error 
Dictionary index is not allowed to contain duplicates. */
static bool check_duplicate_words(GArray *array, print_info_t print_info)
{
	if(array->len < 2)
		return true;

	gulong i;
	struct _worditem *pworditem1, *pworditem2;
	pworditem1 = &g_array_index(array, struct _worditem, 0);
	for (i=1; i < array->len; i++) {
		pworditem2 = &g_array_index(array, struct _worditem, i);
		if(stardict_strcmp(pworditem1->word, pworditem2->word) == 0) {
			print_info("Error, duplicate word %s.\n", pworditem1->word);
			return false;
		}
		pworditem1 = pworditem2;
	}
	return true;
}

static bool read_tab_file(gchar *buffer, GArray *array, print_info_t print_info)
{
	gchar *p, *p1, *p2;
	p = buffer;
	if(g_str_has_prefix(p, "\xEF\xBB\xBF")) // UTF-8 BOM
		p+=3;
	if(!g_utf8_validate(p, -1, NULL)) {
		print_info("Error, invalid UTF-8 encoded text.\n");
		return false;
	}
	struct _worditem worditem;
	glong linenum=1;
	while (true) {
		if (*p == '\0') {
			print_info("Convertion is over.\n");
			break;
		}
		p1 = strchr(p,'\n');
		if (!p1) {
			print_info("Error, no new line at the end.\n");
			return false;
		}
		*p1 = '\0';
		p1++;
		p2 = strchr(p,'\t');
		if (!p2) {
			print_info("Warning: line %ld, no tab! Skipping line.\n", linenum);
			p = p1;
			linenum++;
			continue;
		}
		*p2 = '\0';
		p2++;
		worditem.word = p;
		worditem.definition = p2;
		my_strstrip(worditem.word, linenum, print_info);
		my_strstrip(worditem.definition, linenum, print_info);
		g_strstrip(worditem.word);
		g_strstrip(worditem.definition);
		if (!worditem.word[0]) {
			print_info("Warning: line %ld, bad word! Skipping line.\n", linenum);
			p = p1;
			linenum++;
			continue;
		}
		if (!worditem.definition[0]) {
			print_info("Warning: line %ld, bad definition! Skipping line.\n", linenum);
			p = p1;
			linenum++;
			continue;
		}
		g_array_append_val(array, worditem);
		p = p1;
		linenum++;
	}
	return true;
}

static bool write_dictionary(const char *filename, GArray *array, print_info_t print_info)
{
	glib::CharStr basefilename(g_path_get_basename(filename));
	gchar *ch = strrchr(get_impl(basefilename), '.');
	if (ch)
		*ch = '\0';
	glib::CharStr dirname(g_path_get_dirname(filename));

	const std::string fullbasefilename = std::string(get_impl(dirname)) + G_DIR_SEPARATOR_S + get_impl(basefilename);
	const std::string ifofilename = fullbasefilename + ".ifo";
	const std::string idxfilename = fullbasefilename + ".idx";
	const std::string dicfilename = fullbasefilename + ".dict";
	clib::File ifofile(g_fopen(ifofilename.c_str(),"wb"));
	if (!ifofile) {
		print_info("Write to ifo file %s failed!\n", ifofilename.c_str());
		return false;
	}
	clib::File idxfile(g_fopen(idxfilename.c_str(),"wb"));
	if (!idxfile) {
		print_info("Write to idx file %s failed!\n", idxfilename.c_str());
		return false;
	}
	clib::File dicfile(g_fopen(dicfilename.c_str(),"wb"));
	if (!dicfile) {
		print_info("Write to dict file %s failed!\n", dicfilename.c_str());
		return false;
	}

	guint32 offset_old;
	guint32 tmpglong;
	struct _worditem *pworditem;
	gint definition_len;
	gulong i;
	for (i=0; i < array->len; i++) {
		offset_old = ftell(get_impl(dicfile));
		pworditem = &g_array_index(array, struct _worditem, i);
		definition_len = strlen(pworditem->definition);
		fwrite(pworditem->definition, 1 ,definition_len,get_impl(dicfile));
		fwrite(pworditem->word,sizeof(gchar),strlen(pworditem->word)+1,get_impl(idxfile));
		tmpglong = g_htonl(offset_old);
		fwrite(&(tmpglong),sizeof(guint32),1,get_impl(idxfile));
		tmpglong = g_htonl(definition_len);
		fwrite(&(tmpglong),sizeof(guint32),1,get_impl(idxfile));
	}
	idxfile.reset(NULL);
	dicfile.reset(NULL);

	print_info("%s wordcount: %d\n", get_impl(basefilename), array->len);

#ifndef _WIN32
	std::string command(std::string("dictzip ") + dicfilename);
	system(command.c_str());
#endif

	stardict_stat_t stats;
	g_stat(idxfilename.c_str(), &stats);
	fprintf(get_impl(ifofile), "StarDict's dict ifo file\nversion=2.4.2\nwordcount=%d\n"
		"idxfilesize=%ld\nbookname=%s\nsametypesequence=m\n",
		array->len, (long) stats.st_size, get_impl(basefilename));
	return true;
}

class ArrayWrapper {
public:
	ArrayWrapper(GArray *array)
		: array(array)
	{
	}
	~ArrayWrapper(void)
	{
		g_array_free(array,TRUE);
	}
private:
	GArray *array;
};

bool convert_tabfile(const char *filename, print_info_t print_info)
{
	glib::CharStr buffer;
	glib::Error err;
	if(!g_file_get_contents(filename, get_addr(buffer), NULL, get_addr(err))) {
		print_info("Unable to open file %s, error: %s\n", filename, err->message);
		return false;
	}

	GArray *array = g_array_sized_new(FALSE,FALSE, sizeof(struct _worditem),20000);
	ArrayWrapper array_wrapper(array);
	
	if(!read_tab_file(get_impl(buffer), array, print_info))
		return false;

	if(array->len < 1) {
		print_info("Error: empty dictionary.\n");
		return false;
	}
	g_array_sort(array, comparefunc);
	if(!check_duplicate_words(array, print_info))
		return false;
	
	if(!write_dictionary(filename, array, print_info))
		return false;
	return true;
}
