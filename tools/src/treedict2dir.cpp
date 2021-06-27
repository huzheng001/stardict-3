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
#include <string.h>
#include <sys/stat.h>

#include <glib.h>

#include <string>

void write_file(gchar **buffer, const char *cur_dir, int count, char *dict_buffer)
{
	gchar *p1;
	glong tmpglong;
	glong offset, size, subentry_count;
	FILE *datafile;
	std::string datafilename;

	FILE *orderfile;
	std::string orderfilename;
	orderfilename=cur_dir;
	orderfilename+="/.order";
	orderfile=fopen(orderfilename.c_str(), "wb");

	for (int i=0; i< count; i++) {
		p1 = *buffer + strlen(*buffer) +1;
		memcpy(&(tmpglong),p1,sizeof(glong));
		offset = g_ntohl(tmpglong);
		p1 += sizeof(glong);
                memcpy(&(tmpglong),p1,sizeof(glong));
                size = g_ntohl(tmpglong);
                p1 += sizeof(glong);
                memcpy(&(tmpglong),p1,sizeof(glong));
                subentry_count = g_ntohl(tmpglong);
                p1 += sizeof(glong);
		
		std::string dirname;
		if (subentry_count) {
			fprintf(orderfile, "%s\n", *buffer);
			dirname=cur_dir;
			dirname+='/';
                	dirname+=*buffer;
                	mkdir(dirname.c_str(), S_IRWXU);
		} else {
			fprintf(orderfile, "%s.m\n", *buffer);
			datafilename=cur_dir;
			datafilename+='/';
			datafilename+=*buffer;
			datafilename+=".m";
			datafile=fopen(datafilename.c_str(), "wb");
			fwrite(dict_buffer+offset, 1, size, datafile);
			fclose(datafile);
		}
		*buffer = p1;
                if (subentry_count)
			write_file(buffer, dirname.c_str(), subentry_count, dict_buffer);
	}
	fclose(orderfile);
}

void convert_tdx(char *ifofilename, char *tdx_buffer)
{
        std::string dictfilename(ifofilename);
        dictfilename.replace(dictfilename.length()-sizeof("ifo")+1, sizeof("ifo")-1, "dict");
        gchar *dict_buffer;
        if (!g_file_get_contents(dictfilename.c_str(), &dict_buffer, NULL, NULL)) {
                printf("File %s not exist!\n", dictfilename.c_str());
                return;
        }

	gchar *tmp_buffer = tdx_buffer;
	write_file(&tmp_buffer, ".", 1, dict_buffer);
	g_free(dict_buffer);
}

void convert_ifo(char *ifofilename)
{	
	gchar *buffer;
	if (!g_file_get_contents(ifofilename, &buffer, NULL, NULL)) {
		printf("ifo file not exist!\n");
		return;
	}
#define TREEDICT_MAGIC_DATA "StarDict's treedict ifo file\nversion=2.4.2\n"
	if (!g_str_has_prefix(buffer, TREEDICT_MAGIC_DATA)) {
		g_free(buffer);
		return;
	}
	gchar *p1,*p2,*p3;
	p1 = buffer + sizeof(TREEDICT_MAGIC_DATA)-2;

	std::string bookname;
	p2 = strstr(p1,"\nbookname=");
	p2 = p2 + sizeof("\nbookname=") -1;
	p3 = strchr(p2, '\n');
	bookname.assign(p2, p3-p2);

	std::string author;	
	p2 = strstr(p1,"\nauthor=");
	if (p2) {
		p2 = p2 + sizeof("\nauthor=") -1;
		p3 = strchr(p2, '\n');
		author.assign(p2, p3-p2);
	}

	std::string email;	
	p2 = strstr(p1,"\nemail=");
	if (p2) {
		p2 = p2 + sizeof("\nemail=") -1;
		p3 = strchr(p2, '\n');
		email.assign(p2, p3-p2);
	}

	std::string website;
	p2 = strstr(p1,"\nwebsite=");
	if (p2) {
		p2 = p2 + sizeof("\nwebsite=") -1;
		p3 = strchr(p2, '\n');
		website.assign(p2, p3-p2);
	}

	std::string date;
	p2 = strstr(p1,"\ndate=");
	if (p2) {
		p2 = p2 + sizeof("\ndate=") -1;
		p3 = strchr(p2, '\n');
		date.assign(p2, p3-p2);
	}

	std::string description;
	p2 = strstr(p1,"\ndescription=");
	if (p2) {
		p2 = p2 + sizeof("\ndescription=")-1;
		p3 = strchr(p2, '\n');
		description.assign(p2, p3-p2);
	}

	std::string sametypesequence;
	p2 = strstr(p1,"\nsametypesequence=");
	if (p2) {
		p2+=sizeof("\nsametypesequence=")-1;
		p3 = strchr(p2, '\n');
		sametypesequence.assign(p2, p3-p2);
	}

	g_free(buffer);


	std::string tdxfilename(ifofilename);
	tdxfilename.replace(tdxfilename.length()-sizeof("ifo")+1, sizeof("ifo")-1, "tdx");
	gchar *tdx_buffer;
        if (!g_file_get_contents(tdxfilename.c_str(), &tdx_buffer, NULL, NULL)) {
                printf("File %s not exist!\n", tdxfilename.c_str());
                return;
        }
	mkdir(tdx_buffer, S_IRWXU);

	std::string ifodatafilename(tdx_buffer);
	ifodatafilename += "/.ifo";
	FILE *ifodatafile;
	ifodatafile = fopen(ifodatafilename.c_str(), "wb");
	if (!bookname.empty())
		fprintf(ifodatafile, "bookname=%s\n", bookname.c_str());
	if (!author.empty())
                fprintf(ifodatafile, "author=%s\n", author.c_str());
	if (!email.empty())
                fprintf(ifodatafile, "email=%s\n", email.c_str());
	if (!website.empty())
                fprintf(ifodatafile, "website=%s\n", website.c_str());
	if (!date.empty())
                fprintf(ifodatafile, "date=%s\n", date.c_str());
        if (!description.empty())
                fprintf(ifodatafile, "description=%s\n", description.c_str());
        if (!sametypesequence.empty())
                fprintf(ifodatafile, "sametypesequence=%s\n", sametypesequence.c_str());
	fclose(ifodatafile);

	convert_tdx(ifofilename, tdx_buffer);
	g_free(tdx_buffer);
}

int
main(int argc,char * argv [])
{
	if (argc!=2) {
		printf("please type this:\n./treedict2dir infoBrowse-zh_CN.ifo\n");
		return FALSE;
	}

	setlocale(LC_ALL, "");
	convert_ifo (argv[1]);
	return FALSE;	
}
