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

// Some Germany-Chinese dictionary data files are encoded both in Germany and Chinese, that is, Windows-1252 and GBK, this program try to convert these data files to UTF-8 format.

#include "stdio.h"
#include "stdlib.h"
#include <locale.h>
#include <string.h>
#include <sys/stat.h>
#include <ctype.h>

#include <string>

#include <glib.h>

bool isde(guchar c)
{
	if (c==0xFC || c==0xF6 || c==0xE4 || c==0xDF || c==0xDC || c==0xD6 || c==0xC4)
		return true;
	return false;
}

gchar *convert_de_first(gchar *p, GIConv *gb2utf)
{
	gchar *str;
	std::string string;
	while (*p) {
		if (isascii(*p)) {
			string+=*p;
		} else {
			if ((guchar)*p==0xFC && (isascii(*(p+1)) || isde(*(p+1)))) {
				string+="ü";
			} else if ((guchar)*p==0xF6 && (isascii(*(p+1)) || isde(*(p+1)))) {
				string+="ö";
			} else if ((guchar)*p==0xE4 && (isascii(*(p+1)) || isde(*(p+1)))) {
				string+="ä";
			} else if ((guchar)*p==0xDF && (isascii(*(p+1)) || isde(*(p+1)))) {
				string+="ß";
			} else if ((guchar)*p==0xDC && isascii(*(p+1))) {
				string+="Ü";
			} else if ((guchar)*p==0xD6 && isascii(*(p+1))) {
				string+="Ö";
			} else if ((guchar)*p==0xC4 && isascii(*(p+1))) {
				string+="Ä";
			} else if ((guchar)*p>=0x81 && (guchar)*p<=0xFE && (guchar)*(p+1)>=0x40 && (guchar)*(p+1)<=0xFE) {
				str = g_convert_with_iconv(p, 2, *gb2utf, NULL, NULL, NULL);
				if (str) {
					string+=str;
				} else {
					return NULL;
				}
				p++;
			} else {
				return NULL;
			}
		}
		p++;
	}
	return g_strdup(string.c_str());
}

gchar *convert_de_first2(gchar *p, GIConv *gb2utf)
{
	gchar *str;
	std::string string;
	while (*p) {
		if (isascii(*p)) {
			string+=*p;
		} else {
			if ((guchar)*p==0xFC) {
				string+="ü";
			} else if ((guchar)*p==0xF6) {
				string+="ö";
			} else if ((guchar)*p==0xE4) {
				string+="ä";
			} else if ((guchar)*p==0xDF) {
				string+="ß";
			} else if ((guchar)*p==0xDC) {
				string+="Ü";
			} else if ((guchar)*p==0xD6) {
				string+="Ö";
			} else if ((guchar)*p==0xC4) {
				string+="Ä";
			} else if ((guchar)*p>=0x81 && (guchar)*p<=0xFE && (guchar)*(p+1)>=0x40 && (guchar)*(p+1)<=0xFE) {
				str = g_convert_with_iconv(p, 2, *gb2utf, NULL, NULL, NULL);
				if (str) {
					string+=str;
				} else {
					return NULL;
				}
				p++;
			} else {
				return NULL;
			}
		}
		p++;
	}
	return g_strdup(string.c_str());
}

gchar *convert_gb_first(gchar *p, GIConv *gb2utf)
{
	gchar *str;
	std::string string;
	while (*p) {
		if (isascii(*p)) {
			string+=*p;
		} else {
			if ((guchar)*p>=0x81 && (guchar)*p<=0xFE && (guchar)*(p+1)>=0x40 && (guchar)*(p+1)<=0xFE) {
				str = g_convert_with_iconv(p, 2, *gb2utf, NULL, NULL, NULL);
				if (str) {
					string+=str;
				} else {
					return NULL;
				}
				p++;
			} else if ((guchar)*p==0xFC && (isascii(*(p+1)) || isde(*(p+1)))) {
				string+="ü";
			} else if ((guchar)*p==0xF6 && (isascii(*(p+1)) || isde(*(p+1)))) {
				string+="ö";
			} else if ((guchar)*p==0xE4 && (isascii(*(p+1)) || isde(*(p+1)))) {
				string+="ä";
			} else if ((guchar)*p==0xDF && (isascii(*(p+1)) || isde(*(p+1)))) {
				string+="ß";
			} else if ((guchar)*p==0xDC && isascii(*(p+1))) {
				string+="Ü";
			} else if ((guchar)*p==0xD6 && isascii(*(p+1))) {
				string+="Ö";
			} else if ((guchar)*p==0xC4 && isascii(*(p+1))) {
				string+="Ä";
			} else {
				return NULL;
			}
		}
		p++;
	}
	return g_strdup(string.c_str());	
}

