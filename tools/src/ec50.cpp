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

//file format crack by Luo XiaoHui <tipyluo@hotmail.com>, this program is written by Hu Zheng <huzheng_001@163.com>
//you can mail us for the demo program which have more comment.
//encoding of this file is Chinese simplified -> GB18030

#include "stdio.h"
#include "stdlib.h"
#include <locale.h>
#include <string.h>
#include <glib.h>
#include <string>

//#define MAKEBIG5

#ifdef MAKEBIG5
GIConv gb2big5_converter;
GIConv utf8_converter;
#endif

char dat123[3][4];  //从0x0008e000处读入的数据
char *chGrp[26], *chnptr,*buffer, *word;
char chn123[3][4];
short int wordLen;
unsigned short int m1613, m1611;
long m2e15, m2e19;
unsigned char m15_19[8];
char	langMask;	// 英汉、汉英标记
FILE *f;

GArray *ec_array, *ce_array;
	
char decrypt[32]={
	0x4c,0x61,0x6e,0x67,0x44,0x61,0x6f,0x20,
	0x45,0x2d,0x43,0x20,0x44,0x69,0x63,0x74,
	0x69,0x6f,0x6e,0x61,0x72,0x79,0x20,0x56,
	0x35,0x2e,0x30,0x20,0x5a,0x48,0x48,0x44

};

struct _worditem
{
	gchar *word;
	gchar *definition;
};

static void incM1613();
static long readData(char *ptr, long len, long pos);
static void readBlock();

gint stardict_strcmp(const gchar *s1, const gchar *s2)
{
	gint a;
	a = g_ascii_strcasecmp(s1, s2);
	if (a == 0)
		return strcmp(s1, s2);
	else
		return a;
}

gint comparefunc(gconstpointer a,gconstpointer b)
{
	if (!(((struct _worditem *)a)->word))
		return -1;
	if (!(((struct _worditem *)b)->word))
		return 1;
	return stardict_strcmp(((struct _worditem *)a)->word,((struct _worditem *)b)->word);
}

gchar *to_utf8_phonetic(gchar *text)
{
	gchar *start = strstr(text, "*[");
	if (!start)
		return NULL;
	if (start!= text) {
		g_print("Warnning, middle phonetic!\n");
		return NULL;
	}
	gchar *end = strchr(start + 2, ']');
	if (!end) {
		printf("Warnning, bad phonetic!\n");
		return NULL;
	}	
	*end = '\0';
	
	std::string return_text = "*[";
	const gchar *s, *old_s;
	s = start+2;
	gchar tmpstr[16];
	gint i = 0;
	gint len = strlen(s);
	gint str_len;
	gboolean processed;	
	while (i < len) {
		processed = true;
		switch (*s) {			
		case 0x18:
			return_text+="忙"; break;
		case 0x10:
			return_text+="蓱"; break;
		case 0x0F:
			return_text+="蓲"; break;
		case 0x12:
			return_text+="蕦"; break;
		case 0x11:
			return_text+="訖"; break;
		case 0x0E:
			return_text+="褦"; break;
		case 0x17:
			return_text+="艐"; break;
		case 0x13:
			return_text+="胃"; break;
		case 0x14:
			return_text+="冒"; break;
		case 0x15:
			return_text+="蕛"; break;
		case 0x16:
			return_text+="蕭"; break;						
		/*case 0x:
			return_text+="藧"; break;						
		case 0x:
			return_text+="伞"; break;						
		case 0x:
			return_text+="藦"; break;						
		case 0x:
			return_text+="藡"; break;
		case 0x:
			return_text+="藠"; break;*/
		default:
			processed = false;
		}
		old_s = s;
		s = g_utf8_next_char(s);
		str_len = s - old_s;
		i+= str_len;
		if (!processed) {
			strncpy(tmpstr, old_s, str_len);
			tmpstr[str_len] = '\0';
			return_text += tmpstr;
		}
	}
	return_text+= "]";
	return_text+= (end+1);
	return g_strdup(return_text.c_str());
}

void stripNewLine(gchar *text)
{
	gchar *p = text;
	gchar *n = text;
	while (*p) {
		if (*p == 13 && *(p+1) == 10) {
			*n = 10;
			p+=2;
			n++;
		}
		else {
			if (p != n)
				*n = *p;
			p++;
			n++;
		}
	}
	*n = '\0';
}

