#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <cstring>
#include <cstdlib>
#include <glib/gstdio.h>
#include <glib.h>

#include <string>
#include <list>

#include "libbabylonfile.h"
#include "resourcewrap.hpp"
#include "libcommon.h"

struct _worditem
{
	gchar *word;
	gchar *definition;
};

struct _synworditem
{
	gchar *synword;
	gchar *origword;
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

static gint comparefunc2(gconstpointer a,gconstpointer b)
{
	gint x;
        x = stardict_strcmp(((struct _synworditem *)a)->synword,((struct _synworditem *)b)->synword);
	if (x == 0)
		return ((struct _worditem *)a)->definition - ((struct _worditem *)b)->definition;
	else
		return x;
}

static void html_strstrip(char *str, gint linenum, print_info_t print_info)
{
	char *p1, *p2, *p3;
	p1=str;
	p2=str;
	while (*p1 != '\0') {
		if (*p1 == '<') {
			p1++;
			if ((*p1 == 'b' || *p1 == 'B') && (*(p1+1)=='r' || *(p1+1)=='R') && *(p1+2)=='>') {
				*p2='\n';
				p2++;
				p1+=3;
				continue;
			} else {
				p3 = strchr(p1, '>');
				if (!p3) {
					gchar *infostr = g_strdup_printf("Warning: no < %d\n", linenum);
					print_info(infostr);
					g_free(infostr);
					*p2='<';
					p2++;
					continue;
				}
				*p3='\0';
				gchar *infostr = g_strdup_printf("Warning %d : unknow tag: %s\n", linenum, p1);
				print_info(infostr);
				g_free(infostr);
				*p2='<';
				p2++;
				for (int i=0; i< p3-p1; i++) {
					*p2=*(p1+i);
					p2++;
				}
				*p2='>';
				p2++;
				p1=p3+1;
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

static void newline_strstrip(gchar *str, gint linenum, print_info_t print_info)
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
			} else if (*p1 == '\\') {
				*p2='\\';
				p2++;
				p1++;
				continue;
			} else {
				gchar *infostr = g_strdup_printf("Strip warning %d: %s\n", linenum, p1);
				print_info(infostr);
				g_free(infostr);
				*p2='\\';
				p2++;
				*p2=*p1;
				p2++;
				p1++;
				continue;
			}
		} else {
			*p2 = *p1;
			p2++;
			p1++;
			continue;
		}
	}
	*p2 = '\0';
}

/* truncate article keys and synonyms
 * must be: strlen(key) < MAX_INDEX_KEY_SIZE */
static void truncate_key(gchar *str)
{
	size_t new_size = truncate_utf8_string(str, strlen(str), MAX_INDEX_KEY_SIZE-1);
	str[new_size] = '\0';
}

struct DictFields
{
	DictFields(void)
	:
		sametypesequence("m")
	{

	}
	std::string sametypesequence;
	std::string bookname;
	std::string author;
	std::string email;
	std::string website;
	std::string description;
	std::string date;
};

int read_header_fields(gchar*& p, gint& linenum, DictFields& fields, bool& print_sameword,
		print_info_t print_info, int& stripmethod)
{
	gchar *p1;
	while (true) {
		if (*p == '\n') { // empty line
			p++;
			linenum++;
			break;
		}
		if(*p != '#') {
			print_info("Reading header. The first char must be '#'. Line: %d\n", linenum);
			return EXIT_FAILURE;
		}
		++p; // the first char must be '#', skip it
		p1 = strchr(p, '\n');
		if (!p1) {
			print_info("Error, no new line char. line: %d\n", linenum);
			return EXIT_FAILURE;
		}
		*p1 = '\0';
		p1++;
		linenum++;
		if (g_str_has_prefix(p, "stripmethod=")) {
			p += sizeof("stripmethod=") -1;
			if (strcmp(p, "striphtml")==0)
				stripmethod = 0;
			else if (strcmp(p, "stripnewline")==0)
				stripmethod = 1;
			else if (strcmp(p, "keep")==0)
				stripmethod = 2;
		} else if (g_str_has_prefix(p, "sametypesequence=")) {
			p += sizeof("sametypesequence=") -1;
			fields.sametypesequence = p;
		} else if (g_str_has_prefix(p, "bookname=")) {
			p += sizeof("bookname=") -1;
			fields.bookname = p;
		} else if (g_str_has_prefix(p, "author=")) {
			p += sizeof("author=") -1;
			fields.author = p;
		} else if (g_str_has_prefix(p, "email=")) {
			p += sizeof("email=") -1;
			fields.email = p;
		} else if (g_str_has_prefix(p, "website=")) {
			p += sizeof("website=") -1;
			fields.website = p;
		} else if (g_str_has_prefix(p, "date=")) {
			p += sizeof("date=") -1;
			fields.date = p;
		} else if (g_str_has_prefix(p, "description=")) {
			p += sizeof("description=") -1;
			fields.description = p;
		}
		p = p1;
	}
	return EXIT_SUCCESS;
}

int read_articles(gchar *p, print_info_t print_info, gint& linenum,
		GArray *array, GArray *array2, bool print_sameword, int stripmethod)
{
	gchar *p1, *p2, *p3, *p4, *p5;
	struct _worditem worditem;
	struct _synworditem synworditem;

	while (true) {
		/* p  - word and alternates
		 * p1 - definition
		 * p2 - empty line
		 * p3 - next word if any
		 * */
		if (*p == '\0') {
			print_info("Over\n");
			break;
		}
		p1 = strchr(p,'\n');
		if (!p1) {
			print_info("Error, no new line char after word line: %d\n", linenum);
			return EXIT_FAILURE;
		}
		*p1 = '\0';
		p1++;
		linenum++;
		p2 = strchr(p1,'\n');
		if (!p2) {
			print_info("Error, no new line char after definition line: %d\n", linenum);
			return EXIT_FAILURE;
		}
		*p2 = '\0';
		p2++;
		linenum++;
		p3=p2;
		if (*p3 != '\n') {
			print_info("Error, expected an empty line after definition: %d\n", linenum);
			return EXIT_FAILURE;
		}
		*p3='\0';
		p3++;
		linenum++;

		if (stripmethod == 0) {
			html_strstrip(p1, linenum-2, print_info);
		} else if (stripmethod == 1) {
			newline_strstrip(p1, linenum-2, print_info);
		} else if (stripmethod == 2) {
		}
		g_strstrip(p1);
		if (!(*p1)) {
			print_info("Line %d, bad definition!!!\n", linenum-1);
			p= p3;
			continue;
		}

		p4 = strchr(p, '|');
		if (p4) {
			*p4 = '\0';
			worditem.word = p;
			g_strstrip(worditem.word);
			truncate_key(worditem.word);
			if (!worditem.word[0]) {
				print_info("Line %d, bad word!!!\n", linenum-2);
				p=p3;
				continue;
			}
			worditem.definition = p1;
			g_array_append_val(array, worditem);
			std::list <std::string> WordList;
			WordList.push_back(worditem.word);
			p4++;
			while (true) {
				p5 = strchr(p4, '|');
				if (p5) {
					*p5 = '\0';
					synworditem.synword = p4;
					g_strstrip(synworditem.synword);
					truncate_key(synworditem.synword);
					if (!synworditem.synword[0]) {
						print_info("Line %d, bad word!!!\n", linenum-2);
						p4 = p5+1;
						continue;
					}
					bool find = false;
					for (std::list<std::string>::const_iterator it=WordList.begin(); it!=WordList.end(); ++it) {
						if (*it == synworditem.synword) {
							find= true;
							break;
						}
					}
					if (find) {
						if (print_sameword) {
							print_info("Same word: %s\n", synworditem.synword);
						}
						p4 = p5+1;
						continue;
					} else {
						WordList.push_back(synworditem.synword);
					}
					synworditem.origword = worditem.word;
					synworditem.definition = worditem.definition;
					g_array_append_val(array2, synworditem);
					p4 = p5+1;
				} else {
					synworditem.synword = p4;
					g_strstrip(synworditem.synword);
					truncate_key(synworditem.synword);
					if (!synworditem.synword[0]) {
						print_info("Line %d, bad word!!!\n", linenum-2);
						break;
					}
					bool find = false;
					for (std::list<std::string>::const_iterator it=WordList.begin(); it!=WordList.end(); ++it) {
						if (*it == synworditem.synword) {
							find= true;
							break;
						}
					}
					if (find) {
						if (print_sameword) {
							print_info("Same word: %s\n", synworditem.synword);
						}
						break;
					}
					synworditem.origword = worditem.word;
					synworditem.definition = worditem.definition;
					g_array_append_val(array2, synworditem);
					break;
				}
			}
		} else {
			worditem.word = p;
			g_strstrip(worditem.word);
			truncate_key(worditem.word);
			if (!worditem.word[0]) {
				print_info("Line %d, bad word!!!\n", linenum-2);
				p=p3;
				continue;
			}
			worditem.definition = p1;
			g_array_append_val(array, worditem);
		}
		p= p3;
	}
	return EXIT_SUCCESS;
}

int write_dict_and_index(const std::string& idxfilename, const std::string& dicfilename,
		GArray *array, print_info_t print_info)
{
	FILE *idxfile = g_fopen(idxfilename.c_str(),"wb");
	FILE *dicfile = g_fopen(dicfilename.c_str(),"wb");

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
	print_info("wordcount: %d\n", array->len);
	return EXIT_SUCCESS;
}

int write_synonyms(GArray *array, GArray *array2, const std::string& fullbasefilename,
		print_info_t print_info)
{
	if (array2->len == 0)
		return EXIT_SUCCESS;
	const std::string synfilename = fullbasefilename + ".syn";
	FILE *synfile = g_fopen(synfilename.c_str(),"wb");
	struct _synworditem *psynworditem;
	gint iFrom, iTo, iThisIndex, cmpint;
	bool bFound;
	gulong i;
	struct _worditem *pworditem;
	guint32 tmpglong;
	for (i=0; i< array2->len; i++) {
		psynworditem = &g_array_index(array2, struct _synworditem, i);
		fwrite(psynworditem->synword, 1, strlen(psynworditem->synword)+1, synfile);
		bFound=false;
		iFrom=0;
		iTo=array->len-1;
		while (iFrom<=iTo) {
			iThisIndex=(iFrom+iTo)/2;
			pworditem = &g_array_index(array, struct _worditem, iThisIndex);
			cmpint = stardict_strcmp(psynworditem->origword, pworditem->word);
			if (cmpint>0)
				iFrom=iThisIndex+1;
			else if (cmpint<0)
				iTo=iThisIndex-1;
			else {
				bFound=true;
				break;
			}

		}
		if (!bFound) {
			print_info("Error, %s not find.\n", psynworditem->origword);
			return EXIT_FAILURE;
		}
		do {
			if (iThisIndex==0)
				break;
			pworditem = &g_array_index(array, struct _worditem, iThisIndex-1);
			if (strcmp(psynworditem->origword, pworditem->word)==0)
				iThisIndex--;
			else
				break;
		} while (true);
		bFound=false;
		do {
			pworditem = &g_array_index(array, struct _worditem, iThisIndex);
			if (strcmp(psynworditem->origword, pworditem->word)==0) {
				if (psynworditem->definition == pworditem->definition) {
					bFound=true;
					break;
				} else
					iThisIndex++;
			} else
				break;
		} while (true);
		if (!bFound) {
			print_info("Error, %s definition not find.\n", psynworditem->origword);
			return EXIT_FAILURE;
		}
		tmpglong = g_htonl(iThisIndex);
		fwrite(&(tmpglong),sizeof(guint32),1, synfile);
	}
	fclose(synfile);
	print_info("synwordcount: %d\n", array2->len);
	return EXIT_SUCCESS;
}

int write_ifo(GArray *array, GArray *array2, const std::string& ifofilename,
		const std::string& idxfilename, const gchar *basefilename,
		const DictFields& fields)
{
	FILE *ifofile = g_fopen(ifofilename.c_str(),"wb");
	gchar *synwordcount;
	if (array2->len) {
		synwordcount = g_strdup_printf("synwordcount=%d\n", array2->len);
	} else {
		synwordcount = g_strdup("");
	}
	stardict_stat_t stats;
	g_stat(idxfilename.c_str(), &stats);
	fprintf(ifofile, "StarDict's dict ifo file\nversion=2.4.2\nwordcount=%d\n"
		"%sidxfilesize=%ld\n", array->len, synwordcount, (long) stats.st_size);
	if (fields.bookname.empty())
		fprintf(ifofile, "bookname=%s\n", basefilename);
	else
		fprintf(ifofile, "bookname=%s\n", fields.bookname.c_str());
	if (!fields.author.empty())
		fprintf(ifofile, "author=%s\n", fields.author.c_str());
	if (!fields.email.empty())
		fprintf(ifofile, "email=%s\n", fields.email.c_str());
	if (!fields.website.empty())
		fprintf(ifofile, "website=%s\n", fields.website.c_str());
	if (!fields.date.empty())
		fprintf(ifofile, "date=%s\n", fields.date.c_str());
	if (!fields.description.empty())
		fprintf(ifofile, "description=%s\n", fields.description.c_str());
	fprintf(ifofile, "sametypesequence=%s\n", fields.sametypesequence.c_str());
	fclose(ifofile);
	g_free(synwordcount);
	return EXIT_SUCCESS;
}

void convert_babylonfile(const char *filename, print_info_t print_info, bool strip_html)
{
	stardict_stat_t stats;
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
	gchar *buffer = (gchar *)g_malloc (stats.st_size + 1);

	{
		FILE *tabfile = g_fopen(filename,"r");
		size_t readsize = fread (buffer, 1, stats.st_size, tabfile);
		fclose (tabfile);
		buffer[readsize] = '\0';
	}
	
		
	gchar *p;
	p = buffer;
	if ((guchar)*p==0xEF && (guchar)*(p+1)==0xBB && (guchar)*(p+2)==0xBF) // UTF-8 order characters.
		p+=3;
	gint linenum=1;
	int stripmethod;
	if (strip_html)
		stripmethod = 0;
	else
		stripmethod = 1;
	DictFields fields;
	bool print_sameword;
	if (*p == '\n') { // first line is empty
		print_sameword = false;
		p++;
		linenum++;

		if(read_header_fields(p, linenum, fields, print_sameword, print_info, stripmethod))
			return;
	} else {
		print_sameword = true;
	}

	GArray *array = g_array_sized_new(FALSE,FALSE, sizeof(struct _worditem),20000);
	GArray *array2 = g_array_sized_new(FALSE,FALSE, sizeof(struct _synworditem),20000);

	if(read_articles(p, print_info, linenum, array, array2, print_sameword, stripmethod))
		return;

	g_array_sort(array,comparefunc);
	g_array_sort(array2,comparefunc2);

	const std::string fullbasefilename = std::string(dirname) + G_DIR_SEPARATOR_S + basefilename;
	const std::string ifofilename = fullbasefilename + ".ifo";
	const std::string idxfilename = fullbasefilename + ".idx";
	const std::string dicfilename = fullbasefilename + ".dict";

	if(write_dict_and_index(idxfilename, dicfilename, array, print_info))
		return;

	if(write_synonyms(array, array2, fullbasefilename, print_info))
		return;

	if(write_ifo(array, array2, ifofilename, idxfilename, basefilename, fields))
		return;
	
	g_free(buffer);
	g_array_free(array,TRUE);
	g_array_free(array2,TRUE);

#ifndef _WIN32
	std::string command(std::string("dictzip ") + dicfilename);
	system(command.c_str());
#endif

	g_free(basefilename);
	g_free(dirname);
}
