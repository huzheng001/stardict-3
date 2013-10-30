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


typedef struct
{
char c;
char *from;
char *to;
int ml;
gunichar **mask;
char *mor;
}
RULE;

RULE *rules;

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
int rez;
PAIR *a, *b;

a= (PAIR *)s1;
b= (PAIR *)s2;

rez=stardict_strcmp(a->words, b->words);
//if (rez==0) rez=stardict_strcmp(a->trans, b->trans);
return rez;
}


int MatchesMask(char *s, RULE *r)
{
char *p, rez;
int i, len;
long cir, dot;
gunichar c, *p2;

len=g_utf8_strlen(s, -1);
if (len<=r->ml) return 0;
cir=g_utf8_get_char_validated("^", -1);
dot=g_utf8_get_char_validated(".", -1);

p=g_utf8_offset_to_pointer(s, len-r->ml);
for (i=0; i<r->ml; i++, p=g_utf8_next_char(p))
   {
   p2=r->mask[i];
   if (*p2==dot) continue;
   c=g_utf8_get_char_validated(p, -1);
   rez=(*p2==cir)? 1: 0;
   if (rez)	p2++;
   
   for ( ; *p2; p2++)
   if (*p2==c)
      {
      rez=1-rez;
      break;		
      }
   if (!rez) return 0;
   }
return 1;
}


