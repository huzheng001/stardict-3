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

long off, siz;
FILE *F, *F2, *F3;
long c, i, j, n, lastcons, accent, preaccent, colon, laster, same, ar, ay, nn;
char *spell, *p;
char current[200];
char current2[200];
char prev[200];
char *sp[50];

char *LastChar(char *p)
{
return strchr(p, '\0')-1;
}

void Ar(void)
{
if (ar) strcpy(sp[nn++], "r");
}

char IsConsonant(char *p)
{
return (strchr("bcdfghjklmnpqrstvwxz", *p) ||
(*p=='\xC5' && p[1]=='\x8B') ||
(*p=='\xCA' && p[1]=='\x83') ||
(*p=='\xC3' && p[1]=='\xB0') ||
(*p=='\xCE' && p[1]=='\xB8') ||
(*p=='\xCA' && p[1]=='\x92'));
}

int main (int argc, char *argv[])
{
time_t t0;
struct tm *t;

F=fopen((argc>1)? argv[1]: "/usr/share/festival/dicts/cmu/cmudict-0.4.out", "rt");
F2=fopen("cmudict.idx", "wb");
F3=fopen("cmudict.dict", "wb");

	char *ch;
	ch = fgets(current2, 200, F);
	if (ch == NULL) {
		printf("fgets error!\n");
	}

nn=n=off=siz=0;

*prev=0;
for (i=0; i<50; i++) sp[i]=(char *)malloc(10);

while (fgets(current, 200, F))
   {
   i=0;
   
   spell=strchr(current+2, '(');
   p=strchr(current+2, '\"');
   *p=*current2=0;
   *strchr(p+2, ' ')=0;
   same=0;
   if (*prev) if (strcmp(prev, current+2)==0) 
      {
      same=1;
      n--; 
      }
   n++;
   strcpy(prev, current+2);
   if (strcmp(p+2, "nil")!=0)
      {
      switch (p[2])
         {
         case 'd': sprintf(current2, "(article) "); break;
         case 'n': sprintf(current2, "(noun) "); break;
         case 'v': sprintf(current2, "(verb) "); break;
         case 'j': sprintf(current2, "(adjective) "); break;
         default: sprintf(current2, "(%s) ", p+2); break;
         }
      
      }
   strcat(current2, "[");
   nn=ar=ay=lastcons=0;
   laster=preaccent=accent=-1;
   while ((spell=(char *)strtok(spell, " ()\n\r"))!=NULL)
      {
      if (*spell=='0') 
         {
         i++;
         for (j=nn-1; j>lastcons; j--)
            {
            p=LastChar(sp[j]);
            if (*p==':') 
               {
               
               if (*(p-1)=='i') 
                  {
                  *(p-1)='\xC4';
                  *(p)='\xB1';
                  }
               else if (laster!=j) *p=0;
               }
            }
         lastcons=nn-1;
         if (!IsConsonant(sp[lastcons])) lastcons++;
         spell=NULL;
         continue;
         }
      else if (*spell=='1') 
         {
         i++;
         if (accent!=-1) preaccent=accent;
         accent=lastcons;
         if (accent!=0 && IsConsonant(sp[accent]) && IsConsonant(sp[accent+1])) accent++;
         
         if (laster>=lastcons) 
            {
            p=LastChar(sp[laster]);
				if (*p!=':') 
               {
               memmove(p+2, p+1, strlen(p+1)+1);
               p[1]=':';
               }
            }
         lastcons=nn-1;
         if (!IsConsonant(sp[lastcons])) lastcons++;
         spell=NULL;
         continue;
         }
      
      if (strcmp(spell, "er")==0) 
         {
         Ar(); 
         laster=nn;
         strcpy(sp[nn++], "\xC9\x99");
         ar=2;
         }
      else if (strcmp(spell, "eh")==0) 
         {
         Ar(); strcpy(sp[nn++], "e"); 
         }
      else if (strcmp(spell, "ah")==0) 
         {
         Ar(); strcpy(sp[nn++], "\xCA\x8C"); 
         }
      else if (strcmp(spell, "y")==0) strcpy(sp[nn++], "j");
      else if (strcmp(spell, "ng")==0) strcpy(sp[nn++], "\xC5\x8B");
      else if (strcmp(spell, "sh")==0) strcpy(sp[nn++], "\xCA\x83");
      else if (strcmp(spell, "zh")==0) strcpy(sp[nn++], "\xCA\x92");
      else if (strcmp(spell, "ch")==0) strcpy(sp[nn++], "t\xCA\x83");
      else if (strcmp(spell, "dh")==0) strcpy(sp[nn++], "\xC3\xB0");
      else if (strcmp(spell, "th")==0) strcpy(sp[nn++], "\xCE\xB8");
      else if (strcmp(spell, "ay")==0) 
         {
         Ar(); strcpy(sp[nn++], "a\xC4\xB1");
         ay=2; 
         }
      else if (strcmp(spell, "hh")==0) 
         {
         strcpy(sp[nn++], "h"); 
         }
      else if (strcmp(spell, "ey")==0) 
         {
         Ar(); strcpy(sp[nn++], "e\xC4\xB1"); 
         }
      else if (strcmp(spell, "oy")==0) 
         {
         Ar(); strcpy(sp[nn++], "\xC9\x94\xC4\xB1"); 
         }
      else if (strcmp(spell, "iy")==0) 
         {
         Ar(); strcpy(sp[nn++], "i:"); 
         }
      else if (strcmp(spell, "ae")==0) 
         {
         Ar(); strcpy(sp[nn++], "\xC3\xA6"); 
         }
      else if (strcmp(spell, "ao")==0)
         {
         Ar(); strcpy(sp[nn++], "\xC9\x94"); 
         }
      else if (strcmp(spell, "aa")==0)
         {
         Ar(); strcpy(sp[nn++], "\xC9\x91"); 
         }
      else if (strcmp(spell, "ax")==0) 
         {
         Ar(); strcpy(sp[nn++], "\xC9\x99"); 
         }
      else if (strcmp(spell, "aw")==0) 
         {
         Ar(); strcpy(sp[nn++], "au"); 
         }
      else if (strcmp(spell, "ow")==0) 
         {
         Ar(); strcpy(sp[nn++], "\xC9\x99u"); 
         }
      else if (strcmp(spell, "uw")==0 || strcmp(spell, "uh")==0) 
         {
         Ar(); strcpy(sp[nn++], "u:"); 
         }
      else if (strcmp(spell, "ih")==0) 
         {
         Ar(); strcpy(sp[nn++], "\xC4\xB1"); 
         }
      else if (strcmp(spell, "r")==0)
         {
         ar=2;
         if (nn>0)
            {
            if (strcmp(sp[nn-1], "u:")==0)
               {
               strcpy(sp[nn-1], "u\xC9\x99");
               }
            else if (ay || strcmp(sp[nn-1], "\xC4\xB1")==0)
               {
               strcat(sp[nn-1], "\xC9\x99");
               }
            else if (strcmp(sp[nn-1], "\xC9\x94")==0 || strcmp(sp[nn-1], "\xC9\x91")==0)
            strcat(sp[nn-1], ":");
            }
         }
      else if (strcmp(spell, "jh")==0) strcpy(sp[nn++], "d\xCA\x92"); 
      else strcpy(sp[nn++], spell);
      
      if (ar==1 && nn>=2 && strcmp(sp[nn-2], "e")==0 && IsConsonant(sp[nn-1])) strcpy(sp[nn-2], "\xC9\x9B\xC9\x99");
      if (ar) ar--;
      if (ay) ay--;
      if (nn>=2) if (IsConsonant(sp[nn-1]) && strcmp(sp[nn-2], "\xC9\x91")==0) strcpy(sp[nn-2], "\xC9\x94");
      
      spell=NULL;
      }
   
   if (ar) if (strcmp(sp[nn-1], "e")==0) strcpy(sp[nn-1], "\xC9\x9B\xC9\x99");
   
   for (j=0; j<nn; j++) 
      {
      if (i!=1) if (j==accent || j==preaccent) strcat(current2, "\'");
      strcat(current2, sp[j]);
      }
   strcat(current2, "]");
   
   printf("%s\t%s\n", current+2, current2);
   
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
      fwrite(current+2, strlen(current+2)+1, 1, F2);
      for (j=3; j>=0; j--)
      fwrite(((char*)&off)+j, 1, 1, F2);
      }
   else fseek(F2, -4L, SEEK_END); 
   
   for (j=3; j>=0; j--)	
   fwrite(((char*)&siz)+j, 1, 1, F2);
   off+=strlen(current2);
   }
fclose(F3);
F3=fopen("cmudict.ifo", "wt");

time(&t0);
t=gmtime(&t0);

fprintf(F3, "StarDict's dict ifo file\nversion=2.4.2\nwordcount=%li\nidxfilesize=%li\n", n, ftell(F2));
fprintf(F3, "bookname=CMU American English spelling\ndate=%i.%02i.%02i\nsametypesequence=m\n",
t->tm_year+1900, t->tm_mon+1, t->tm_mday);

for (i=0; i<50; i++) free (sp[i]);

fclose(F);
fclose(F2);
fclose(F3);
return 0;
}
