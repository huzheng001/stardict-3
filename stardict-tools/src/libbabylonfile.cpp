#include <string.h>
#include <stdlib.h>
#include <glib/gstdio.h>
#include <glib.h>

#include <string>
#include <list>

#include "libbabylonfile.h"

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

void convert_babylonfile(const char *filename, print_info_t print_info)
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
	GArray *array2 = g_array_sized_new(FALSE,FALSE, sizeof(struct _synworditem),20000);
		
	gchar *p, *p1, *p2, *p3, *p4, *p5;
	p = buffer;
	if ((guchar)*p==0xEF && (guchar)*(p+1)==0xBB && (guchar)*(p+2)==0xBF) // UTF-8 order characters.
		p+=3;
	struct _worditem worditem;
	struct _synworditem synworditem;
	gint linenum=1;
	while (1) {
		if (*p == '\0') {
                        print_info("Over\n");
                        break;
                }
		p1 = strchr(p,'\n');
		if (!p1) {
			gchar *str = g_strdup_printf("Error, no end line 1: %d\n", linenum);
			print_info(str);
			g_free(str);
			return;
		}
		*p1 = '\0';
		p1++;
		linenum++;
		p2 = strchr(p1,'\n');
		if (!p2) {
			gchar *str = g_strdup_printf("Error, no end line 2: %d\n", linenum);
			print_info(str);
			g_free(str);
			return;
		}
		*p2 = '\0';
		p2++;
		linenum++;
		p3=p2;
		if (*p3 != '\n') {
			gchar *str = g_strdup_printf("Error, not null line %d", linenum);
			print_info(str);
			g_free(str);
			return;
		}
		*p3='\0';
		p3++;
		linenum++;
		
		html_strstrip(p1, linenum-2, print_info);
		g_strstrip(p1);
		if (!(*p1)) {
			gchar *str = g_strdup_printf("%s-%d, bad definition!!!\n", basefilename, linenum-1);
			print_info(str);
			g_free(str);
			p= p3;
                        continue;
		}	
		
		p4 = strchr(p, '|');
		if (p4) {
			*p4 = '\0';
			worditem.word = p;
                        g_strstrip(worditem.word);
                        if (!worditem.word[0]) {
				gchar *str = g_strdup_printf("%s-%d, bad word!!!\n", basefilename, linenum-2);
				print_info(str);
				g_free(str);
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
                        		if (!synworditem.synword[0]) {
						gchar *str = g_strdup_printf("%s-%d, bad word!!!\n", basefilename, linenum-2);
						print_info(str);
						g_free(str);
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
						gchar *str = g_strdup_printf("Same word: %s\n", synworditem.synword);
						print_info(str);
						g_free(str);
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
                                        if (!synworditem.synword[0]) {
						gchar *str = g_strdup_printf("%s-%d, bad word!!!\n", basefilename, linenum-2);
						print_info(str);
						g_free(str);
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
						gchar *str = g_strdup_printf("Same word: %s\n", synworditem.synword);
						print_info(str);
						g_free(str);
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
			if (!worditem.word[0]) {
				gchar *str = g_strdup_printf("%s-%d, bad word!!!\n", basefilename, linenum-2);
				print_info(str);
				g_free(str);
				p=p3;
				continue;
			}
			worditem.definition = p1;
			g_array_append_val(array, worditem);
		}
		p= p3;
	}		
	g_array_sort(array,comparefunc);
	g_array_sort(array2,comparefunc2);

	gchar ifofilename[256];
	gchar idxfilename[256];
	gchar dicfilename[256];
	sprintf(ifofilename, "%s" G_DIR_SEPARATOR_S "%s.ifo", dirname, basefilename);
	sprintf(idxfilename, "%s" G_DIR_SEPARATOR_S "%s.idx", dirname, basefilename);
	sprintf(dicfilename, "%s" G_DIR_SEPARATOR_S "%s.dict", dirname, basefilename);
	FILE *ifofile = g_fopen(ifofilename,"wb");
	FILE *idxfile = g_fopen(idxfilename,"wb");
	FILE *dicfile = g_fopen(dicfilename,"wb");
	
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

	if (array2->len) {
		gchar synfilename[256];
	        sprintf(synfilename, "%s" G_DIR_SEPARATOR_S "%s.syn", dirname, basefilename);
		FILE *synfile = g_fopen(synfilename,"wb");
		struct _synworditem *psynworditem;
		gint iFrom, iTo, iThisIndex, cmpint;
		bool bFound;
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
				gchar *str = g_strdup_printf("Error, %s not find.\n", psynworditem->origword);
				print_info(str);
				g_free(str);
				return;
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
				gchar *str = g_strdup_printf("Error, %s definition not find.\n", psynworditem->origword);
				print_info(str);
				g_free(str);
                                return;
                        }
			tmpglong = g_htonl(iThisIndex);
	                fwrite(&(tmpglong),sizeof(guint32),1, synfile);
		}
		fclose(synfile);
		gchar *str = g_strdup_printf("%s synwordcount: %d\n", basefilename, array2->len);
		print_info(str);
		g_free(str);
	}

	gchar *synwordcount;
	if (array2->len) {
		synwordcount = g_strdup_printf("synwordcount=%d\n", array2->len);
	} else {
		synwordcount = g_strdup("");
	}
	g_stat(idxfilename, &stats);
        fprintf(ifofile, "StarDict's dict ifo file\nversion=2.4.2\nwordcount=%d\n%sidxfilesize=%ld\nbookname=%s\nsametypesequence=m\n", array->len, synwordcount, stats.st_size, basefilename);
        fclose(ifofile);
	g_free(synwordcount);

	
	g_free(buffer);
	g_array_free(array,TRUE);
	g_array_free(array2,TRUE);

#ifndef _WIN32
	gchar command[256];
        sprintf(command, "dictzip %s", dicfilename);
        system(command);
#endif

	g_free(basefilename);
	g_free(dirname);
}