int	compare(char *str1, const char *str2, int len)
{
	unsigned char c1=0, c2=0;

	for (int i=0; i<len; i++)
	{
		c1 = *(str1+i);
		c2 = *(str2+i);

		if (c1==0)
		{
			break;
		}

		if (c1<=0x7a && c1>=0x61)
		{
			c1 -= 0x20;
		}
		if (c2<=0x7a && c2>=0x61)
		{
			c2 -= 0x20;
		}
		if (c1!=c2)
		{
			break;
		}
	}

	return c1-c2;
}

void deal1611(unsigned long &cpy1611, int cx)
{
	unsigned short int dl, dh, tmp;

	dl = (cpy1611 & 0x0000ffff);
	dh = (cpy1611 & 0xffff0000)>>16;

	if (cx>=0x10)
	{
		dh = dl;
		dl = 0;
		dh <<= (cx-0x10);

	}
	else
	{
		tmp = dl;
		dl <<= cx;
		dh <<= cx;
		cx = 0x10 - cx;
		tmp >>= cx;
		dh = dh | tmp;
	}

	cpy1611 = dh;
	cpy1611 <<= 16;
	cpy1611 += dl;
}

void getWord(char *wptr)
{
	short int i;
	char *tmptr = word;

	wordLen = 0;
	while (*(wptr+m1613)!=0x00)
	{
		*tmptr = *(wptr+m1613);
		tmptr++;
		incM1613();
		wordLen++;
	}
	*tmptr = 0x00;
	incM1613();

	for (i=0; i<8; i++)
	{
		m15_19[i] = (unsigned char)*(wptr+m1613);
		incM1613();
	}
}

void getWord1(char *wptr)
{
	short int i;
	char *tmptr = word;

	wordLen = 0;
	while (*(wptr+m1613)!=0x00)
	{
		*tmptr = *(wptr+m1613);
		tmptr++;
		m1613++;
		wordLen++;
	}
	*tmptr = 0x00;
	m1613++;

	m2e15 = m2e19 =0;
	i=0;
	while (i<4)
	{
		m2e15 *= 0xc8;
		m2e15 += (unsigned char)(*(wptr+m1613))-0x20;
		m1613++;
		i++;
	}
	i=0;
	while (i<4)
	{
		m2e19 *= 0xc8;
		m2e19 += (unsigned char)(*(wptr+m1613))-0x20;
		m1613++;
		i++;
	}
}

