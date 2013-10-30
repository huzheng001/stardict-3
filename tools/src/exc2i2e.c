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
#include <string.h>

void Under2Space(char *p)
{
for (p=strchr(p, '_'); p; p=strchr(p, '_')) *(p++)=' ';
}


int main (int argc, char *argv[])
{
int i;
char *p;
char word[200], word2[200];

FILE *file;

for (i=0; i<4; i++)
   {
   switch (i)
      {
      case 0: file = freopen("/usr/share/wordnet/noun.exc", "rt", stdin); break;
      case 1: file = freopen("/usr/share/wordnet/verb.exc", "rt", stdin); break;
      case 2: file = freopen("/usr/share/wordnet/adj.exc", "rt", stdin); break;
      case 3: file = freopen("/usr/share/wordnet/adv.exc", "rt", stdin); break;
      }
   if (file == NULL) {
       printf("freopen error!\n");
   }
   while (fgets(word, 200, stdin))
      {
      p=strchr(word, '\n');
      if (p) *p=0;
      p=strchr(word, ' ');
      *(p++)=0;
      Under2Space(word);
      for (p=strtok(p, " "); p; p=strtok(NULL, " "))
         {
         switch(i)
            {
            case 0:
            sprintf(word2, "%ses", p);
            if (strcmp(word, word2)==0) continue;
            break;
            
            case 1:
            if (p[strlen(p)-1]=='y')
               {
               strcpy(word2, p);
               strcpy(word2+strlen(word2)-1, "ied");
               if (strcmp(word, word2)==0) continue;
               }
            if (strchr("bcdfghjklmnpqrstvz", p[strlen(p)-1]))
               {
               sprintf(word2, "%s%ced", p, p[strlen(p)-1]);
               if (strcmp(word, word2)==0) continue;
               sprintf(word2, "%s%cing", p, p[strlen(p)-1]);
               if (strcmp(word, word2)==0) continue;
               }
            if (p[strlen(p)-1]=='e')
               {
               sprintf(word2, "%sd", p);
               if (strcmp(word, word2)==0) continue;
               }
            sprintf(word2, "%sed", p);
            if (strcmp(word, word2)==0) continue;
            sprintf(word2, "%sing", p);
            if (strcmp(word, word2)==0) continue;
            break;
            
            case 2:
            case 3:
            if (p[strlen(p)-1]=='y')
               {
               strcpy(word2, p);
               strcpy(word2+strlen(p)-1, "ier");
               //printf ("word=%s! word2=%s!\t", word, word2);
               if (strcmp(word, word2)==0) continue;
               strcpy(word2+strlen(p)-1, "iest");
               if (strcmp(word, word2)==0) continue;
               }
            if (strchr("bcdfghjklmnpqrstvz", p[strlen(p)-1]))
               {
               sprintf(word2, "%s%cer", p, p[strlen(p)-1]);
               if (strcmp(word, word2)==0) continue;
               sprintf(word2, "%s%cest", p, p[strlen(p)-1]);
               if (strcmp(word, word2)==0) continue;
               }
            if (p[strlen(p)-1]=='e')
               {
               sprintf(word2, "%sr", p);
               if (strcmp(word, word2)==0) continue;
               sprintf(word2, "%sst", p);
               if (strcmp(word, word2)==0) continue;
               }
            sprintf(word2, "%ser", p);
            if (strcmp(word, word2)==0) continue;
            sprintf(word2, "%sest", p);
            if (strcmp(word, word2)==0) continue;
            break;
            }
         
         Under2Space(p);
         if (strcmp(word, p)==0) continue;
			printf((i)? "%s : => {%s}\n": "%s : pl. of {%s}\n", word, p);
			switch(i)
				{
				case 0: printf("%s : (pl) %s\n", p, word); break;
				case 1: printf("%s : (%s)\n", p, word); break;
				case 2: printf("%s : %s (>)\n", p, word); break;
				case 3: printf("%s : (adv.) %s (>)\n", p, word); break;
				}
         }
      }
   }
return 0;
}
