#include <string.h>
#include <stdlib.h>
#include <glib/gstdio.h>
#include <glib.h>

#include <string>

#include "libsd2foldoc.h"


static void convert2foldocfile(const gchar *ifofilename, print_info_t print_info)
{
	std::string idxfilename=ifofilename;
	idxfilename.replace(idxfilename.length()-sizeof("ifo")+1, sizeof("ifo")-1, "idx");
	struct stat idx_stats;
	if (g_stat (idxfilename.c_str(), &idx_stats) == -1) {
		gchar *str = g_strdup_printf("File not exist: %s\n", idxfilename.c_str());
		print_info(str);
		g_free(str);
		return;
	}
	std::string dictfilename=ifofilename;
        dictfilename.replace(dictfilename.length()-sizeof("ifo")+1, sizeof("ifo")-1, "dict");
        struct stat dict_stats;
        if (g_stat (dictfilename.c_str(), &dict_stats) == -1) {
	  gchar *str = g_strdup_printf("File not exist: %s\nPlease do \"mv somedict.dict.dz \
			somedict.dict.gz;gunzip somedict.dict.gz\"\n", dictfilename.c_str());
	  print_info(str);
	  g_free(str);
	  return;
        }
	gchar *idxbuffer = (gchar *)g_malloc (idx_stats.st_size);
	gchar *idxbuffer_end = idxbuffer+idx_stats.st_size;
	FILE *idxfile;
	idxfile = g_fopen(idxfilename.c_str(),"rb");
	size_t fread_size;
	fread_size = fread (idxbuffer, 1, idx_stats.st_size, idxfile);
	if (fread_size != (size_t)idx_stats.st_size) {
		g_print("fread error!\n");
	}
	fclose (idxfile);

	gchar *dictbuffer = (gchar *)g_malloc (dict_stats.st_size);
        FILE *dictfile;
        dictfile = g_fopen(dictfilename.c_str(),"rb");
        fread_size = fread (dictbuffer, 1, dict_stats.st_size, dictfile);
	if (fread_size != (size_t)dict_stats.st_size) {
		g_print("fread error!\n");
	}
        fclose (dictfile);

	gchar *basefilename = g_path_get_basename(ifofilename);
	std::string txtfilename=basefilename;
	g_free(basefilename);
	txtfilename.replace(txtfilename.length()-sizeof("ifo")+1, sizeof("ifo")-1, "txt");
	gchar *str = g_strdup_printf("Write to file: %s\n", txtfilename.c_str());
	print_info(str);
	g_free(str);
	FILE *txtfile;
	txtfile = g_fopen(txtfilename.c_str(),"w");

	gchar *p=idxbuffer;
	int wordlen;
	guint32 offset, size;
	gchar *data;
	while (1) {
		if (p == idxbuffer_end) {
			break;
		}
		wordlen=strlen(p);
		fwrite(p, wordlen, 1, txtfile);
		// headwords start in col 0, definitions start in col 8 
		// see dictfmt --help for details
		fwrite("\n", 1, 1, txtfile);
		fwrite("\t", 1, 1, txtfile);

		p+=wordlen +1;
		offset=*reinterpret_cast<guint32 *>(p);
		offset=g_ntohl(offset);
		p+=sizeof(guint32);
		size=*reinterpret_cast<guint32 *>(p);
		size=g_ntohl(size);
		p+=sizeof(guint32);
		data=dictbuffer+offset;
		// write word definitions
		while ((guint32)(data-(dictbuffer+offset))<size) {
			switch (*data) {
				case '\n':
					fwrite("\n", 1, 1, txtfile);
					fwrite("\t", 1, 1, txtfile);
					break;
				case '\\':
					fwrite("\\\\", 2, 1, txtfile);
					break;
				default:
					fwrite(data, 1, 1, txtfile);
					break;
			}
			data++;
		}
		fwrite("\n", 1, 1, txtfile);
	}
	fclose(txtfile);
	g_free(idxbuffer);
	g_free(dictbuffer);
}

void sd2foldoc(const char *ifofilename, print_info_t print_info)
{
	gchar *buffer;
	g_file_get_contents(ifofilename, &buffer, NULL, NULL);
	if (!g_str_has_prefix(buffer, "StarDict's dict ifo file\nversion=2.4.2\n")) {
		print_info("Error, file version is not 2.4.2\n");
		g_free(buffer);
		return;
	}
	gchar *p1,*p2;

        p1 = buffer;
        p2 = strstr(p1,"\nsametypesequence=");
	if (!p2) {
		print_info("Error, no sametypesequence=\n");
		g_free(buffer);
		return;
	}
	p2 += sizeof("\nsametypesequence=") -1;
	if (*p2=='m' && *(p2+1)=='\n') {
		convert2foldocfile(ifofilename, print_info);
	} else {
		print_info("Error, sametypesequence is not m\n");
		g_free(buffer);
		return;
	}
	g_free(buffer);
}