bool doSearch(const gchar *searchWord)
{
	short int len, i, j;
	unsigned long offset;
	unsigned char frch;  //
	char m2e1d[6];
	char *cptr;	//cptr
	const char *wptr;	//wptr

	wptr = searchWord;
	frch = *wptr;

	if (frch>=0xb0)
	{//
		unsigned short int si=0;
		langMask = 'c';
		si = (unsigned char)frch;
		si -= 0xb0;
		si <<= 9;
		cptr = chnptr+si;
		i = (unsigned char)*(cptr+1);
		j = (unsigned char)*(cptr);
		m1611 = i*0x100;
		m1611 += j;
		m1613 = 0;
		cptr += 2;

		for (; cptr<=(chnptr+si+0x0200); cptr+=3)
		{
			if (compare(cptr, wptr+1, 3)>=0)
			{
				break;
			}
			m1611++;
			continue;
		}
		readBlock();
		while (m1613<0x800)
		{
			if (*(buffer+m1613)==0x01 || *(buffer+m1613)==0x1a)
			{
				break;
			}
			incM1613();
		}

		do
		{
			if (*(buffer+m1613)==0x1a)
			{
				return false;
			}
			incM1613();
			getWord(buffer);

		}while ((compare(word, wptr, wordLen+1))<0);
		
		/* convent m15_19[] to m2e15 and m2e19 */
		i=0;
		m2e15=m2e19=0;

		while (i<4)
		{
			m2e15 *= 0xc8;
			m2e15 += m15_19[i]-0x20;
			i++;
		}
		while (i<8)
		{
			m2e19 *= 0xc8;
			m2e19 += m15_19[i]-0x20;
			i++;
		}

		*(word+wordLen) = 0x0d;
		*(word+wordLen+1) = 0x0a;

		offset = m2e19;
		offset += (unsigned char)(*chn123[1]);
		offset += (unsigned char)(*(chn123[1]+1)) * 0x100;
		offset += (unsigned char)(*(chn123[1]+2)) * 0x10000;
		offset += (unsigned char)(*(chn123[1]+3)) * 0x1000000;
		fseek(f, offset, SEEK_SET);
		if ((len=readData(m2e1d, 2, offset))<=0)
		{
			return false;
		}
		i = 0x0c00;
		i -= wordLen;
		i -= 4;
		j = (unsigned char)(*(m2e1d));
		j += (unsigned char)(*(m2e1d+1)) * 0x100;

		if (i>j)
		{
			i = j;
		}

		if ((len=readData(word+wordLen+2, i, offset+2))<=0)
		{
			return false;
		}
	}
	else
	{//小于0xb0,
		langMask = 'e';
		if (frch<=0x5a && frch>=0x41)
		{
			frch -= 0x41;
		}
		else if (frch<=0x7a && frch>=0x61)
		{
			frch -= 0x61;
		}
		else
		{
			return false;
		}


		cptr = chGrp[frch];
		i = (unsigned char)*(cptr+1);
		j = (unsigned char)(*cptr);
		m1611 = i*0x100;
		m1611 += j;
		m1613 = 0;

		cptr += 2;
		for (; cptr<(chGrp[frch]+0x0c00); cptr+=5)
		{
			if (compare(cptr, wptr+1, 5)>=0)
			{
				break;
			}
			m1611++;
			continue;
		}

		readBlock();
		
		while (m1613<0x800)
		{
			if (*(buffer+m1613)==0x01 || *(buffer+m1613)==0x1a)
			{

				break;
			}
			incM1613();			
		}

		do
		{
			if (*(buffer+m1613)==0x1a)
			{
				return false;
			}
			incM1613();
			getWord(buffer);

		}while ((compare(word, wptr, wordLen+1))<0);

		/* convent m15_19[] to m2e15 and m2e19 */
		i=0;
		m2e15=m2e19=0;

		while (i<4)
		{
			m2e15 *= 0xc8;
			m2e15 += m15_19[i]-0x20;
			i++;
		}
		while (i<8)
		{
			m2e19 *= 0xc8;
			m2e19 += m15_19[i]-0x20;
			i++;
		}

		*(word+wordLen) = 0x0d;
		*(word+wordLen+1) = 0x0a;

		offset = m2e19;
		offset += (unsigned char)(*dat123[1]);
		offset += (unsigned char)(*(dat123[1]+1)) * 0x100;
		offset += (unsigned char)(*(dat123[1]+2)) * 0x10000;
		offset += (unsigned char)(*(dat123[1]+3)) * 0x1000000;

		fseek(f, offset, SEEK_SET);

		if ((len=readData(m2e1d, 4, offset))<=0)
		{
			return false;
		}

		if ((len=readData(m2e1d+4, 2, offset+4))<=0)
		{
			return false;
		}

		i = 0x0c00;
		i -= wordLen;
		i -= 4;
		j = (unsigned char)(*(m2e1d+4));
		j += (unsigned char)(*(m2e1d+5)) * 0x100;

		if (i>j)
		{
			i = j;
		}

		if ((len=readData(word+wordLen+2, i, offset+6))<=0)
		{
			return false;
		}
	}


	*(word+wordLen+2+len)=0;

	return true;
}

void readBlock()
{
	char *ptr;	
	unsigned long offset;

	if (m1611>=0x8000)
	{
		offset = 0xffff0000 + m1611;
	}
	else
	{
		offset = 0x00000000 + m1611;
	}

	deal1611(offset, 0x0b);  

	if (langMask=='e')
	{
		ptr = dat123[0];
	}
	else if (langMask=='c')
	{
		ptr = chn123[0];
	}
	else
	{
		return;
	}

	offset += (unsigned char)(*ptr);
	offset += (unsigned char)((*(ptr+1)))<<8;
	offset += (unsigned char)((*(ptr+2)))<<16;
	offset += (unsigned char)((*(ptr+3)))<<24;

	fseek(f, offset, SEEK_SET);

	if ((readData(buffer, 0x800, offset))<=0)
	{
		return;
	}
}

