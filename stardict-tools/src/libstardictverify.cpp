#include <string.h>
#include <stdlib.h>
#include <glib/gstdio.h>
#include <glib.h>

#include <string>

#include "libstardictverify.h"

static int verify_synfile(const gchar *synfilename, guint synwordcount, guint idxwordcount, print_info_t print_info)
{
	struct stat stats;
	if (g_stat (synfilename, &stats) == -1) {
		gchar *str = g_strdup_printf("File not exist: %s\n", synfilename);
		print_info(str);
		g_free(str);
		return EXIT_FAILURE;
	}
	gchar *str = g_strdup_printf("Verifing file: %s\n", synfilename);
	print_info(str);
	g_free(str);
	gchar *buffer = (gchar *)g_malloc (stats.st_size);
	gchar *buffer_end = buffer+stats.st_size;
	FILE *synfile;
	synfile = g_fopen(synfilename,"rb");
	fread (buffer, 1, stats.st_size, synfile);
	fclose (synfile);

	gchar *p=buffer;
	gchar *preword=NULL;
	int wordlen;
	gint cmpvalue;
	guint32 index;
	guint wordcount=0;
	bool have_errors=false;

	while (true) {
		if (p == buffer_end) {
			break;
		}
		wordlen=strlen(p);
		if (wordlen==0) {
			print_info("Error: wordlen==0\n");
		} else {
			if (wordlen>=256) {
				gchar *str = g_strdup_printf("Error: wordlen>=256, %s\n", p);
				print_info(str);
				g_free(str);
                        }
                        if (g_ascii_isspace(*p)) {
				gchar *str = g_strdup_printf("Warning: begin with space, %s\n", p);
				print_info(str);
				g_free(str);
                        }
                        if (g_ascii_isspace(*(p+wordlen-1))) {
				gchar *str = g_strdup_printf("Warning: end with space, %s\n", p);
				print_info(str);
				g_free(str);
                        }
                }
                if (strpbrk(p, "\t\n")) {
			gchar *str = g_strdup_printf("Warning: contain invalid character, %s\n", p);
			print_info(str);
			g_free(str);
                }
                if (!g_utf8_validate(p, wordlen, NULL)) {
			gchar *str = g_strdup_printf("Error: invalid utf8 string, %s\n", p);
			print_info(str);
			g_free(str);
			have_errors=true;
                }
                if (preword) {
                        cmpvalue=stardict_strcmp(preword, p);
			if (cmpvalue>0) {
				gchar *str = g_strdup_printf("Error: wrong string order, %s\n", p);
				print_info(str);
				g_free(str);
				have_errors=true;
                        }
                }
                preword=p;
		p+=wordlen +1;
		index=*reinterpret_cast<guint32 *>(p);
                index=g_ntohl(index);
                if (index>=idxwordcount) {
			gchar *str = g_strdup_printf("Error: index is wrong, %s\n", preword);
			print_info(str);
			g_free(str);
			have_errors=true;
                }
		p+=sizeof(guint32);
		wordcount++;
	}

	if (wordcount!=synwordcount) {
		gchar *str = g_strdup_printf("Error: in .ifo file, synwordcount=%d, while the real synwordcount is %d\n", synwordcount, wordcount);
		print_info(str);
		g_free(str);
		have_errors=true;
        }

        g_free(buffer);
	return have_errors ? EXIT_FAILURE : EXIT_SUCCESS;
}

