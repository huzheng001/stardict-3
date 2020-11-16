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

#include "stdio.h"
#include "stdlib.h"
#include <locale.h>
#include "zlib.h"
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>


#include <gtk/gtk.h>
#include <glib.h>

void convert_idxfile(gchar *idxfilename)
{
	g_print("converting file %s\n", idxfilename);
	gchar basefilename[256];
	strcpy(basefilename, idxfilename);
	if (g_str_has_suffix(idxfilename,".idx.gz"))
		basefilename[strlen(idxfilename)- (sizeof(".idx.gz") -1)] = '\0';
	else
		basefilename[strlen(idxfilename)- (sizeof(".idx") -1)] = '\0';
	
	gchar fullfilename[256];
	sprintf(fullfilename, "%s.bak", idxfilename);
	rename(idxfilename, fullfilename);
	
	gzFile in;	
	in = gzopen(fullfilename,"rb");
	if (in == NULL)
	{
		return;
	}	
	int len;
	char buf[409600];
	len = gzread(in,buf,sizeof(buf)-1);
	if (len < 0)
		return;
	if (len == 0) {
		gzclose(in);
		return;
	}
	
	buf[len] = '\0';
	gchar *idx_begin = strstr(buf,"\nBEGIN:\n") + sizeof("\nBEGIN:\n") -1 + sizeof(glong);
	gchar *bufhead = (gchar *)g_memdup(buf, idx_begin- buf);
		
	gint buffer_len = len - (idx_begin - buf);
	gchar *idxdatabuffer = (gchar *)g_memdup(idx_begin, buffer_len);
	for (;;) {
		len = gzread(in,buf,sizeof(buf));
		if (len < 0)
			return;
		if (len == 0)
			break;
		idxdatabuffer = (gchar *)g_realloc(idxdatabuffer,buffer_len + len);
		memcpy(idxdatabuffer + buffer_len, buf, len);			
		buffer_len += len;
	}
	gzclose(in);
	unlink(fullfilename);
	

	gchar newidxfilename[1024];
	sprintf(newidxfilename, "%s.idx", basefilename);
	FILE *file;
	if (!(file = fopen (newidxfilename, "wb"))) {
		return;
	}
	fwrite(idxdatabuffer, 1, buffer_len, file);
	fclose(file);
	
	sprintf(newidxfilename, "%s.ifo", basefilename);
	if (!(file = fopen (newidxfilename, "wb"))) {
		return;
	}
	fprintf(file, "StarDict's dict ifo file\nversion=2.4.2\n");
	
	glong tmpglong;
	idx_begin = strstr(bufhead,"\nBEGIN:\n") + sizeof("\nBEGIN:\n") -1;
	memcpy(&tmpglong, idx_begin, sizeof(glong));
	glong wordcount = g_ntohl(tmpglong);
	fprintf(file, "wordcount=%ld\n", wordcount);
	fprintf(file, "idxfilesize=%d\n", buffer_len);
	gchar *tmp_a = bufhead+ sizeof("StarDict's idx file\nversion=2.1.0\n") -1;
	gchar *tmp_b = strstr(tmp_a, "BEGIN:\n");
	gchar *opinion_str = g_strndup(tmp_a, tmp_b-tmp_a);
	fwrite(opinion_str, 1, strlen(opinion_str), file);
	fclose(file);
	
	g_free(bufhead);
	g_free(idxdatabuffer);
}

void convert_file(gchar *idxfilename)
{
	gzFile in;	
	in = gzopen(idxfilename,"rb");
	if (in == NULL) {
		return;
	}	
	int len;
	char buf[256];
	len = gzread(in,buf,sizeof(buf)-1);
	if (len < 0)
		return;
	if (len == 0) {
		gzclose(in);
		return;
	}
	gzclose(in);
	
	buf[len] = '\0';
	if (g_str_has_prefix(buf, "StarDict's idx file\nversion=2.1.0\n"))
		convert_idxfile(idxfilename);
}

void convert_dir(const gchar *dirname)
{
	GDir *dir = g_dir_open(dirname, 0, NULL);	
	if (dir) {
		const gchar *filename;	
		gchar fullfilename[256];
		while ((filename = g_dir_read_name(dir))!=NULL) {	
			sprintf(fullfilename, "%s/%s", dirname, filename);
			if (g_file_test(fullfilename, G_FILE_TEST_IS_DIR)) {
				convert_dir(fullfilename);
			}
			else if ((g_str_has_suffix(filename,".idx.gz")) || (g_str_has_suffix(filename,".idx"))) {
				convert_file(fullfilename);
			}
		}
		g_dir_close(dir);
	}
}


int
main(int argc,char * argv [])
{
	setlocale(LC_ALL, "");
	g_print("converting stadict-2.1.0 dictionary files to 2.4.2 file format...\n");
	convert_dir ("/usr/share/stardict/dic");
	gchar home_dir[256];
	sprintf(home_dir, "%s/.stardict/dic", g_get_home_dir());
	convert_dir(home_dir);
	g_print("Done!\n");

	return FALSE;
	
}