void incM1613()
{
	m1613++;
	if (m1613==0x800)
	{
		m1611++;
		readBlock();
		m1613=0;
	}
}

long readData(char *ptr, long len, long pos)
{
	unsigned short int i1, i2, dec;
	long count;
	char c;

	i1 = pos & 0x000003ff;  //获取 pos 的最后10位
	i2 = i1 + (pos>>10);			//去掉 pos 的最后10位，再与 i1 相加

	size_t fread_size;
	fread_size = fread(ptr, 1, len, f);
	if (fread_size != (size_t)len) {
		g_print("fread error!\n");
	}

	//以下部分为解密过程
	count = 0;
	dec = i1;
	while(count<len)
	{
		dec = i1;
		dec = dec & 0x01f;

		c = decrypt[dec];
		c = c ^ i2;
		*ptr = *ptr ^ c;
		ptr++;

		i2++;
		i1++;
		if (i1==0x400)
		{
			i1 = 0;	
			i2++;
		}
		count++;
	}

	return len;
}

void captureAllChn(int chnum)
{
	struct _worditem worditem;
#ifdef MAKEBIG5
	gchar *big5word;
#endif
	
	short int len, i, j;
	unsigned long offset;
	char *cptr, *ptr;	//cptr
	bool end = false;
	const char const_ff[5]={(char)(0x0ff),(char)(0x0ff),(char)(0x0ff),(char)(0x0ff),(char)(0x0ff)};

	langMask = 'c';
	unsigned short int si=0;
	si = chnum;
	si <<= 9;
	cptr = chnptr+si;

	i = (unsigned char)*(cptr+1);
	j = (unsigned char)(*cptr);
	m1611 = i*0x100;
	m1611 += j;

	cptr += 2;
	m1611--;
	for (; (cptr<(chnptr+si+0x0200) && !end); cptr+=3)
	{
		m1613 = 0;
		if (compare(cptr, const_ff, 3)>=0)
		{
			end = true;
		}
		m1611++;

		if (m1611>=0x8000)
		{
			offset = 0xffff0000 + m1611;
		}
		else
		{
			offset = 0x00000000 + m1611;
		}

		deal1611(offset, 0x0b);  //

		ptr = chn123[0];
		offset += (unsigned char)(*ptr);
		offset += (unsigned char)((*(ptr+1)))<<8;
		offset += (unsigned char)((*(ptr+2)))<<16;
		offset += (unsigned char)((*(ptr+3)))<<24;

		fseek(f, offset, SEEK_SET);

		if ((len=readData(buffer, 0x800, offset))<=0)
		{
			g_print("error in %d! readData\n", chnum);
			return;
		}

		while (m1613<0x800)
		{
			if (*(buffer+m1613)==0x01 || *(buffer+m1613)==0x1a)
			{
				break;
			}
			m1613++;
		}

		do
		{
			if (*(buffer+m1613)==0x1a)
			{
				g_print("error in %d! 0x1a\n", chnum);
				return;
			}
			m1613++;
			getWord1(buffer);

			word[wordLen] = '\0';
			
			if (word[0]) {
#ifdef MAKEBIG5
				big5word = g_convert_with_iconv(word, wordLen,gb2big5_converter,NULL,NULL,NULL);
				if (big5word) {
					worditem.word = g_convert_with_iconv(big5word, -1, utf8_converter,NULL,NULL,NULL);
					g_free(big5word);
				}
				else {
					worditem.word = NULL;
					//printf("convert to big5 failed1: %s\n", word);
				}
#else
				worditem.word = g_locale_to_utf8(word, wordLen,NULL, NULL, NULL);
#endif
				if (worditem.word) {
					g_free(worditem.word);
					worditem.word = g_strdup(word);
					g_array_append_val(ce_array, worditem); //success
				}
				else {
					// here have about 1700 words that are invalid :(, this is the only space that are not perfect!!!
					//printf("bad chinese word: %s\n", word);					
				}
			}
			else {
				//printf("empty chinese word\n");
			}
			
		}while (m1613<0x800);
	}	
}

