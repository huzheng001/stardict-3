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
#include <glib.h>
#include <glib/gprintf.h>

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

typedef struct
{
char *words;
char *trans;
}
PAIR;

PAIR *arr;

long off, siz, n, nn, i, j;
char same, di;
FILE *F, *F2, *F3;
char *p, *p2;
char *current, *current2;
char prev[200];

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
time_t t0;
struct tm *t;

arr=(PAIR *)malloc(sizeof(PAIR)*40000);

setbuf(stdout, NULL);

for (di=0; di<2; di++)
   {
   F=fopen((argc>1)? argv[1]: "i2e.dict.utf", "rt");
   if (!F)
      {
      printf("\nFile %s not found\n\n", (argc>1)? argv[1]: "i2e.dict.utf");
      exit(1);
      }
   
   *prev=n=off=0;
   current=current2=malloc(400);
   
   while (fgets((!di)? current: current2, 200, F))
      {
      p=strstr((!di)? current: current2, " : ");
      *p=0;
      if (!di) current2=p+3; else current=strdup(p+3);
      p2=strpbrk((!di)? current2: current, "\n\r");
      if (p2) *p2=0;
      p2=strchr(current, '(');
      if (p2)
         {
         if (!strstr(current2, p2)) strcat(current2, p2-1);
         p2[-1]=0;
         }
      arr[n].words=(!di)? strdup(current): current;
      arr[n].trans=strdup(current2);
      n++;
      
      if (g_utf8_validate(current, -1, NULL)!=TRUE ||
      g_utf8_validate(current2, -1, NULL)!=TRUE) 
         {
         fprintf(stderr, "\nError! Invalid character found, first convert to UTF-8.\n");
         fprintf(stderr, "Example: recode latin1..utf-8 <i2e.dict >i2e.dict.utf\n\n");
         exit(1);
         }
      
      
      }
   free((!di)? current: current2);
   printf("\nSorting %s, please wait...\n", (!di)? "English": "Spanish");
   
   qsort(arr, n, sizeof(PAIR), cmp);
   
   F2=fopen((!di)? "/usr/share/stardict/dic/i2e.idx": "/usr/share/stardict/dic/e2i.idx", "wb");
   F3=fopen((!di)? "/usr/share/stardict/dic/i2e.dict": "/usr/share/stardict/dic/e2i.dict", "wb");
   
   nn=0;
   for (i=0; i<n; i++)
      {
      same=0;
      
      current=arr[i].words;
      current2=arr[i].trans;
      if (*prev) if (strcmp(prev, current)==0)
         {
         same=1;
         nn--;
         }
      nn++;
      strcpy(prev, current);
      
      printf("%s\t%s\n", current, current2);
      
      if (same) 
         {
         fwrite("\n", 1, 1, F3);
         siz++;
         off++;
         }
      else siz=0;
      
      siz+=strlen(current2);
      
      fwrite(current2, strlen(current2), 1, F3);
      
      if (!same)
         {
         fwrite(current, strlen(current)+1, 1, F2);
         for (j=3; j>=0; j--)
         fwrite(((char*)&off)+j, 1, 1, F2);
         }
      else fseek(F2, -4L, SEEK_END); 
      
      for (j=3; j>=0; j--)	
      fwrite(((char*)&siz)+j, 1, 1, F2);
      off+=strlen(current2);
      free(arr[i].words);
      free(arr[i].trans);
      }
   fclose(F3);
   
   F3=fopen((!di)? "/usr/share/stardict/dic/i2e.ifo": "/usr/share/stardict/dic/e2i.ifo", "wt");
   
   time(&t0);
   t=gmtime(&t0);
   
   fprintf(F3, "StarDict's dict ifo file\nversion=2.4.2\nwordcount=%li\nidxfilesize=%li\nbookname=I2E ", nn, ftell(F2));
   fprintf(F3, (!di)? "English-Spanish": "Spanish-English");
   fprintf(F3, "\ndate=%i.%02i.%02i\nsametypesequence=m\n",
   t->tm_year+1900, t->tm_mon+1, t->tm_mday);
   
   fclose(F);
   fclose(F2);
   fclose(F3);
   printf("\nTotal %li entries written.\n", nn);
   }
free(arr);

return 0;
}
