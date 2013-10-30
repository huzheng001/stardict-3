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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

long off, siz, n, i, cr, j;
FILE *F, *F2, *F3, *F4;
char *p, *p2, *p3, *ext;
char words[20000];
time_t t0;
struct tm *t;
char s[100];
char path[250];
int cyc, m;

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
   arr=(PAIR *)malloc(sizeof(PAIR)*40000);

   setbuf(stdout, 0);

if (argc>1)
   {
   printf("Just run ydp2dict in a folder containing dict*.dat files\n");
   exit(0);
   }

for (cyc=0; cyc<=1; cyc++)
   {
   strcpy(s, (cyc==0)? "English_Polish": "Polish_English");
   F=fopen((cyc==0)? "dict100.dat": "dict101.dat", "rb");
   if (!F) 
      {
      perror((cyc==0)? "Can't open dict100.dat": "Can't open dict101.dat");
      continue;
      }
   F2=fopen((cyc==0)? "dict100.idx": "dict101.idx", "rb");
   if (!F) 
      {
      perror((cyc==0)? "Can't open dict100.idx": "Can't open dict101.idx");
      continue;
      }
   
   sprintf(path, "/usr/share/stardict/dic/ydp_%s.idx", s);
   ext=strstr(path, ".idx")+1;
   F3=fopen(path, "wb");
   strcpy(ext, "dict");
   F4=fopen(path, "wb");
   
   fseek(F2, 16, SEEK_SET);
   size_t fread_size;
   fread_size = fread(&siz, 4, 1, F2);
   if (fread_size != 1) {
       printf("fread error!\n");
   }
   fseek(F2, siz, SEEK_SET);
   
   n=off=0;
   while (fread(&siz, 4, 1, F)==1)
      {
      fseek(F2, 8, SEEK_CUR);
      p=arr[n].words=malloc(100);
      
      while ((i=getc(F2))!=0) *(p++)=i;
      *p=0;
      p=arr[n].words;
      arr[n].words=g_convert(p, -1, "utf-8", "cp1250", 0, 0, 0);
      free(p);
      if (arr[n].words==NULL) 
         {
         perror(p);
         exit(1); 
         }
      
      fread_size = fread(words, siz, 1, F);
      if (fread_size != 1) {
          printf("fread error!\n");
}
      words[siz]=0;
      
      while ((p=strchr(words, '\\'))!=0)
         {
         for (p2=p+1; isalnum((unsigned char) *p2); p2++) ;
         if (*p2==' ') p2++;
         
         if (strncmp(p+1, "par", 3)==0 /*|| strncmp(p+1, "line", 4)==0*/) *(p++)='\n';
         
         else if (strncmp(p+1, "i ", 2)==0 && *p2!='}'/*|| strncmp(p+1, "b ", 2)==0*/)
            {
            *(p++)='_';
            
            m=0;
            for (p3=p2; *p3; p3++)
            if (*p3=='}')
               {
               if (m==0) 
                  {
                  *p3='_';
                  break;
                  }
               else m--;
               }
            else if (*p3=='{') m++;
            //if ((p3=strchr(p2, '}'))!=0) 
            }
         
         else if (strncmp(p+1, "b ", 2)==0 && *p2!='}' /*|| strncmp(p+1, "b ", 2)==0*/)
            {
            //for (p3=p2; *p3==' '; p3++);
            //p3--;
				
            *(p++)='\t';
            for (p3=p2; *p3; p3++)
            if (*p3=='}')
               {
               if (m==0) 
                  {
                  if (strncmp(p3, "}{\\b ", 5)==0)
                     {
                     memmove(p3, p3+5, strlen(p3+5)+1);
                     }
                  else 
                     {
                     *p3='\t';
                     break; 
                     }
                  }
               else m--;
               }
            else if (*p3=='{') m++;
            }
         //else if (strncmp(p+1, "cf1 ", 4)==0='}') *(p++)='\t';
         
         memmove(p, p2, strlen(p2)+1);
         
         }
      siz=0;
      
      p=arr[n].trans=malloc(strlen(words)*2);
      
      cr=0;
      for (p2=words; *p2; p2++)
         {
         if (*p2=='\n')
            {
            cr++;
            if (cr<3) *(p++)='\n';
            }
         else 
            {
            cr=0;
            if (*p2==0x7F) *(p++)='~';
            else if (*p2==(char)0x86) *(p++)='e';
            else if (*p2==(char)0x8E) *(p++)='\'';
            else if (*p2==(char)0x82) *(p++)='o';
            else if (*p2==(char)0x8D) *(p++)=':';
            else if (*p2==(char)0x8A) *(p++)='i';
            else if (*p2==(char)0x8b) *(p++)='a';
            else if (*p2==(char)0x97) 
               {
               *(p++)='<';
               *(p++)='d'; 
               *(p++)='h'; 
               *(p++)='>'; 
               }
            else if (*p2==(char)0x89) 
               {
               *(p++)='<';
               *(p++)='t'; 
               *(p++)='h'; 
               *(p++)='>'; 
               }
            else if (*p2==(char)0x88) 
               {
               *(p++)='<';
               //*(p++)='a'; 
               *(p++)='e'; 
               *(p++)='>'; 
               }
            else if (*p2==(char)0x85) 
               {
               *(p++)='<';
               *(p++)='s'; 
               *(p++)='h'; 
               *(p++)='>'; 
               }
            else if (*p2==(char)0x90) 
               {
               *(p++)='<';
               *(p++)='n'; 
               *(p++)='g'; 
               *(p++)='>'; 
               }
            else if (*p2==(char)0x98) 
               {
               *(p++)='<';
               *(p++)='a'; 
               *(p++)='e'; 
               *(p++)='>'; 
               }
            else if (*p2==(char)0x83) 
               {
               *(p++)='<';
               *(p++)='z'; 
               *(p++)='h'; 
               *(p++)='>'; 
               }
            else if (*p2==(char)0x87) 
               {
               *(p++)='<';
               *(p++)='a'; 
               *(p++)='>'; 
               }
            else if (*p2!='{' && *p2!='}') *(p++)=*p2;
            }
         }
      *p=0;
      for (p--; *p=='\n'; p--) *p=0;
      p=strchr(arr[n].trans, '\n');
      if (strstr(arr[n].trans, "--- KEYWORD ---")) memmove(arr[n].trans, p+1, strlen(p+1));
      p=arr[n].trans;
      arr[n].trans=g_convert(p, -1, "utf-8", "cp1250", 0, 0, 0);
      if (arr[n].trans==NULL) 
         {
         perror(p);
         exit(1); 
         }
      free(p);
      
      off+=strlen(arr[n++].trans);
      }
   
   qsort(arr, n, sizeof(PAIR), cmp);
   
   for (off=i=0; i<n; i++)
      {
      
      fwrite(arr[i].trans, strlen(arr[i].trans), 1, F4);
      
      fwrite(arr[i].words, strlen(arr[i].words)+1, 1, F3);
      for (j=3; j>=0; j--)
      fwrite(((char*)&off)+j, 1, 1, F3);
      
      
      siz=strlen(arr[i].trans);
      for (j=3; j>=0; j--)
      fwrite(((char*)&siz)+j, 1, 1, F3);
      off+=siz;
      //printf("%s>>%s\n", arr[i].words, arr[i].trans);
      
      free(arr[i].words);
      free(arr[i].trans);
      }
   siz=ftell(F3);
   fclose(F3);
   
   strcpy(ext, "ifo");
   F3=fopen(path, "wt");
   
   time(&t0);
   t=gmtime(&t0);
   
   fprintf(F3, "StarDict's dict ifo file\nversion=2.4.2\nwordcount=%li\nidxfilesize=%li\nbookname=", n, siz);
   fprintf(F3, "YDP %s dictionary", s);
   fprintf(F3, "\ndate=%i.%02i.%02i\nsametypesequence=m\n",
   t->tm_year+1900, t->tm_mon+1, t->tm_mday);
   
   fclose(F);
   fclose(F2);
   fclose(F3);
   
   printf("\nTotal %li entries written: /usr/share/stardict/dic/ydp_%s.*\n", n, s);
   }
printf("\nRestart StarDict now!\n\n");
return 0;
}