void searchAllChn()
{
	gint tmpwordlen;
	struct _worditem *pworditem;
	gchar *tmpword, *tmp_utf8, *p;
#ifdef MAKEBIG5
	gchar *big5word;
#endif
	
	g_print("Searching...\n");
	for (guint i = 0; i < ce_array->len; i++) {
		pworditem = &g_array_index(ce_array, struct _worditem, i);
		if (doSearch(pworditem->word) && word[0]) {
			stripNewLine(word);
			tmpwordlen = strlen(pworditem->word);
			if (strncmp(word, pworditem->word, tmpwordlen) == 0 && word[tmpwordlen] == 10) {				
			}
			else {
				printf("fix wrong head: %s", pworditem->word);
				g_free(pworditem->word);
				p = strchr(word, '\n');
				*p='\0';
				tmpwordlen = strlen(word);				
				pworditem->word = g_strdup(word);
				g_strstrip(pworditem->word);
				printf(" | %s\n", pworditem->word);				
			}
			g_strstrip(word + tmpwordlen +1);
#ifdef MAKEBIG5
			big5word = g_convert_with_iconv(word + tmpwordlen+1, -1,gb2big5_converter,NULL,NULL,NULL);
			if (big5word) {
				tmp_utf8 = g_convert_with_iconv(big5word, -1, utf8_converter,NULL,NULL,NULL);
				g_free(big5word);
			}
			else {
				tmp_utf8 = NULL;
				//printf("convert to big5 failed: %s\n", pworditem->word);
			}				
#else
			tmp_utf8 = g_locale_to_utf8(word + tmpwordlen +1, -1,NULL, NULL, NULL);
#endif
			if (tmp_utf8) {
				pworditem->definition = tmp_utf8;
				tmpword = pworditem->word;
#ifdef MAKEBIG5
				big5word = g_convert_with_iconv(tmpword, -1,gb2big5_converter,NULL,NULL,NULL);
				pworditem->word = g_convert_with_iconv(big5word, -1, utf8_converter,NULL,NULL,NULL);
				g_free(big5word);
#else
				pworditem->word = g_locale_to_utf8(tmpword, -1,NULL, NULL, NULL);;
#endif
				g_free(tmpword);
			}
			else {
				//printf("chinese word's definition invalid: %s\n", pworditem->word);
				g_free(pworditem->word);
				pworditem->word= NULL;
			}
		}
		else {
			printf("chinese word's definition not found: %s\n", pworditem->word);
			g_free(pworditem->word);
			pworditem->word= NULL;
		}
		if (i %10000 == 0)
			printf(".");
	}
}

void captureAllWord(int chnum)
{	
	struct _worditem worditem;
	
	short int len, i, j;
	unsigned long offset;
	char *cptr, *ptr;
	bool end = false;
	const char const_ff[5]={(char)(0x0ff),(char)(0x0ff),(char)(0x0ff),(char)(0x0ff),(char)(0x0ff)};

	langMask = 'e';
	
	cptr = chGrp[chnum];
	i = (unsigned char)*(cptr+1);
	j = (unsigned char)(*cptr);
	m1611 = i*0x100;
	m1611 += j;
	

	cptr += 2;
	m1611--;
	for (; (cptr<(chGrp[chnum]+0x0c00) && !end); cptr+=5)
	{
		m1613 = 0;
		if (compare(cptr, const_ff, 5)>=0)
		{
			end = true;
		}
		m1611++;
	
		if (m1611>=0x8000)
		{
			offset = 0xffff0000 + m1611;
		}
		else
		{
			offset = 0x00000000 + m1611;
		}

		deal1611(offset, 0x0b);  //cable -> 0x0017e800

		ptr = dat123[0];
		offset += (unsigned char)(*ptr);
		offset += (unsigned char)((*(ptr+1)))<<8;
		offset += (unsigned char)((*(ptr+2)))<<16;
		offset += (unsigned char)((*(ptr+3)))<<24;

		fseek(f, offset, SEEK_SET);

		if ((len=readData(buffer, 0x800, offset))<=0)
		{
			g_print("error in %d! readData\n", chnum);
			return;
		}
		
		while (m1613<0x800)
		{
			if (*(buffer+m1613)==0x01 || *(buffer+m1613)==0x1a)
			{
				break;
			}
			m1613++;
		}

		do
		{
			if (*(buffer+m1613)==0x1a)
			{
				g_print("error in %d! 0x1a\n", chnum);
				return;
			}
			m1613++;
			getWord1(buffer);	
			
			word[wordLen] = '\0';
			g_strstrip(word);
			if (word[0]) {
				if (g_utf8_validate(word, -1,NULL)) {
					worditem.word = g_strdup(word);
					g_array_append_val(ec_array, worditem); //success
				}
				else {
					printf("bad english word: %s\n", word);
				}			
			}
			else {
				//printf("empty english word!\n");
			}
		}while (m1613<0x800);
	}
}