gchar *convert_gb_first2(gchar *p, GIConv *gb2utf)
{
	gchar *str;
	std::string string;
	while (*p) {
		if (isascii(*p)) {
			string+=*p;
		} else {
			if ((guchar)*p>=0x81 && (guchar)*p<=0xFE && (guchar)*(p+1)>=0x40 && (guchar)*(p+1)<=0xFE) {
				str = g_convert_with_iconv(p, 2, *gb2utf, NULL, NULL, NULL);
				if (str) {
					string+=str;
				} else {
					return NULL;
				}
				p++;
			} else if ((guchar)*p==0xFC && (isascii(*(p+1)) || isde(*(p+1)))) {
				string+="ü";
			} else if ((guchar)*p==0xF6 && (isascii(*(p+1)) || isde(*(p+1)))) {
				string+="ö";
			} else if ((guchar)*p==0xE4 && (isascii(*(p+1)) || isde(*(p+1)))) {
				string+="ä";
			} else if ((guchar)*p==0xDF && (isascii(*(p+1)) || isde(*(p+1)))) {
				string+="ß";
			} else if ((guchar)*p==0xDC && isascii(*(p+1))) {
				string+="Ü";
			} else if ((guchar)*p==0xD6 && isascii(*(p+1))) {
				string+="Ö";
			} else if ((guchar)*p==0xC4 && isascii(*(p+1))) {
				string+="Ä";
			} else {
			}
		}
		p++;
	}
	return g_strdup(string.c_str());	
}

void convert(char *filename)
{
	struct stat stats;
        if (stat (filename, &stats) == -1)
        {
                printf("file not exist!\n");
                return;
        }
        gchar *basefilename = g_path_get_basename(filename);
        FILE *degbfile;
        degbfile = fopen(filename,"r");

        gchar *buffer = (gchar *)g_malloc (stats.st_size + 1);
	size_t fread_size;
        fread_size = fread (buffer, 1, stats.st_size, degbfile);
	if (fread_size != (size_t)stats.st_size) {
		g_print("fread error!\n");
	}
        fclose (degbfile);
        buffer[stats.st_size] = '\0';

	gchar utffilename[256];
	sprintf(utffilename, "%s.utf", basefilename);
	FILE *utffile = fopen(utffilename,"w");

	GIConv gb2utf = g_iconv_open("UTF-8", "GBK");

	gchar *p, *p1, *p2;
	p=buffer;
	gchar *str;
	int linenum = 1;
	while (true) {
		if (*p=='\0')
			break;
		p1 = strchr(p, '\n');
		if (!p1) {
			g_print("Error, no ending new line.\n");
			return;
		}
		*p1='\0';
		p2 = strchr(p, '\t');
		if (p2) {
			*p2='\0';
			str = convert_de_first(p ,&gb2utf);
			if (str) {
				fwrite(str, strlen(str), 1, utffile);
				g_free(str);
			} else {
				str = convert_gb_first(p ,&gb2utf);
				if (str) {
					fwrite(str, strlen(str), 1, utffile);
					g_free(str);
				} else {
					str = convert_de_first2(p ,&gb2utf);
					if (str) {
						fwrite(str, strlen(str), 1, utffile);
						g_free(str);
					} else {
						str = convert_gb_first2(p ,&gb2utf);
		                                if (str) {
                		                        fwrite(str, strlen(str), 1, utffile);
                                		        g_free(str);
							g_print("Warning: %d\n", linenum);
		                                } else {
							g_print("Error1: %d\n", linenum);
						}
					}
				}
			}
			fwrite("\t", 1, 1, utffile);
			p2++;
			str = convert_de_first(p2 ,&gb2utf);
			if (str) {
				fwrite(str, strlen(str), 1, utffile);
				g_free(str);
			} else {
				str = convert_gb_first(p2 ,&gb2utf);
                                if (str) {
                                        fwrite(str, strlen(str), 1, utffile);
                                        g_free(str);
                                } else {
					str = convert_de_first2(p2 ,&gb2utf);
					if (str) {
                                		fwrite(str, strlen(str), 1, utffile);
                                		g_free(str);
                        		} else {
						str = convert_gb_first2(p2 ,&gb2utf);
		                                if (str) {
                		                        fwrite(str, strlen(str), 1, utffile);
                                		        g_free(str);
							g_print("Warning: %d\n", linenum);
		                                } else {
                	                        	g_print("Error2: %d\n", linenum);
						}
					}
                                }
                        }
		} else {
			str = convert_de_first(p ,&gb2utf);
			if (str) {
				fwrite(str, strlen(str), 1, utffile);
				g_free(str);
			} else {
				str = convert_gb_first(p ,&gb2utf);
				if (str) {
                                        fwrite(str, strlen(str), 1, utffile);
                                        g_free(str);
                                } else {
					str = convert_de_first2(p ,&gb2utf);
                        		if (str) {
                                		fwrite(str, strlen(str), 1, utffile);
                                		g_free(str);
                        		} else {
						str = convert_gb_first2(p ,&gb2utf);
		                                if (str) {
                		                        fwrite(str, strlen(str), 1, utffile);
                                		        g_free(str);
							g_print("Warning: %d\n", linenum);
		                                } else {
                	                        	g_print("Error: %d\n", linenum);
						}
					}
                                }
			}
		}
		fwrite("\n", 1, 1, utffile);
		p=p1+1;
		linenum++;
	} 

	g_iconv_close(gb2utf);
	fclose(utffile);

	g_free(buffer);
	g_free(basefilename);
}

int main(int argc,char * argv [])
{
        if (argc<2) {
                printf("please type this:\n./degb2utf fundset.txt\n");
                return FALSE;
        }
	setlocale(LC_ALL, "");
	for (int i=1; i< argc; i++)
                convert (argv[i]);
        return FALSE;
}

