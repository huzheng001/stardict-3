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

#include <cstring>
#include <cstdlib>
#include <string>
#include <sstream>
#include <glib/gstdio.h>
#include <glib.h>

#include "libtabfile.h"
#include "libcommon.h"

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

static void my_strstrip(char *str, glong linenum)
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
				g_warning("Warning: line %ld: end by \\.", linenum);
				*p2='\\';
				p2++;
				continue;
			}
			else {
				g_warning("Warning: line %ld: \\%c is unsupported escape sequence.", linenum, *p1);
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
Dictionary index is allowed to contain duplicates in fact. */
/*
static bool check_duplicate_words(GArray *array)
{
	if(array->len < 2)
		return true;

	gulong i;
	struct _worditem *pworditem1, *pworditem2;
	pworditem1 = &g_array_index(array, struct _worditem, 0);
	bool return_var = true;
	for (i=1; i < array->len; i++) {
		pworditem2 = &g_array_index(array, struct _worditem, i);
		if ((stardict_strcmp(pworditem1->word, pworditem2->word) == 0) && (strcmp(pworditem1->definition, pworditem2->definition) ==0)) {
			g_warning("Error, duplicate word: %s", pworditem2->word);
			return_var = false;
		}
		pworditem1 = pworditem2;
	}
	return return_var;
}
*/

static bool read_tab_file(gchar *buffer, GArray *array)
{
	gchar *p, *p1, *p2;
	p = buffer;
	if(g_str_has_prefix(p, "\xEF\xBB\xBF")) // UTF-8 BOM
		p+=3;
	if(!g_utf8_validate(p, -1, NULL)) {
		g_critical("Error, invalid UTF-8 encoded text.");
		return false;
	}
	struct _worditem worditem;
	glong linenum=1;
	while (true) {
		if (*p == '\0') {
			g_message("Convertion is over.");
			break;
		}
		p1 = strchr(p,'\n');
		if (!p1) {
			g_critical("Error, no new line at the end.");
			return false;
		}
		*p1 = '\0';
		p1++;
		p2 = strchr(p,'\t');
		if (!p2) {
			g_warning("Warning: line %ld, no tab! Skipping line.", linenum);
			p = p1;
			linenum++;
			continue;
		}
		*p2 = '\0';
		p2++;
		worditem.word = p;
		worditem.definition = p2;
		my_strstrip(worditem.word, linenum);
		my_strstrip(worditem.definition, linenum);
		g_strstrip(worditem.word);
		g_strstrip(worditem.definition);
		if (!worditem.word[0]) {
			g_warning("Warning: line %ld, bad word! Skipping line.", linenum);
			p = p1;
			linenum++;
			continue;
		}
		if (!worditem.definition[0]) {
			g_warning("Warning: line %ld, bad definition! Skipping line.", linenum);
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

static bool write_dictionary(const char *filename, GArray *array)
{
	glib::CharStr basefilename(g_path_get_basename(filename));
	gchar *ch = strrchr(get_impl(basefilename), '.');
	if (ch)
		*ch = '\0';
	glib::CharStr dirname(g_path_get_dirname(filename));

	const std::string fullbasefilename = build_path(get_impl(dirname), get_impl(basefilename));
	const std::string ifofilename = fullbasefilename + ".ifo";
	const std::string idxfilename = fullbasefilename + ".idx";
	const std::string dicfilename = fullbasefilename + ".dict";
	clib::File ifofile(g_fopen(ifofilename.c_str(),"wb"));
	if (!ifofile) {
		g_critical("Write to ifo file %s failed!", ifofilename.c_str());
		return false;
	}
	clib::File idxfile(g_fopen(idxfilename.c_str(),"wb"));
	if (!idxfile) {
		g_critical("Write to idx file %s failed!", idxfilename.c_str());
		return false;
	}
	clib::File dicfile(g_fopen(dicfilename.c_str(),"wb"));
	if (!dicfile) {
		g_critical("Write to dict file %s failed!", dicfilename.c_str());
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

	g_message("%s wordcount: %d.", get_impl(basefilename), array->len);

#ifndef _WIN32
	std::stringstream command;
	command << "dictzip \"" << dicfilename << "\"";
	int result;
	result = system(command.str().c_str());
	if (result == -1) {
		g_print("system() error!\n");
	}
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

bool convert_tabfile(const char *filename)
{
	glib::CharStr buffer;
	glib::Error err;
	if(!g_file_get_contents(filename, get_addr(buffer), NULL, get_addr(err))) {
		g_critical("Unable to open file %s, error: %s.", filename, err->message);
		return false;
	}

	GArray *array = g_array_sized_new(FALSE,FALSE, sizeof(struct _worditem),20000);
	ArrayWrapper array_wrapper(array);
	
	if(!read_tab_file(get_impl(buffer), array))
		return false;

	if(array->len < 1) {
		g_critical("Error: empty dictionary.");
		return false;
	}
	g_array_sort(array, comparefunc);
	//if(!check_duplicate_words(array))
	//	return false;
	
	if(!write_dictionary(filename, array))
		return false;
	return true;
}