void searchAllWord()
{
	g_print("Searching...\n");
#ifdef MAKEBIG5
	gchar *big5word;
#endif
	gint tmpwordlen;
	struct _worditem *pworditem;
	gchar *tmp_utf8;
	gchar *p;
	for (guint i = 0; i < ec_array->len; i++) {
		pworditem = &g_array_index(ec_array, struct _worditem, i);
		if (doSearch(pworditem->word) && word[0]) {
			stripNewLine(word);
			tmpwordlen = strlen(pworditem->word);
			if (strncmp(word, pworditem->word, tmpwordlen) == 0 && word[tmpwordlen] == 10) {
			}
			else {
				//printf("fix wrong head: %s", pworditem->word);
				g_free(pworditem->word);
				p = strchr(word, '\n');
				*p='\0';
				tmpwordlen = strlen(word);				
				pworditem->word = g_strdup(word);
				g_strstrip(pworditem->word);
				//printf(" | %s\n", pworditem->word);				
			}
			g_strstrip(word + tmpwordlen +1);
#ifdef MAKEBIG5
			big5word = g_convert_with_iconv(word + tmpwordlen+1, -1,gb2big5_converter,NULL,NULL,NULL);
			if (big5word) {
				tmp_utf8 = g_convert_with_iconv(big5word, -1, utf8_converter,NULL,NULL,NULL);
				g_free(big5word);
			}
			else {
				tmp_utf8 = NULL;
				//printf("convert to big5 failed: %s\n", pworditem->word);
			}				
#else
			tmp_utf8 = g_locale_to_utf8(word + tmpwordlen+1, -1,NULL, NULL, NULL);
#endif			
			if (tmp_utf8) {
				pworditem->definition = to_utf8_phonetic(tmp_utf8);
				if (pworditem->definition) {
					g_free(tmp_utf8);
				}
				else
					pworditem->definition = tmp_utf8;
			}
			else {
				//g_print("english word's definition invalid: %s\n", pworditem->word);
				g_free(pworditem->word);
				pworditem->word= NULL;
			}
		}
		else {
			g_print("english word's definition not found: %s\n", pworditem->word);
			g_free(pworditem->word);
			pworditem->word= NULL;
		}
	}
}

void initVari()
{
	buffer = new char[0x800];
	word = new char[0x0c00];
}

void closeVari()
{
	for(int i=0; i<26; i++)
	{
		delete []chGrp[i]; 
	}
	delete []buffer;
	delete []word;
}

void initFile()
{
	unsigned long len;
	unsigned short int i;
	
	f = fopen("ec50.dat", "r");

	fseek(f, 0x0008e000, SEEK_SET);
	len = 4;
	i = 0;
	while (i<3 && len==4)
	{
		len = readData(dat123[i], 4, 0x0008e000+4*i);
		i++;
	}
	
	if (len<4)
	{
		return;
	}

	fseek(f, 0x000c0000, SEEK_SET);

	len = 4;
	i = 0;
	while (i<3 &&len==4)
	{
		len = readData(chn123[i], 4, 0x000c0000+4*i);
		i++;
	}
	if (len<4)
	{
		return;
	}

	fseek(f, 0x00105800, SEEK_SET);
	chnptr = new char[0x9000];
	len = readData(chnptr, 0x9000, 0x00105800);
	
	//定位到 ec50.dat 的 0x000f2000，并从中读取26个大小为0x0c00大小的区域
	fseek(f, 0x000f2000, SEEK_SET);
	len = 0x0c00;
	i = 0;
	while (i<26 && len==0x0c00)
	{		
		chGrp[i] = new char[0x0c00];
		len = readData(chGrp[i], 0x0c00, 0x000f2000+0x0c00*i); 
		i++;
	}
	if (len<0x0c00)
	{
		return;
	}
}

