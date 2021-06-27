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

// notice: here is a example_treedict.tar.bz2
// try "./directory2treedic example_treedict"

//use "ls > .order" to generate the .order file.

//only one type identifier is supported currently...who need more?

//If you have problem with filename convertion to utf, add 
//	G_BROKEN_FILENAMES=1
//	export G_BROKEN_FILENAMES
// to your ~/.bash_profile

#include "stdio.h"
#include "stdlib.h"
#include <locale.h>
#include <string.h>
#include <sys/stat.h>

#include <glib.h>

#include <string>


typedef struct MyDir{
	gboolean have_orderfile;
	GDir *dir;
	gchar *orderfile_buffer;
	gchar *p;
}MyDir;

MyDir *my_dir_open(const gchar *path)
{
	MyDir *mydir=(MyDir *)g_malloc(sizeof(MyDir));
	std::string orderfilename(path);
	orderfilename+="/.order";
	if (g_file_test(orderfilename.c_str(), G_FILE_TEST_EXISTS)) {
		mydir->have_orderfile = true;
		struct stat stats;
		stat (orderfilename.c_str(), &stats);
		FILE *orderfile;
		orderfile = fopen(orderfilename.c_str(),"r");
		mydir->orderfile_buffer = (gchar *)g_malloc (stats.st_size + 1);
		size_t fread_size;
		fread_size = fread (mydir->orderfile_buffer, 1, stats.st_size, orderfile);
		if (fread_size != (size_t)stats.st_size) {
			g_print("fread error!\n");
		}
		fclose (orderfile);
		mydir->orderfile_buffer[stats.st_size] = '\0';
		mydir->p = mydir->orderfile_buffer;
	} else {
		mydir->have_orderfile = false;
		mydir->dir = g_dir_open(path, 0, NULL);
	}
	return mydir;
}

const gchar *my_dir_read_name(MyDir *mydir)
{
	if (mydir->have_orderfile) {
		if (*(mydir->p) == '\0') {
			return NULL;
		} else {
			gchar *p1= strchr(mydir->p, '\n');
			*p1='\0';
			gchar *p2=mydir->p;
			mydir->p=p1+1;
			return p2;
		}
	} else {
		return g_dir_read_name(mydir->dir);
	}
}

void my_dir_close(MyDir *mydir)
{
	if (mydir->have_orderfile) {
		g_free(mydir->orderfile_buffer);
	} else {
		g_dir_close(mydir->dir);
	}
	g_free(mydir);
}

