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

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <glib.h>

typedef struct
{
char *words;
char *trans;
}
PAIR;

PAIR *arr;

gint stardict_strcmp(const gchar *s1, const gchar *s2)
{
gint a;
a = g_ascii_strcasecmp(s1, s2);
if (a == 0)
return strcmp(s1, s2);
else
return a;
}


int cmp(const void *s1, const void *s2)
{
PAIR *a, *b;

a= (PAIR *)s1;
b= (PAIR *)s2;

return stardict_strcmp(a->words, b->words);
}

int main (int argc, char *argv[])
{
long off, siz;
FILE *F, *F2, *F3;
long i, j, n, nn;
char *p, *p2, *ext;
char lang[50];
char fname[100];
char *current;
char *current2;
char *ch;

time_t t0;
struct tm *t;

arr=(PAIR *)malloc(sizeof(PAIR)*200000);

current=malloc(10000);
current2=malloc(10000);

fprintf(stderr, "Enter thesaurus language [WordNet_English]: ");
fflush(stderr);
ch = fgets(lang, sizeof(lang), stdin);
if (ch == NULL) {
	printf("fgets error!\n");
}
if ((p=strchr(lang, '\n'))!=NULL) *p=0;
if (*lang==0) strcpy(lang, "WordNet_English");
F=fopen((argc>1)? argv[1]: "/usr/share/myspell/dicts/th_en_US_v2.dat", "rt");
if (!F)
   {
   printf("\nFile %s not found\n\n", (argc>1)? argv[1]: "/usr/share/myspell/dicts/th_en_US_v2.dat");
   exit(1);
   }

ch = fgets(current2, 200, F);
if (ch == NULL) {
	printf("fgets error!\n");
}

nn=n=j=off=siz=0;

while (fgets(current, 10000, F))
   {
   p=strchr(current, '|');
   
   *p=0;
   j=atol(p+1);
   char *ch;
   for (i=*current2=0; i<j; i++)
      {
      p2=strchr(current2, 0);
      if (j>1) 
         {
         sprintf(p2, "%li. ", i+1);
         p2=strchr(p2, 0); 
         }
      ch = fgets(p2, 10000, F);
      if (ch == NULL) {
          printf("fgets error!\n");
      }
      p=strchr(p2, '|');
      *(p++)=' ';
      if (strncmp(p2, "(noun)", 6)==0 || strncmp(p2, "(verb)", 6)==0) memmove(p2+2, p2+5, strlen(p2+5)+1);
      else if (strncmp(p2, "(adj)", 5)==0) memmove(p2+2, p2+4, strlen(p2+4)+1);
      else if (strncmp(p2, "- ", 2)==0) memmove(p2, p2+2, strlen(p2+2)+1);
      
      for (p=p2; (p=strstr(p2, "(generic term)"))!=NULL; p++)
         {
         p[1]='>';
         memmove(p+2, p+13, strlen(p+13)+1);
         }
      for (p=p2; (p=strstr(p2, "(similar term)"))!=NULL; p++)
         {
         p[1]='~';
         memmove(p+2, p+13, strlen(p+13)+1);
         }
      
      for (p=p2; (p=strchr(p, '|'))!=NULL; p++)
         {
         *(p++)=',';
         memmove(p+1, p, strlen(p)+1);
         *p=' ';
         }
      }
   
   p=strchr(current2, 0);
   if (p[-1]=='\n') p[-1]=0;
   if (g_utf8_validate(current, -1, NULL)!=TRUE ||
   g_utf8_validate(current2, -1, NULL)!=TRUE) 
      {
      fprintf(stderr, "\nError! Invalid character found, first convert to UTF-8.\n");
      fprintf(stderr, "Example: recode latin1..utf-8 <th_de_DE_v2.dat >th_de_DE_v2.dat.utf\n\n");
      exit(1);
      }
   
   arr[nn].words=strdup(current);
   arr[nn].trans=strdup(current2);
   //printf("%s:\n%s\n\n", words[nn], trans[nn]);
   nn++;
   }
free(current);
free(current2);

fprintf(stderr, "\nSorting %s, please wait...\n", lang);
fflush(stderr);

qsort(arr, nn, sizeof(PAIR), cmp);

sprintf(fname, "/usr/share/stardict/dic/ooothes_%s.", lang);
ext=strchr(fname, 0);
strcpy(ext, "idx");
F2=fopen(fname, "wb");
strcpy(ext, "dict");
F3=fopen(fname, "wb");

for (i=0; i<nn; i++)
   {
   current=arr[i].words;
   current2=arr[i].trans;
   
   siz=strlen(current2);
   
   fwrite(current2, strlen(current2), 1, F3);
   
   fwrite(current, strlen(current)+1, 1, F2);
   for (j=3; j>=0; j--)
   fwrite(((char*)&off)+j, 1, 1, F2);
   
   for (j=3; j>=0; j--)	
   fwrite(((char*)&siz)+j, 1, 1, F2);
   off+=strlen(current2);
   free(current);
   free(current2);
   }

fclose(F3);
strcpy(ext, "ifo");
F3=fopen(fname, "wt");

printf("\nTotal %li entries written.\n", nn);

time(&t0);
t=gmtime(&t0);

fprintf(F3, "StarDict's dict ifo file\nversion=2.4.2\nwordcount=%li\nidxfilesize=%li\n", nn, ftell(F2));
fprintf(F3, "bookname=%s thesaurus\ndate=%i.%02i.%02i\nsametypesequence=m\n", lang,
t->tm_year+1900, t->tm_mon+1, t->tm_mday);

free(arr);
fclose(F);
fclose(F2);
fclose(F3);
return 0;
}