void closeFile()
{
	fclose(f);
}

void save_array_to_file(GArray *array, const gchar *basefilename)
{
	g_array_sort(array,comparefunc);
		
	gchar idxfilename[256];
	gchar dicfilename[256];
	sprintf(idxfilename, "%s.idxdata", basefilename);
	sprintf(dicfilename, "%s.dict", basefilename);
	FILE *idxfile = fopen(idxfilename,"w");
	FILE *dicfile = fopen(dicfilename,"w");

	glong tmpglong = 0;
	fwrite(&(tmpglong),sizeof(glong),1,idxfile);		
	
	guint wordcount = array->len;

	long offset_old;
	const gchar *previous_word = "";
	struct _worditem *pworditem;	
	gint definition_len;
	gulong i;
	for (i= 0;i < array->len; i++) {
		pworditem = &g_array_index(array, struct _worditem, i);
		if (pworditem->word) {			
			if (strcmp(previous_word,pworditem->word)==0) {
				//printf("Double! %s\n",previous_word);
				wordcount--;
			}
			else {
				previous_word = pworditem->word;
				offset_old = ftell(dicfile);
				definition_len = strlen(pworditem->definition);
				fwrite(pworditem->definition, 1 ,definition_len,dicfile);
						
				fwrite(pworditem->word,sizeof(gchar),strlen(pworditem->word)+1,idxfile);
				tmpglong = g_htonl(offset_old);
				fwrite(&(tmpglong),sizeof(glong),1,idxfile);
				tmpglong = g_htonl(definition_len);
				fwrite(&(tmpglong),sizeof(glong),1,idxfile);	
			}
		}
		else {			
			wordcount--;
		}
	}	
	for (i= 0;i < array->len; i++) {
		pworditem = &g_array_index(array, struct _worditem, i);
		if (pworditem->word) {
			g_free(pworditem->word);
			g_free(pworditem->definition);
		}
	}	
		
	fseek(idxfile, 0,SEEK_SET);
	tmpglong = g_htonl(wordcount);
	fwrite(&(tmpglong),sizeof(glong),1,idxfile);
	
	g_print("%s wordcount: %u\n", basefilename, wordcount);

	g_free(buffer);
    g_array_free(array,TRUE);
	
	fclose(idxfile);
	fclose(dicfile);
}

void convert()
{
	initVari();
	initFile();

#ifdef MAKEBIG5
gb2big5_converter = g_iconv_open("BIG5","GB2312");
utf8_converter = g_iconv_open("UTF-8","BIG5");
#endif
	
	ec_array = g_array_sized_new(FALSE,FALSE, sizeof(struct _worditem),500000);			
	for (int i=0; i<26; i++) {
		g_print("captureAllWord%d...\n", i);
		captureAllWord(i);
		g_print("%u\n", ec_array->len);
	}
	searchAllWord();
#ifdef MAKEBIG5
	save_array_to_file(ec_array, "langdao-ec-big5");
#else
	save_array_to_file(ec_array, "langdao-ec-gb");
#endif
	
	
	
	/*ce_array = g_array_sized_new(FALSE,FALSE, sizeof(struct _worditem),450000);			
	for (int i=0; i<72; i++) {
		g_print("captureAllChn%d...\n", i);
		captureAllChn(i);
		g_print("%u\n", ce_array->len);
	}
	searchAllChn();
#ifdef MAKEBIG5
	save_array_to_file(ce_array, "langdao-ce-big5");
#else
	save_array_to_file(ce_array, "langdao-ce-gb");
#endif*/

#ifdef MAKEBIG5
	g_iconv_close(gb2big5_converter);
	g_iconv_close(utf8_converter);
#endif
	
	closeVari();
	closeFile();
};

int
main(int argc,char * argv [])
{
	setlocale(LC_ALL, "");
	
	convert();	
	
	return FALSE;	
}