static int verify_idxfile(const gchar *idxfilename, guint ifo_wordcount, guint ifo_index_file_size, print_info_t print_info)
{
	struct stat stats;
        if (g_stat (idxfilename, &stats) == -1) {
		gchar *str = g_strdup_printf("File not exist: %s\n", idxfilename);
		print_info(str);
		g_free(str);
                return EXIT_FAILURE;
        }
	gchar *str = g_strdup_printf("Verifing file: %s\n", idxfilename);
	print_info(str);
	g_free(str);
	if (ifo_index_file_size!=(guint)stats.st_size) {
		gchar *str = g_strdup_printf("Error: in .ifo file, idxfilesize=%d, while the real idx file size is %ld\n", ifo_index_file_size, stats.st_size);
		print_info(str);
		g_free(str);
		return EXIT_FAILURE;
	}

	gchar *buffer = (gchar *)g_malloc (stats.st_size);
	gchar *buffer_end = buffer+stats.st_size;
	FILE *idxfile;
        idxfile = g_fopen(idxfilename,"rb");
        fread (buffer, 1, stats.st_size, idxfile);
        fclose (idxfile);

	gchar *p=buffer;
	gchar *preword=NULL;
	int wordlen;
	gint cmpvalue;
	guint32 size;
	guint wordcount=0;
	bool have_errors=false;

	while (1) {
                if (p == buffer_end) {
                        break;
                }
		wordlen=strlen(p);
		if (wordlen==0)	{
			print_info("Error: wordlen==0\n");
		} else {
			if (wordlen>=256) {
				gchar *str = g_strdup_printf("Error: wordlen>=256, %s\n", p);
				print_info(str);
				g_free(str);
			}
			if (g_ascii_isspace(*p)) {
				gchar *str = g_strdup_printf("Warning: begin with space, %s\n", p);
				print_info(str);
				g_free(str);
			}
			if (g_ascii_isspace(*(p+wordlen-1))) {
				gchar *str = g_strdup_printf("Warning: end with space, %s\n", p);
				print_info(str);
				g_free(str);
			}
		}
		if (strpbrk(p, "\t\n")) {
			gchar *str = g_strdup_printf("Warning: contain invalid character, %s\n", p);
			print_info(str);
			g_free(str);
		}
		if (!g_utf8_validate(p, wordlen, NULL)) {
			gchar *str = g_strdup_printf("Error: invalid utf8 string, %s\n", p);
			print_info(str);
			g_free(str);
			have_errors=true;
		}
		if (preword) {
			cmpvalue=stardict_strcmp(preword, p);
			if (cmpvalue>0) {
				gchar *str = g_strdup_printf("Error: wrong string order, %s\n", p);
				print_info(str);
				g_free(str);
				have_errors=true;
			}
		}
		preword=p;
		p+=wordlen +1 + sizeof(guint32);
		size=*reinterpret_cast<guint32 *>(p);
		size=g_ntohl(size);
		if (size==0) {
			gchar *str = g_strdup_printf("Error: definition size==0, %s\n", preword);
			print_info(str);
			g_free(str);
		}
		p+=sizeof(guint32);
		wordcount++;
	}
	if (wordcount!=ifo_wordcount) {
		gchar *str = g_strdup_printf("Error: in .ifo file, wordcount=%d, while the real word count is %d\n", ifo_wordcount, wordcount);
		print_info(str);
		g_free(str);
		have_errors=true;
	}

	g_free(buffer);
	return have_errors ? EXIT_FAILURE : EXIT_SUCCESS;
}

int stardict_verify(const char *ifofilename, print_info_t print_info)
{
	gchar *buffer;

	g_file_get_contents(ifofilename, &buffer, NULL, NULL);
	if (!g_str_has_prefix(buffer, "StarDict's dict ifo file\nversion=2.4.2\n")) {
		g_free(buffer);
		return EXIT_FAILURE;
	}

	gchar *p1,*p2,*p3;

	guint wordcount;
	p1 = buffer;
	p2 = strstr(p1,"\nwordcount=");
	p3 = strchr(p2+ sizeof("\nwordcount=")-1,'\n');
	gchar *tmpstr = (gchar *)g_memdup(p2+sizeof("\nwordcount=")-1, p3-(p2+sizeof("\nwordcount=")-1)+1);
	tmpstr[p3-(p2+sizeof("\nwordcount=")-1)] = '\0';
	wordcount = atol(tmpstr);
	g_free(tmpstr);

	guint synwordcount=0;
	p2 = strstr(p1,"\nsynwordcount=");
	if (p2) {
		p3 = strchr(p2+ sizeof("\nsynwordcount=")-1,'\n');
		tmpstr = (gchar *)g_memdup(p2+sizeof("\nsynwordcount=")-1, p3-(p2+sizeof("\nsynwordcount=")-1)+1);
		tmpstr[p3-(p2+sizeof("\nsynwordcount=")-1)] = '\0';
		synwordcount = atol(tmpstr);
		g_free(tmpstr);
	}

	guint index_file_size;
	p2 = strstr(p1,"\nidxfilesize=");
	p3 = strchr(p2+ sizeof("\nidxfilesize=")-1,'\n');
	tmpstr = (gchar *)g_memdup(p2+sizeof("\nidxfilesize=")-1, p3-(p2+sizeof("\nidxfilesize=")-1)+1);
	tmpstr[p3-(p2+sizeof("\nidxfilesize=")-1)] = '\0';
	index_file_size = atol(tmpstr);
	g_free(tmpstr);

	g_free(buffer);

	bool have_errors = false;
	std::string idxfilename=ifofilename;
	idxfilename.replace(idxfilename.length()-sizeof("ifo")+1, sizeof("ifo")-1, "idx");
	int val = verify_idxfile(idxfilename.c_str(), wordcount, index_file_size, print_info);
	if (val==EXIT_FAILURE)
		have_errors = true;
	
	std::string synfilename=ifofilename;
	synfilename.replace(synfilename.length()-sizeof("ifo")+1, sizeof("ifo")-1, "syn");
	if (synwordcount) {
		val = verify_synfile(synfilename.c_str(), synwordcount, wordcount, print_info);
		if (val==EXIT_FAILURE)
			have_errors = true;
	} else {
		struct stat stats;
		if (g_stat (synfilename.c_str(), &stats) != -1) {
			print_info("Error: .syn file exists but no \"synwordcount=\" entry in .ifo file\n");
			have_errors = true;
		}
	}
	return have_errors ? EXIT_FAILURE : EXIT_SUCCESS;
}