int main (int argc, char *argv[])
{
long off, siz, n, nn, i, j;
char same;
FILE *F, *F2, *F3;
char *p, *p2;
char *current, *current2;
char buf[20000];
char prev[200];
char prev2[200];
char lang[50];
char fname[100];
time_t t0;
struct tm *t;
long nr, len, flen, common;
char *ch;

arr=(PAIR *)malloc(sizeof(PAIR)*10000000);
rules=(RULE *)malloc(sizeof(RULE)*10000);

current=malloc(200);

//setbuf(stdout, 0);
if (argc<3) 
   {
   printf("\nUsage:\n\nmyspell2dic es_ES.dic.utf es_ES.aff.utf\n\n");
   exit(0);
   }

fprintf(stderr, "Enter grammar language [Spanish]: ");
fflush(stderr);
ch = fgets(lang, sizeof(lang), stdin);
if (ch == NULL) {
    printf("fgets error!\n");
}
if ((p=strchr(lang, '\n'))!=NULL) *p=0;
if (*lang==0) strcpy(lang, "Spanish");

F=fopen(argv[2], "rt");

if (!F)
   {
   printf("\nFile %s not found\n\n", argv[2]);
   exit(1);
   }

nr=0;

while (fgets(current, 200, F))
if (strncmp(current, "SFX ", 4)==0)
   {
   
   rules[nr].c=*strtok(current+4, " \t\n");
   p=strtok(NULL, " \t\n");
   if (!p) continue;
   p=strdup(p);
   if (*p=='0') *p=0;
   rules[nr].from=p;
   p=strtok(NULL, " \t\n");
   if (!p) continue;
   p=strdup(p);
   if (*p=='0') *p=0;
   rules[nr].to=p;
   p=strtok(NULL, " \t\n");
   if (!p) continue;
   rules[nr].mor=strdup(p);
   
   rules[nr].mask=malloc(sizeof(gunichar *));
   for (i=0; *p; p=g_utf8_next_char(p), i++)
      {
      if (g_utf8_validate(p, -1, NULL)!=TRUE) exit(1);
      
      if (*p=='[')
         {
         p2=g_utf8_strchr(p, -1, ']');
         *p2=0;
         rules[nr].mask[i]=g_utf8_to_ucs4(p+1, -1, 0, 0, 0);
         *p2=']';
         
         p=p2;
         }
      else 
         {
         rules[nr].mask[i]=malloc(sizeof(gunichar *)*2);
         rules[nr].mask[i][0]=g_utf8_get_char_validated(p, -1);
         rules[nr].mask[i][1]=0;
         }
      rules[nr].mask=(gunichar **)realloc(rules[nr].mask, sizeof(gunichar *)*(i+2));
      }
   rules[nr++].ml=i;
   }
fclose(F);
F=fopen(argv[1], "rt");

if (!F)
   {
   printf("\nFile %s not found\n\n", argv[1]);
   exit(1);
   }

fprintf(stderr, "\nConversion can take several minutes, please wait...\n");
fflush(stderr);

ch = fgets(current, 200, F);
if (ch == NULL) {
    printf("fgets error!\n");
}
n=0;
while (fgets(current, 200, F))
   {
   p=strchr(current, '/');
   if (!p) continue;
   *buf=0;
   //printf("\n%s/", current);
   for (*(p++)=0; *p>' '; p++)
      {
		//printf("%c:\n", *p);
		if (*buf!=0) 
			{strcat(buf, "\n\n");
			}
      
      for (i=0; i<nr; i++)
      if (*p==rules[i].c) 
         {
         
         len=strlen(current);
         flen=strlen(rules[i].from);
         common=len-flen;
         
         if (common>0)
            {
            if (MatchesMask(current, rules+i))
               {
               current2=malloc(len-flen+strlen(rules[i].to)+1);
               strncpy(current2, current, (*rules[i].from==0)? len: common);
               strcpy(current2+((*rules[i].from==0)? len: common), rules[i].to);
               
               if (g_utf8_validate(current, -1, NULL)!=TRUE ||
               g_utf8_validate(current2, -1, NULL)!=TRUE) 
                  {
                  fprintf(stderr, "\nError! Invalid character found, first convert to UTF-8.\n\n");
                  fprintf(stderr, "Examples:\nrecode latin1..utf-8 <es_ES.dic >es_ES.dic.utf\n");
                  fprintf(stderr, "recode KOI8-RU..utf-8 <uk_UA.aff >uk_UA.aff.utf\n");
                  fprintf(stderr, "The encoding is specified in .aff file.");
                  exit(1);
                  }
               //arr[n].words=strdup(current);
               //arr[n].trans=current2;
               //n++;
               p2=strchr(buf, 0);
               if (*buf!=0) if (*(p2-1)!='\n') strcpy(p2++, " ");
               
               strcpy(p2, current2);
               
               arr[n].words=strdup(current2);
               arr[n].trans=malloc(strlen(current)+6);
               sprintf(arr[n].trans, "=> {%s}", current);
               //printf("%s\n", current2);
               n++;
               if (n>9999990) arr=(PAIR *)malloc(sizeof(PAIR)*100000000);
               }
            }
         
         
         }
      //printf("%s                     \r", current);
      }
      arr[n].words=strdup(current);
      arr[n].trans=strdup(buf);
      n++;
   }
fprintf(stderr, "\nSorting %li pairs, merging, please wait...\n", n);
fflush(stderr);
free(current);

qsort(arr, n, sizeof(PAIR), cmp);

sprintf(fname, "/usr/share/stardict/dic/myspell_%s_grammar.", lang);
p=strchr(fname, 0);

strcpy(p, "idx");
F2=fopen(fname, "wb");
if (!F2)
   {
   printf("\nCan't write to %s\n\n", fname);
   exit(1);
   }

strcpy(p, "dict");
F3=fopen(fname, "wb");
if (!F3)
   {
   printf("\nCan't write to %s\n\n", fname);
   exit(1);
   }


nn=off=*prev=*prev2=0;
for (i=0; i<n; i++)
   {
   same=0;
   

   current=arr[i].words;
   current2=arr[i].trans;
   if (*prev) if (strcmp(prev, current)==0)
      {
      if (strcmp(current2, prev2)==0) continue;
      same=1;
      nn--;
      }
   nn++;
   strcpy(prev, current);
   strcpy(prev2, current2);
   
   if (same) 
      {
      fwrite(" ", 1, 1, F3);
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

strcpy(p, "ifo");
F3=fopen(fname, "wt");

time(&t0);
t=gmtime(&t0);

fprintf(F3, "StarDict's dict ifo file\nversion=2.4.2\nwordcount=%li\nidxfilesize=%li\nbookname=", nn, ftell(F2));
fprintf(F3, "myspell %s grammar forms", lang);
fprintf(F3, "\ndate=%i.%02i.%02i\nsametypesequence=m\n",
t->tm_year+1900, t->tm_mon+1, t->tm_mday);

fclose(F);
fclose(F2);
fclose(F3);
printf("\nTotal %li entries written.\n", nn);
free(arr);
for (i=0; i<nr; i++)
   {
   free(rules[i].from);
   free(rules[i].to);
   free(rules[i].mor);
   for (j=0; j<rules[i].ml; j++) free(rules[i].mask[j]);
   free(rules[i].mask);
   }
free(rules);
fprintf(stderr, "\nRestart StarDict now!\n\n");

return 0;
}
