/*
 * Copyright 2011 kubtek <kubtek@mail.com>
 *
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

#include "kmp.h"

static int* prefix = NULL;
static int max_size = 0;

static void GetPrefixValue(const char* strPattern, int iPatternLen)
{
    if (iPatternLen>max_size) {
        prefix = (int*)realloc(prefix, iPatternLen*sizeof(int));
        max_size = iPatternLen;
    }
   
    int i, j; /* i runs through the string, j counts the hits*/
    i = 1; j = 0;
    prefix[0] = 0;
   
    while(i<iPatternLen)
    {
        if(strPattern[i] == strPattern[j])
        {
            prefix[i] = ++j;
        }
        else
        {
            j = 0;
            prefix[i] = j;
        }
       
        i++;
    }
}

static int KMPStringMatch(const char* strPattern, int iPatternLen, const char* strTarget, int iTargetLen)
{
	int j =0;
	int i;
	for (i=0;i<iPatternLen;i++) {
		while ((strPattern[i] != strTarget[j]) && (j > 0))
			j = prefix[j-1];
		if (strPattern[i] == strTarget[j])
			j++;
		if (j == iTargetLen)
			return i - j + 1;
	}
	return -1;
}

int KMP(const char* strPattern, int len, const char* strTarget)
{
	GetPrefixValue(strPattern, len);
	return KMPStringMatch(strPattern, len, strTarget, strlen(strTarget));
}

void KMP_end()
{
	free(prefix);
	prefix=NULL;
	max_size=0;
}
