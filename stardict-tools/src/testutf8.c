/*
*  This program is free software; you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation; either version 2 of the License, or
*  (at your option) any later version.
*
*  This program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU Library General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with this program; if not, write to the Free Software
*  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>

long off, siz, n, i, idxsiz;
char same, j;
FILE *F, *F2;
char *p, *p2, *idx;
char *words;

int main (int argc, char *argv[])
{
if (argc<2)
	{
	printf("\nMissing dict name!\n");
	getchar();
	exit(1);
	}
words=(char *)malloc(300);

sprintf(words, "/usr/share/stardict/dic/%s.ifo", argv[1]);
F=fopen(words, "rt");

while (fgets(words, 200, F))
   {
   if (strncmp(words, "wordcount=", 10)==0) n=atol(words+10);
   if (strncmp(words, "idxfilesize=", 12)==0) idxsiz=atol(words+12);
   }
fclose(F);
sprintf(words, "/usr/share/stardict/dic/%s.idx", argv[1]);
F=fopen(words, "rb");
sprintf(words, "/usr/share/stardict/dic/%s.dict", argv[1]);
F2=fopen(words, "rb");

idx=malloc(idxsiz);
fread(idx, idxsiz, 1, F);
fclose(F);

for (p=idx, i=0; i<n; i++)
   {
   if (g_utf8_validate(p, -1, NULL)!=TRUE) fprintf(stderr, "Error: %s\n", p);
   p=strchr(p, 0)+1;
   for (j=3; j>=0; j--)
   ((char*)&off)[(int)j]=*p++;
   for (j=3; j>=0; j--)
   ((char*)&siz)[(int)j]=*p++;
   fseek(F2, off, SEEK_SET);
   words=(char *)realloc(words, siz+1);
   fread(words, siz, 1, F2);
   words[siz]=0;
   if (g_utf8_validate(words, -1, NULL)!=TRUE) fprintf(stderr, "Error: %s\n", words);
   }
fclose(F2);
return 0;
}
