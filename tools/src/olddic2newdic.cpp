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

//convert old stardict(1.3)'s dic file to new stardict(2.1.0) data file.
//the mydict.idxhead file should be edited by your self.

#define DISABLE_CONVERT_LOCALE
#define DISABLE_CONVERT_ENDIAN

#include <string.h>
#include <stdio.h>
#include <locale.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

#include <gtk/gtk.h>
#include <glib.h>

#define LIB_WORD_ENGLISH 0
#define LIB_WORD_GB 1
#define LIB_WORD_BIG5 2
#define LIB_WORD_PY 3

// Fix compilation on hurd
#ifndef MAP_NORESERVE
#define MAP_NORESERVE 0
#endif

void vConvertEndian(unsigned int * input)
{
	union {
    	unsigned int i;    
    	char c[4];
    } u, v;

    u.i = *input;
    v.c[0] = u.c[3];
    v.c[1] = u.c[2];
    v.c[2] = u.c[1];
    v.c[3] = u.c[0];
    *input = v.i;
}

struct _worditem
{
	gchar *word;
	gchar *data;
	glong datasize;
};

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
	return stardict_strcmp(((struct _worditem *)a)->word,((struct _worditem *)b)->word);
}

gchar *to_utf8_phonetic(const gchar *text, glong len)
{
	gchar return_text[1024];
	const gchar *s = text;
	const gchar *old_s;
	gint i = 0;
	gint str_len;
	gboolean processed;
	return_text[0] = '\0';	
	while (i < len) {
		processed = true;
		switch (*s) {
		case 'K':
			strcat(return_text, "æ"); break;
		case 'A':
			strcat(return_text, "ɑ"); break;
		case 'C':
			strcat(return_text, "ɒ"); break;
		case 'V':
			strcat(return_text, "ʌ"); break;
		case 'E':
			strcat(return_text, "ә"); break;
		case 'W':
			strcat(return_text, "є"); break;
		case 'G':
			strcat(return_text, "ŋ"); break;
		case 'Q':
			strcat(return_text, "θ"); break;
		case 'J':
			strcat(return_text, "ð"); break;
		case 'S':
			strcat(return_text, "ʃ"); break;
		case '3':
			strcat(return_text, "ʒ"); break;						
		case ':':
			strcat(return_text, "ː"); break;						
		case '9':
			strcat(return_text, "ɡ"); break;						
		case '!':
			strcat(return_text, "ˏ"); break;						
		case '`':
			strcat(return_text, "ˋ"); break;
		case '\'':
			strcat(return_text, "ˊ"); break;
		default:
			processed = false;
		}
		old_s = s;
		s = g_utf8_next_char(s);
		str_len = s - old_s;
		i+= str_len;
		if (!processed)
			strncat(return_text, old_s, str_len);
	}
	return g_strdup(return_text);
}