void add_dir(FILE *tdxfile, FILE *dicfile, char *dirname, gint32 *wordcount)
{
	gchar *utf8filename;
	gint32 tmpgint32, offset_old;
	gchar *p;
	gsize bytes_written;
	
	p = strrchr(dirname, '/');
	if (p)
		utf8filename = g_filename_to_utf8(p+1, -1, NULL, &bytes_written,NULL);
	else
		utf8filename = g_filename_to_utf8(dirname, -1, NULL, &bytes_written,NULL);
	if (!utf8filename) {
		g_print("filename %s convert to utf8 error!\n", dirname);
		return;
	}
	fwrite(utf8filename,sizeof(gchar), bytes_written+1, tdxfile);
	tmpgint32 = g_htonl(0);
	fwrite(&(tmpgint32),sizeof(gint32),1,tdxfile); //need more work...
	tmpgint32 = g_htonl(0);
	fwrite(&(tmpgint32),sizeof(gint32),1,tdxfile); //need more work...
	gint32 subcount_offset = ftell(tdxfile);
	tmpgint32 = g_htonl(0);
	fwrite(&(tmpgint32),sizeof(gint32),1,tdxfile); //it will be rewrite later.

	(*wordcount)++;
	
	gint32 subwordcount = 0;
	
	MyDir* mydir = my_dir_open(dirname);
	const gchar *filename;
	gchar fullfilename[256];
	gint len;
	FILE *file;
	struct stat stats;
	gchar *buffer = NULL;
	gint32 last_buffersize=0;
	gint dirname_len = strlen(dirname);
	while ((filename = my_dir_read_name(mydir))!=NULL) {
		if (filename[0]=='.') //hiden file.
			continue;
		subwordcount++;
		sprintf(fullfilename, "%s/%s", dirname, filename);
		if (g_file_test(fullfilename, G_FILE_TEST_IS_DIR)) {
			add_dir(tdxfile, dicfile, fullfilename, wordcount);
		}
		else {			
			len = strlen(filename);
			utf8filename = g_filename_to_utf8(fullfilename+ dirname_len+1, len -2, NULL, &bytes_written,NULL);			
			if (utf8filename) {				
				offset_old = ftell(dicfile);	
				stat (fullfilename, &stats);						
				if (last_buffersize < stats.st_size) {
					buffer = (gchar *)g_realloc(buffer, stats.st_size);
					last_buffersize = stats.st_size;
				}
				file = fopen(fullfilename,"r");
				size_t fread_size;
				fread_size = fread (buffer, 1, stats.st_size, file);
				if (fread_size != (size_t)stats.st_size) {
					g_print("fread error!\n");
				}
				fclose (file);
				fwrite(buffer, 1, stats.st_size, dicfile);				
								
				fwrite(utf8filename,sizeof(gchar), bytes_written+1, tdxfile);
				tmpgint32 = g_htonl(offset_old);
				fwrite(&(tmpgint32),sizeof(gint32),1,tdxfile);
				tmpgint32 = g_htonl(stats.st_size);
				fwrite(&(tmpgint32),sizeof(gint32),1,tdxfile);
				tmpgint32 = g_htonl(0);
				fwrite(&(tmpgint32),sizeof(gint32),1,tdxfile);
				
				g_free(utf8filename);								
				(*wordcount)++;
			}
			else {
				g_print("filename %s convert to utf8 error!\n", fullfilename);
				return;
			}
		}
	}
	my_dir_close(mydir);
	g_free(buffer);
	
	gint32 tmp_offset = ftell(tdxfile);
	fseek(tdxfile,subcount_offset,SEEK_SET);
	tmpgint32 = g_htonl(subwordcount);
	fwrite(&(tmpgint32),sizeof(gint32),1,tdxfile);
	fseek(tdxfile,tmp_offset,SEEK_SET);
}

void convert(char *dirname)
{	
	struct stat stats;
	
	if (stat (dirname, &stats) == -1)
	{
		printf("directory not exist!\n");
		return;
	}		
		
	gchar filename[256];
	
	sprintf(filename, "%s/.ifo", dirname);
	if (stat (filename, &stats) == -1)
	{
		printf("%s not exist!\n", filename);
		return;
	}		
	FILE *ifofile;
	ifofile = fopen(filename, "r");
	gchar *buffer;
	gint buffer_len = stats.st_size;
	buffer = (gchar *)g_malloc (stats.st_size);
	size_t fread_size;
	fread_size = fread (buffer, 1, stats.st_size, ifofile);
	if (fread_size != (size_t)stats.st_size) {
		g_print("fread error!\n");
	}
	fclose (ifofile);

	FILE *tdxfile,*dicfile;
	sprintf(filename, "%s.tdx", dirname);
	tdxfile = fopen(filename,"w");
	sprintf(filename, "%s.dict", dirname);
	dicfile = fopen(filename,"w");


	gint32 wordcount = 0;
	
	add_dir(tdxfile, dicfile, dirname, &wordcount);
		
	g_print("wordcount: %d\n",wordcount);
	
	fclose(tdxfile);
	fclose(dicfile);

	sprintf(filename, "%s.ifo", dirname);
	ifofile = fopen(filename,"w");
	fprintf(ifofile, "StarDict's treedict ifo file\nversion=2.4.2\n");
	fprintf(ifofile, "wordcount=%d\n", wordcount);
	sprintf(filename, "%s.tdx", dirname);
	stat (filename, &stats);
	fprintf(ifofile, "tdxfilesize=%ld\n", (long) stats.st_size);
	fwrite(buffer, 1, buffer_len, ifofile);
	g_free(buffer);
	fclose(ifofile);


	gchar command[256];
	sprintf(command, "gzip -9 %s.tdx -f", dirname);
	int result;
	result = system(command);
	if (result == -1) {
		g_print("system() error!\n");
	}
	sprintf(command, "dictzip %s.dict -f", dirname);
	result = system(command);
	if (result == -1) {
		g_print("system() error!\n");
	}
}

int
main(int argc,char * argv [])
{
	if (argc!=2) {
		printf("please type this:\n./directory2treedic InfoBrowse\n");
		return FALSE;
	}

	setlocale(LC_ALL, "");
	convert (argv[1]);
	return FALSE;	
}
