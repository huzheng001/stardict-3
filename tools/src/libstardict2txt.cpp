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

#include <string.h>
#include <stdlib.h>
#include <glib/gstdio.h>
#include <glib.h>

#include <string>

#include "libstardict2txt.h"

static void convert2tabfile(const gchar *ifofilename, const gchar* txtfilename)
{
	std::string idxfilename=ifofilename;
	idxfilename.replace(idxfilename.length()-sizeof("ifo")+1, sizeof("ifo")-1, "idx");
	stardict_stat_t idx_stats;
	if (g_stat (idxfilename.c_str(), &idx_stats) == -1) {
		g_critical("File not exist: %s.", idxfilename.c_str());
		return;
	}
	std::string dictfilename=ifofilename;
	dictfilename.replace(dictfilename.length()-sizeof("ifo")+1, sizeof("ifo")-1, "dict");
	stardict_stat_t dict_stats;
	if (g_stat (dictfilename.c_str(), &dict_stats) == -1) {
#ifdef _WIN32
		g_critical("File not exist: %s\nPlease rename somedict.dict.dz to somedict.dict.gz and use SevenZip to uncompress the somedict.dict.gz file, then you can get the somedict.dict file.", dictfilename.c_str());
#else
		g_critical("File not exist: %s\nPlease do \"mv somedict.dict.dz somedict.dict.gz;gunzip somedict.dict.gz\"", dictfilename.c_str());
#endif
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
		g_print("fread error 2!\n");
	}
	fclose (dictfile);

	g_message("Writing to file: %s.", txtfilename);
	FILE *txtfile;
	txtfile = g_fopen(txtfilename,"w");

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
		fwrite("\t", 1, 1, txtfile);
		p+=wordlen +1;
		offset=*reinterpret_cast<guint32 *>(p);
		offset=g_ntohl(offset);
		p+=sizeof(guint32);
		size=*reinterpret_cast<guint32 *>(p);
		size=g_ntohl(size);
		p+=sizeof(guint32);
		data=dictbuffer+offset;
		while ((guint32)(data-(dictbuffer+offset))<size) {
			switch (*data) {
				case '\n':
					fwrite("\\n", 2, 1, txtfile);
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

void convert_stardict2txt(const char *ifofilename, const char* txtfilename)
{
	gchar *buffer;
	g_file_get_contents(ifofilename, &buffer, NULL, NULL);
	if (!g_str_has_prefix(buffer, "StarDict's dict ifo file\nversion=")) {
		g_critical("Error, file version is not 2.4.2.");
		g_free(buffer);
		return;
	}
	gchar *p1,*p2;

	p1 = buffer;
	p2 = strstr(p1,"\nsametypesequence=");
	if (!p2) {
		g_critical("Error, no sametypesequence=.");
		g_free(buffer);
		return;
	}
	p2 += sizeof("\nsametypesequence=") -1;
	if (g_ascii_islower(*p2) && *(p2+1)=='\n') {
		convert2tabfile(ifofilename, txtfilename);
	} else {
		g_critical("Error, sametypesequence must be a single lower case letter, preferably 'm'.");
		g_free(buffer);
		return;
	}
	g_free(buffer);
}