void convert(char *filename,char *idxheadfilename)
{
	struct stat stats;
	if (stat (idxheadfilename, &stats) == -1)
	{
		printf("idxhead file not exist!\n");
		return;
	}

	FILE *idxheadfile;
	idxheadfile = fopen(idxheadfilename,"r");
	gchar *buffer;
	buffer = (gchar *)g_malloc (stats.st_size + 1);
	size_t fread_size;
	fread_size = fread (buffer, 1, stats.st_size, idxheadfile);
	if (fread_size != (size_t)stats.st_size) {
		g_print("fread error!\n");
	}
	fclose (idxheadfile);
	buffer[stats.st_size] = '\0';
	//gboolean sametypesequence = FALSE;
	//if (strstr(buffer,"sametypesequence="))
		//sametypesequence = TRUE;
	
	//in the next code we will always treat sametypesequence to be TRUE.
	//as now all old stardict dictionaries use these two feature.	
	
	FILE *idxfile,*dicfile;
	gchar str[1024],basename[256];
	
	strcpy(basename,idxheadfilename);
	basename[strlen(idxheadfilename)-8]='\0';
	
	sprintf(str,"%s.idx",basename);
	idxfile = fopen(str,"w");
	sprintf(str,"%s.dict",basename);
	dicfile = fopen(str,"w");
	
	fwrite(buffer, 1, stats.st_size, idxfile);
	g_free(buffer);
	
	long wordcount_offset = ftell(idxfile);
	glong tmpglong=0;
	fwrite(&(tmpglong),sizeof(glong),1,idxfile);

	int fd=open(filename,O_RDONLY);
    if(fd==-1)
    {
		g_print("open fail\n");
        return;
    }

    // get length of dicfile.
    struct stat stStat;
    if(fstat(fd,&stStat)!=0)
    {
        g_print("stat fail\n");
        return;
    }
    int iFileSize=stStat.st_size;
    
	// get item count
	lseek(fd,0-sizeof(int)*2,SEEK_END);
	unsigned int iCapacity,iStyle;
	ssize_t read_size;
	read_size = read(fd,&iCapacity,sizeof(int));
	if (read_size == -1) {
		g_print("read() error!\n");
	}
	read_size = read(fd,&iStyle,sizeof(int));
	if (read_size == -1) {
		g_print("read() error!\n");
	}
	//disable the next two line when the convert file is from the same arch machine.
#ifndef DISABLE_CONVERT_ENDIAN
	vConvertEndian(&iCapacity);
    vConvertEndian(&iStyle);
#endif

    unsigned char cIndex=(unsigned char)(iStyle>>24);
    unsigned char cWord=(unsigned char)(iStyle>>16);
    unsigned char cMeaning=(unsigned char)(iStyle>>8);
    unsigned char cMark=(unsigned char)iStyle;
	g_print("flag: %c %c %c\n",cIndex,cWord,cMeaning);

    // mmap the file to memory
    caddr_t pFileMem=(caddr_t)mmap( (caddr_t)0,iFileSize-sizeof(int)*2,
                            PROT_READ,MAP_SHARED|MAP_NORESERVE,fd,0 );
    if(pFileMem==MAP_FAILED)
    {
        g_print("mmap fail\n");
        return;
    }

    // begin to read items.
    caddr_t p=pFileMem;
    caddr_t pMeaning, pMark;
	gchar *utf8_str;
#ifndef DISABLE_CONVERT_LOCALE
	gchar *locale_str;
	gsize locale_write_size;
	GIConv locale_converter;
	GIConv utf8_converter;
//	locale_converter = g_iconv_open("GB2312","BIG5");
//	utf8_converter = g_iconv_open("UTF-8","GB2312");

	locale_converter = g_iconv_open("BIG5","GB2312");
	utf8_converter = g_iconv_open("UTF-8","BIG5");
	
	//the locale_converter have no problem!but why it fail later?
/*	locale_str = g_convert_with_iconv("�~�^���",8,locale_converter,NULL,&locale_write_size,NULL);
	if (!locale_str) {
		g_print("convert fail\n");		
	}
	else
		printf("%s",locale_str);
	return;*/

#endif
	gsize write_size;
	long tmp_long,wordcount=0;
	int word_len, meaning_len,mark_len=0;
	gulong iLength=0;
		
	GArray *array = g_array_sized_new(FALSE,FALSE, sizeof(struct _worditem),iCapacity);
    struct _worditem worditem;
	glong old_size;
	
	while(p<pFileMem+iFileSize-sizeof(int)*2 && iLength<iCapacity)
    {
		iLength++;
		word_len = strlen(p);
#ifndef DISABLE_CONVERT_LOCALE
		locale_str = g_convert_with_iconv(p,word_len,locale_converter,NULL,&locale_write_size,NULL);
		if (locale_str) {
			utf8_str = g_convert_with_iconv(locale_str,locale_write_size,utf8_converter,NULL,NULL,NULL);
			g_free(locale_str);
		}
		else {
			printf("%s convert to other locale error!!!\n",p);
			utf8_str = NULL;
		}
#else
		utf8_str = g_locale_to_utf8(p,word_len,NULL,NULL,NULL);
#endif
		if (utf8_str)
			g_strstrip(utf8_str);
		if (!utf8_str || (*utf8_str=='\0'))
		{
			printf("%s convert to utf8 error!!!\n",p);
			pMeaning=p+word_len+1;
			meaning_len = strlen(pMeaning);
	        if ( !cMark )
	            p = pMeaning+meaning_len+1;
    	    else
			{
	            pMark = pMeaning+meaning_len+1;
				mark_len = strlen(pMark);
        	    p = pMark+mark_len+1;
			}
			if (utf8_str)
				g_free(utf8_str);
			continue;
		}
		worditem.word = utf8_str;
		//utf8_str will be free at last.
		worditem.data = NULL;
		worditem.datasize = 0;		
		old_size=0;
		
        pMeaning=p+word_len+1;
		meaning_len = strlen(pMeaning);
        if ( !cMark )   // no Mark field, eg py2gb
            pMark = NULL;
        else
		{			
            pMark = pMeaning+meaning_len+1;
			mark_len = strlen(pMark);
#ifndef DISABLE_CONVERT_LOCALE
			locale_str = g_convert_with_iconv(pMark,mark_len,locale_converter,NULL,&locale_write_size,NULL);
			if (locale_str) {
				utf8_str = g_convert_with_iconv(locale_str,locale_write_size,utf8_converter,NULL,&write_size,NULL);
				g_free(locale_str);
			}
			else {
				printf("%s convert to other locale error!!!\n",pMark);
				utf8_str = NULL;
			}
#else
			utf8_str = g_locale_to_utf8(pMark,mark_len,NULL,&write_size,NULL); //mark may contains Chinese too,ie. "not"
#endif
			if (utf8_str)
			{		
				gchar *p_str = to_utf8_phonetic(utf8_str, write_size);
				g_free(utf8_str);
				write_size = strlen(p_str);
				worditem.datasize += (write_size + 1);
					
				worditem.data = (gchar *)g_realloc(worditem.data,worditem.datasize);
				/*if ((cWord == LIB_WORD_GB) || (cWord == LIB_WORD_BIG5))
					memcpy(worditem.data+old_size,"Y",sizeof(gchar)); // Chinese Yin Biao
				else
					memcpy(worditem.data+old_size,"T",sizeof(gchar));  // English Phonetic
				old_size+=sizeof(gchar);
				tmp_long = write_size;				
				memcpy(worditem.data+old_size,&tmp_long,sizeof(glong));
				old_size+=sizeof(glong);*/
				memcpy(worditem.data+old_size,p_str,write_size+1);				
				old_size+= (write_size+1);
				g_free(p_str);
			}
			else {
				worditem.datasize += 1;
				worditem.data = (gchar *)g_realloc(worditem.data,worditem.datasize);
				memcpy(worditem.data+old_size,"", 1);				
				printf("%s 's mark convert to utf8 error!\n",p);
			}
		}

#ifndef DISABLE_CONVERT_LOCALE
			locale_str = g_convert_with_iconv(pMeaning, meaning_len,locale_converter,NULL,&locale_write_size,NULL);
			if (locale_str) {
				utf8_str = g_convert_with_iconv(locale_str,locale_write_size,utf8_converter,NULL,&write_size,NULL);
				g_free(locale_str);
			}
			else {
				printf("%s convert to other locale error!!!\n",pMeaning);
				utf8_str = NULL;
			}
#else
		utf8_str = g_locale_to_utf8(pMeaning,meaning_len,NULL,&write_size,NULL);
#endif		
		if (utf8_str)
		{
			tmp_long = write_size;
		}
		else
		{
			printf("%s 's meaning convert to utf8 error!\n",p);
			tmp_long = 1;
			utf8_str = g_strdup("");
		}
		worditem.datasize += tmp_long;
		worditem.data = (gchar *)g_realloc(worditem.data,worditem.datasize);		
		/*memcpy(worditem.data+old_size,"M",sizeof(gchar));
		old_size+=sizeof(gchar);
		memcpy(worditem.data+old_size,&tmp_long,sizeof(glong));
		old_size+=sizeof(glong);*/
		memcpy(worditem.data+old_size,utf8_str,tmp_long);				
		old_size+= tmp_long;
		g_free(utf8_str);
		
		g_array_append_val(array, worditem);
		
        if ( !cMark )
            p = pMeaning+meaning_len+1;
        else
            p = pMark+mark_len+1;
		wordcount++;
    }
#ifndef DISABLE_CONVERT_LOCALE
	g_iconv_close(locale_converter);
	g_iconv_close(utf8_converter);
#endif

	//g_qsort_with_data(parray->pdata,parray->len,sizeof(gpointer),comparefunc,NULL);
	g_array_sort(array,comparefunc);
	
	long offset_old=0;
	
	gulong i;
	
	gchar *previous_word = g_strdup(""); //there should have no word equal this.
	glong previous_datasize = 0;
	gchar *previous_data = g_strdup("");
	struct _worditem *pworditem;
	for (i=0;i<array->len;i++)
	{
		pworditem = &g_array_index(array, struct _worditem, i);
		
		// should use g_ascii_strcasecmp() ??
		if (strcmp(previous_word,pworditem->word)==0) {
			if ((previous_datasize == pworditem->datasize)&&
				(memcmp(previous_data,pworditem->data,previous_datasize)==0)) {
				
				g_print("word %s is complete duplicated! droped!\n" ,previous_word);				
				wordcount--;
				continue;
			}
			else {
				g_print("word %s is duplicated! droped!\n" ,previous_word);				
				wordcount--;
				continue;
				
				//i don't want to emerge here,it is a bad work!
				/*
				g_print("word %s is duplicated! merged!\n" ,previous_word);
				fseek(dicfile,-sizeof(glong),SEEK_CUR);
				pworditem->datasize += previous_datasize;
				fwrite(&(pworditem->datasize),sizeof(glong),1,idxfile);
				fwrite(pworditem->data,sizeof(gchar),pworditem->datasize,dicfile);
				*/
			}									
		}
		g_free(previous_word);
		g_free(previous_data);
		previous_word = pworditem->word;
		previous_datasize = pworditem->datasize;
		previous_data = pworditem->data;
		
		offset_old = ftell(dicfile);
		fwrite(pworditem->data,sizeof(gchar),pworditem->datasize,dicfile);
		
		fwrite(pworditem->word,sizeof(gchar),strlen(pworditem->word)+1,idxfile);
		tmpglong = g_htonl(offset_old);
		fwrite(&(tmpglong),sizeof(glong),1,idxfile);
		tmpglong = g_htonl(pworditem->datasize);
		fwrite(&(tmpglong),sizeof(glong),1,idxfile);			
	}
	g_free(previous_word);
	g_free(previous_data);
	g_array_free(array,TRUE);
	
	fseek(idxfile,wordcount_offset,SEEK_SET);
	tmpglong = g_htonl(wordcount);
	fwrite(&(tmpglong),sizeof(glong),1,idxfile);
	
	g_print("old wordcount: %d\n",iCapacity);
	g_print("wordcount: %ld\n",wordcount);

    close(fd);
	fclose(idxfile);
	fclose(dicfile);
}
	
int
main(int argc,char * argv [])
{
	if (argc!=3) {
		printf("please type this:\n./olddic2newdic filename.dic filename.idxhead\n");
		return FALSE;
	}
	
	setlocale(LC_ALL, "");
	gtk_init (&argc, &argv);
	convert(argv[1],argv[2]);
	return FALSE;
}
