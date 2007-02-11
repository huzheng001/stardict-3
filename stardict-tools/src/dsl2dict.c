/*
 *  dsl2dict.c - converter from Lingvo (http://www.lingvo.com) Dictionary
 *  Specification Language (.dsl extension) to .dict format.
 *
 *  Version 0.1
 *  Mon Apr 10 21:41:37 2006
 *  Copyright  2006  Eugene Dmitriev
 *  Email: strdup@gmail.com
 * 
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

/*
 * This tool is intended to convert dictionary files in DSL format (see
 * Lingvo help->Allendix->DSL compiler) to .dict format for working it
 * under StarDict. 
 *
 * I wrote this tool fast and dirty for personal using but I hope it will useful
 * for somebody else. If you get any problems, questions or advices don't hesitate
 * to contact me by e-mail. I need feedback from users.
 * 
 * Please be in respect to dictionary license and it's author.
 *
 * P.S. Exuse me for my English and feel free to correct my grammar and lexic
 * mistakes :)
 */

/*
 TODO list in the order of priorities:

 - Support Lingvo DSL formatting commands:
 Translate DSL formatting commands to Pango markup language (but
 sometimes Lingvo DSL file has overlapped commands).

 - Replacement function:
 Realise replacement functionality (see [c COLOR] pattern as example).

 - Support ASCII files.

 - add .ann file to the corresponding dictionary.

 - i18n l10n, gettext.

 - Support many languages:
 Support as large as possible languages, at least all which original
 Lingvo supports officisally.

 - Support unsuppported languages (Chinese, etc.).
 */

#define _GNU_SOURCE

#include <stdio.h>
#include <wchar.h>
#include <stdlib.h>
#include <unistd.h>
#include <iconv.h>
#include <string.h>
#include <errno.h>
#include <pcre.h>
#include <locale.h>
#include <libgen.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <gtk/gtk.h>
#include <glib.h>


#define BUFSIZE BUFSIZ
#define UCS_MARK 0xFEFF
#define UCS_CHARLEN 2
#define BUFOVERLAP 64
#define IDX_NODE_SIZE 1000


extern char *optarg;
extern int optind;
extern int errno;
extern int (*pcre_callout)(pcre_callout_block *);


struct worditem {
    char *word;
    long word_offset;
    long word_size;
};

struct idx_node {
    struct worditem *idx_items;
    struct idx_node *next_node;
};

struct pattern {
    const char *regex;
    const char *repl;
    pcre *compile;
    pcre_extra *extra;
    int repl_marker;
    unsigned char (*replaces)[2];
};

struct pattern patterns[] = {
    {NULL, NULL, NULL, NULL, 0, NULL},
// Lingvo DSL language (see Lingvo help)
    {"\t(?C)", "  ", NULL, NULL, 0, NULL},
    {"\r\n(?C)", "\n", NULL, NULL, 0, NULL},
    {"\\\\\\[(?C)","[", NULL, NULL, 0, NULL},
    {"\\\\\\](?C)","]", NULL, NULL, 0, NULL},
    {"\\\\ (?C)", " ", NULL, NULL, 0, NULL},
    {"\\[m0\\](?C)", "", NULL, NULL, 0, NULL},
    {"\\[m1\\](?C)", "", NULL, NULL, 0, NULL},
    {"\\[m2\\](?C)", "", NULL, NULL, 0, NULL},
    {"\\[m3\\](?C)", "", NULL, NULL, 0, NULL},
    {"\\[m4\\](?C)", "", NULL, NULL, 0, NULL},
    {"\\[m5\\](?C)", "", NULL, NULL, 0, NULL},
    {"\\[m6\\](?C)", "", NULL, NULL, 0, NULL},
    {"\\[m7\\](?C)", "", NULL, NULL, 0, NULL},
    {"\\[m8\\](?C)", "", NULL, NULL, 0, NULL},
    {"\\[m9\\](?C)", "", NULL, NULL, 0, NULL},
    {"\\[/m\\](?C)", "", NULL, NULL, 0, NULL},
    {"\\[t\\](?C)", "", NULL, NULL, 0, NULL},
    {"\\[/t\\](?C)", "", NULL, NULL, 0, NULL},
    {"\\[\\*\\](?C)", "", NULL, NULL, 0, NULL},
    {"\\[/\\*\\](?C)", "", NULL, NULL, 0, NULL},
    {"\\[p\\](?C)", "", NULL, NULL, 0, NULL},
    {"\\[/p\\](?C)", "", NULL, NULL, 0, NULL},
    {"\\[trn\\](?C)", "", NULL, NULL, 0, NULL},
    {"\\[/trn\\](?C)", "", NULL, NULL, 0, NULL},
    {"\\[!trs\\](?C)", "", NULL, NULL, 0, NULL},
    {"\\[/!trs\\](?C)", "", NULL, NULL, 0, NULL},
    {"\\[com\\](?C)", "", NULL, NULL, 0, NULL},
    {"\\[/com\\](?C)", "", NULL, NULL, 0, NULL},
    {"\\[lang[^]]+\\](?C)", "", NULL, NULL, 0, NULL},
    {"\\[/lang\\](?C)", "", NULL, NULL, 0, NULL},
    {"\\[s\\][^[]+\\[/s\\](?C)", "", NULL, NULL, 0, NULL},
    {"\\[b\\](?C)", "", NULL, NULL, 0, NULL},
    {"\\[/b\\](?C)", "", NULL, NULL, 0, NULL},
    {"\\[i\\](?C)", "", NULL, NULL, 0, NULL},
    {"\\[/i\\](?C)", "", NULL, NULL, 0, NULL},
    {"\\[u\\](?C)", "", NULL, NULL, 0, NULL},
    {"\\[/u\\](?C)", "", NULL, NULL, 0, NULL},
    {"\\[sub\\](?C)", "", NULL, NULL, 0, NULL},
    {"\\[/sub\\](?C)", "", NULL, NULL, 0, NULL},
    {"\\[sup\\](?C)", "", NULL, NULL, 0, NULL},
    {"\\[/sup\\](?C)", "", NULL, NULL, 0, NULL},
    {"\\[ref\\](?C)", "", NULL, NULL, 0, NULL},
    {"\\[/ref\\](?C)", "", NULL, NULL, 0, NULL},
    {"\\[c\\](?C)", "", NULL, NULL, 0, NULL},
    {"\\[c ([^]]+)\\](?C)", "", NULL, NULL, 0, NULL},
    {"\\[/c\\](?C)", "", NULL, NULL, 0, NULL},
    {"\\[ex\\](?C)", "", NULL, NULL, 0, NULL},
    {"\\[/ex\\](?C)", "", NULL, NULL, 0, NULL},

/*
// Replace with Pango markup language - not fully supported yet!
    {"\\[b\\](?C)", "<b>", NULL, NULL, 0, NULL},
    {"\\[/b\\](?C)", "</b>", NULL, NULL, 0, NULL},
    {"\\[i\\](?C)", "<i>", NULL, NULL, 0, NULL},
    {"\\[/i\\](?C)", "</i>", NULL, NULL, 0, NULL},
    {"\\[u\\](?C)", "<u>", NULL, NULL, 0, NULL},
    {"\\[/u\\](?C)", "</u>", NULL, NULL, 0, NULL},
    {"\\[sub\\](?C)", "<sub>", NULL, NULL, 0, NULL},
    {"\\[/sub\\](?C)", "</sub>", NULL, NULL, 0, NULL},
    {"\\[sup\\](?C)", "<sup>", NULL, NULL, 0, NULL},
    {"\\[/sup\\](?C)", "</sup>", NULL, NULL, 0, NULL},
    {"\\[ref\\](?C)", "<span foreground=\"blue\" underline=\"single\">", NULL, NULL, 0, NULL},
    {"\\[/ref\\](?C)", "</span>", NULL, NULL, 0, NULL},
    {"\\[c\\](?C)", "<span foreground=\"green\">", NULL, NULL, 0, NULL},
    {"\\[c ([^]]+)\\](?C)", "<span color=\\1>", NULL, NULL, 0, NULL},
    {"\\[/c\\](?C)", "</span>", NULL, NULL, 0, NULL},
    {"\\[ex\\](?C)", "<span foreground=\"grey\">", NULL, NULL, 0, NULL},
    {"\\[/ex\\](?C)", "</span>", NULL, NULL, 0, NULL},
*/

// UTF-8 Engish prononciation table
// If anybody would suggest something better - you are welcome!
    {"\\x{2021}(?C)",
     "\xC3\xA6",           // U+00E6 LATIN SMALL LETTER AE
     NULL, NULL, 0, NULL},
    {"\\x{2030}(?C)",
     "\xC3\xB0",           // U+00F0 LATIN SMALL LETTER ETH
     NULL, NULL, 0, NULL},
    {"\\x{40e}(?C)",
     "\xC4\xB1",           // U+0131 LATIN SMALL LETTER DOTLESS I
     NULL, NULL, 0, NULL},
    {"\\x{402}(?C)",
     "i\xCB\x90",          // 'i' & U+02D0 MODIFIER LETTER TRIANGULAR COLON
     NULL, NULL, 0, NULL},
    {"\\x{40a}(?C)",
     "\xC5\x8B",           // U+014B LATIN SMALL LETTER ENG
     NULL, NULL, 0, NULL},
    {"\\x{403}(?C)",
     "a\xCB\x90",          // 'a' & U+02D0 MODIFIER LETTER TRIANGULAR COLON
     NULL, NULL, 0, NULL},
    {"\\x{20ac}(?C)",
     "\xC9\x94",           // U+0254 LATIN SMALL LETTER OPEN O
     NULL, NULL, 0, NULL},
    {"\\x{201a}(?C)",
     "\xC9\x94\xCB\x90",   // U+0254 LATIN SMALL LETTER OPEN O & U+02D0 MODIFIER LETTER TRIANGULAR COLON
     NULL, NULL, 0, NULL},
    {"\\x{2020}(?C)",
     "\xC9\x99",           // U+0259 LATIN SMALL LETTER SCHWA
     NULL, NULL, 0, NULL},
    {"\\x{201e}(?C)",
     "\xC9\x99\xCB\x90",   // U+0259 LATIN SMALL LETTER SCHWA & U+02D0 MODIFIER LETTER TRIANGULAR COLON
     NULL, NULL, 0, NULL},
    {"\\x{45e}(?C)",
     "\xC9\x9B",           // U+025B LATIN SMALL LETTER OPEN E
     NULL, NULL, 0, NULL},
    {"\\x{40f}(?C)",
     "\xCA\x86",           // U+0286 LATIN SMALL LETTER ESH WITH CURL
     NULL, NULL, 0, NULL},
    {"\\x{2026}(?C)",
     "\xCA\x8C",           // U+028C LATIN SMALL LETTER TURNED V
     NULL, NULL, 0, NULL},
    {"\\x{409}(?C)",
     "\xCA\x93",           // U+0293 LATIN SMALL LETTER EZH WITH CURL
     NULL, NULL, 0, NULL},
    {"\\x{2039}(?C)",
     "\xCA\xA4",           // U+02A4 LATIN SMALL LETTER DEZH DIGRAPH
     NULL, NULL, 0, NULL},
    {"\\x{a0}(?C)",
     "\xCA\xA7",          // U+02A7 LATIN SMALL LETTER TESH DIGRAPH
     NULL, NULL, 0, NULL},
    {"\\x{40b}(?C)",
     "\xCE\xB8",          // U+03B8 GREEK SMALL LETTER THETA
     NULL, NULL, 0, NULL},
    {"\\x{b5}(?C)", "a", NULL, NULL, 0, NULL},
    {"\\x{a4}(?C)", "b", NULL, NULL, 0, NULL},
    {"\\x{ac}(?C)", "d", NULL, NULL, 0, NULL},
    {"\\x{b1}(?C)", "g", NULL, NULL, 0, NULL},
    {"\\x{406}(?C)", "h", NULL, NULL, 0, NULL},
    {"\\x{456}(?C)", "j", NULL, NULL, 0, NULL},
    {"\\x{b0}(?C)", "k", NULL, NULL, 0, NULL},
    {"\\x{407}(?C)", "r", NULL, NULL, 0, NULL},
    {"\\x{a9}(?C)", "s", NULL, NULL, 0, NULL},
    {"\\x{404}(?C)", "z", NULL, NULL, 0, NULL},
    {"\\x{452}(?C)", "v", NULL, NULL, 0, NULL},
    {"\\x{a6}(?C)", "w", NULL, NULL, 0, NULL},
    {"\\x{a7}(?C)", "f", NULL, NULL, 0, NULL},
    {"\\x{ae}(?C)", "l", NULL, NULL, 0, NULL},
    {"\\x{490}(?C)", "m", NULL, NULL, 0, NULL},
    {"\\x{ad}(?C)", "n", NULL, NULL, 0, NULL},
    {"\\x{408}(?C)", "p", NULL, NULL, 0, NULL},
    {"\\x{ab}(?C)", "t", NULL, NULL, 0, NULL},
    {"\\x{40c}(?C)", "u", NULL, NULL, 0, NULL},
    {"\\x{453}(?C)",
     "u\xCB\x90",        //  'u' & U+02D0 MODIFIER LETTER TRIANGULAR COLON
     NULL, NULL, 0, NULL},
};


static void print_usage()
{
    printf(
        "Usage: dsl2dict [-o directory_name][-h] dsl_file\n\n"
        "Options:\n"
        "-o directory_name\n"
        "\tDirectory name for all dictionary files - .dict, .idx, .ifo. Also\n"
        "\tthis name will be used for all these files with corresponding\n"
        "\tsuffix. If this option is omitted, the name of .dsl file without\n"
        "\ttrailing .dsl suffix will be used.\n"
        "\n"
        "-h\n"
        "\tPrint this information\n"
        "\n"
        "dsl_file\n"
        "\tThe .dsl dictionary for convert to .dict\n"
        );
}

static int callout(pcre_callout_block *cb)
{
    struct pattern *p = (struct pattern *)(cb->callout_data);
    (p->replaces)[cb->start_match][0] = p->repl_marker;
    (p->replaces)[cb->start_match][1] = (cb->current_position)-(cb->start_match);
    return 1;
}

// Stardict worlds sorting fuction - see DICTFILE_FORMAT
static int stardict_strcmp(const void *param1, const void *param2)
{
    struct worditem **idx_item1 = (struct worditem **)param1;
    struct worditem **idx_item2 = (struct worditem **)param2;
	int a;

	a = g_ascii_strcasecmp((*idx_item1)->word, (*idx_item2)->word);
	if (a == 0)
        return strcmp((*idx_item1)->word, (*idx_item2)->word);
	else
		return a;
}

static int fill_patterns(unsigned char (*replaces)[2])
{
    const unsigned char *lc_tables = pcre_maketables();
    int i;
    const char *pcre_errptr;
    int pcre_erroffset;

    setlocale(LC_CTYPE, "en_US.UTF-8");
    for (i=1, pcre_errptr = NULL; i < sizeof(patterns)/sizeof(patterns[0]); i++) {
        patterns[i].repl_marker = i;
        patterns[i].replaces = replaces;
        patterns[i].compile =
            pcre_compile(
                patterns[i].regex,
                PCRE_MULTILINE | PCRE_UTF8,
                &pcre_errptr, &pcre_erroffset,
                lc_tables
                );
        if ( !(patterns[i].compile) ) {
            fprintf(stderr, "%s\n", pcre_errptr);
            return 1;
        }

        patterns[i].extra = pcre_study(patterns[i].compile, 0, &pcre_errptr);
        if (pcre_errptr) {
            fprintf(stderr, "%s\n", pcre_errptr);
            return 1;
        } else if ( !(patterns[i].extra) ) {
            if ( !(patterns[i].extra = (pcre_extra *)malloc(sizeof(pcre_extra))) ) {
                fprintf(stderr, "In file %s, line %d ", __FILE__, __LINE__);
                perror("error allocate memory");
                exit(errno);
            }
        }

        patterns[i].extra->flags |= PCRE_EXTRA_CALLOUT_DATA;
        patterns[i].extra->callout_data = &patterns[i];
    }

    return 0;
}

static char *make_dict(char *dslfilename, char *outputdir, char *bname)
{
    char *dictfilename;
    FILE *dslfile, *dictfile;
    char inbuff[BUFSIZE+1] = { 0 };
    char outbuff[BUFSIZE+1] = { 0 };
    //FIXME: May be prefer to use ptrdiff_t for second element of replaces array 
    unsigned char replaces[BUFSIZE+1][2] = { {0, 0} };
    unsigned short firstword = 0;
    iconv_t iconv_direction;
    int i;

    if ( !(dslfile = fopen(dslfilename, "r")) ) {
        perror("Error open dslfile");
        exit(errno);
    }

    if ( !(dictfilename = (char *)malloc(strlen(outputdir)+strlen(bname)+strlen(".dict")+1)) ) { 
        fprintf(stderr, "In file %s, line %d ", __FILE__, __LINE__);
        perror("error allocate memory"); 
        exit(errno);
    } 
    sprintf(dictfilename, "%s%s.dict", outputdir, bname);
    if ( !(dictfile = fopen(dictfilename, "w")) ) {
        perror("Error open dictfile");
        exit(errno);
    }
    
    fread(&firstword, sizeof(firstword), 1, dslfile);
    if (UCS_MARK == firstword) {
        if ( 0 > (iconv_direction = iconv_open("UTF-8", "UCS-2")) ) {
            perror("Error conversion direction");
        }
    } else {
        //TODO list.
        fprintf(stderr, "Not Windows UCS-2 format\n");
        exit(1);
    }

    fill_patterns(replaces);

    while ( !(feof(dslfile)) ) {
        size_t inbuff_size;
        size_t outbuff_size = BUFSIZE;
        size_t writems;
        char *inbufptr = (char *) inbuff;
        char *outbufptr = (char *) outbuff;
        long filepos = BUFOVERLAP * UCS_CHARLEN;
        char *buffborder;
        int vector[30] = { 0 };

        inbuff_size = fread(inbuff, sizeof(char), BUFSIZE, dslfile);

        if ( !(feof(dslfile)) ) {
            inbuff_size -= filepos;
            if ( 0 > (iconv(iconv_direction,
                            &inbufptr, &inbuff_size, &outbufptr, &outbuff_size)) ) {
                perror("iconv error");
                exit(errno);
            }
            buffborder = outbufptr;
            inbuff_size += filepos;
            if ( 0 > (iconv(iconv_direction,
                            &inbufptr, &inbuff_size, &outbufptr, &outbuff_size)) ) {
                perror("iconv error");
                exit(errno);
            }
        } else {
            if ( 0 > (iconv(iconv_direction,
                            &inbufptr, &inbuff_size, &outbufptr, &outbuff_size)) ) {
                perror("iconv error");
                exit(errno);
            }
            buffborder = outbufptr;
        }
        *outbufptr = 0;
        
        for (i=1; i < sizeof(patterns)/sizeof(patterns[0]); i++) {
            int match_res =
                pcre_exec(
                    patterns[i].compile, patterns[i].extra,
                    outbuff, outbufptr-outbuff, 0,
                    PCRE_NOTBOL | PCRE_NOTEOL,
                    vector, sizeof(vector)/sizeof(vector[0])
                    );
            switch (match_res) {
            case PCRE_ERROR_NOMATCH: break;
            default:
                fprintf(stderr, "Matching error %d\n", match_res);
                break;
            }
        }
        
        for (i = 0, outbufptr = outbuff; outbuff+i < buffborder; i++) {
            if (replaces[i][0]) {
                writems = fwrite(outbufptr, sizeof(char), outbuff+i-outbufptr, dictfile);
                if ( (outbuff+i-outbufptr) != writems) {
                    fprintf(stderr, "Write error: not enough disk space or another error.\n");
                    exit(1);
                }
                writems = fwrite(
                    patterns[replaces[i][0]].repl,
                    sizeof(char),
                    strlen(patterns[replaces[i][0]].repl),
                    dictfile
                    );
                if ( (strlen(patterns[replaces[i][0]].repl)) != writems) {
                    fprintf(stderr, "Write error: not enough disk space or another error.\n");
                    exit(1);
                }
                outbufptr = outbuff + i + replaces[i][1];
            }
        }

        if (outbufptr >= buffborder) {
            filepos -= (outbufptr - buffborder) * 2;
        } else {
            writems = fwrite(outbufptr, sizeof(char), buffborder-outbufptr, dictfile);
            if ( (buffborder-outbufptr) != writems) {
                fprintf(stderr, "Write error - not enough disk space or another error.\n");
                exit(1);
            }
        }

        memset(replaces, 0, (BUFSIZE+1)*sizeof(replaces[0]));
        if ( !(feof(dslfile)) ) fseek(dslfile, -filepos, SEEK_CUR);
    }

    if ( 0 > (iconv_close(iconv_direction)) ) perror("Error close iconv");
    if (fclose(dictfile)) perror("Error close dictfile");
    if (fclose(dslfile)) perror("Error close dslfile");
    return dictfilename;
}

static int make_idxifo(char *dictfilename, char *outputdir, char *bname) {
    char *idxfilename, *ifofilename;
    FILE *dictfile, *idxfile, *ifofile;
    struct worditem *last_idx = NULL, *idx_items, **arr_idxitemsptr;
    struct idx_node *firstnode = NULL, *node = NULL;
    int i, j, wordcounter = 0;

    if ( !(idxfilename = (char *)malloc(strlen(outputdir)+strlen(bname)+strlen(".idx")+1)) ) {
        fprintf(stderr, "In file %s, line %d ", __FILE__, __LINE__);
        perror("error allocate memory");
        exit(errno);
    }
    sprintf(idxfilename, "%s%s.idx", outputdir, bname);
    if ( !(idxfile = fopen(idxfilename, "w")) ) {
        perror("Error open idxfile");
        return errno;
    }

    if ( !(ifofilename = (char *)malloc(strlen(outputdir)+strlen(bname)+strlen(".ifo")+1)) ) {
        fprintf(stderr, "In file %s, line %d ", __FILE__, __LINE__);
        perror("error allocate memory");
        exit(errno);
    }
    sprintf(ifofilename, "%s%s.ifo", outputdir, bname);
    if ( !(ifofile = fopen(ifofilename, "w")) ) {
        perror("Error open ifofile");
        return errno;
    }

    if ( !(dictfile = fopen(dictfilename, "r")) ) {
        perror("Error open dictfile");
        return errno;
    }

    do {
        i = 0;
        if ( !(firstnode) ) {
            node = (struct idx_node *)calloc(1,sizeof(*node));
            firstnode = node;
        } else {
            node->next_node = (struct idx_node *)calloc(1,sizeof(*node));
            node = node->next_node;
        }
        node->idx_items = (struct worditem *)calloc(IDX_NODE_SIZE, sizeof(struct worditem));

        while ( !(feof(dictfile)) && (i < IDX_NODE_SIZE) ) {
            char *word = NULL;
            size_t len = 0;
            ssize_t readlen = getline(&word, &len, dictfile);

            if (readlen > 0 && 0x20 != *word && 0xA != *word && 0x9 != *word && '#' != *word) {
                word[readlen-1] = 0;
                if ( !(node->idx_items[i].word = strdup(word)) ) {
                    fprintf(stderr, "In file %s, line %d ", __FILE__, __LINE__);
                    perror("error allocate memory");
                    exit(errno);
                }

                node->idx_items[i].word_offset = ftell(dictfile);
                if (last_idx)
                    last_idx->word_size =
                        node->idx_items[i].word_offset - readlen - last_idx->word_offset;
                last_idx = &node->idx_items[i];
                i++;
            }
        }
        wordcounter += i;
    } while ( !(feof(dictfile)) );
    last_idx->word_size = ftell(dictfile) - last_idx->word_offset;
    if (fclose(dictfile)) perror("Error close dictfile");

    if ( !(idx_items = (struct worditem *)malloc(wordcounter * sizeof(struct worditem))) ) {
        fprintf(stderr, "In file %s, line %d ", __FILE__, __LINE__);
        perror("error allocate memory");
        exit(errno);
    }
    struct worditem *idxitemsptr = idx_items;
    arr_idxitemsptr = (struct worditem **)malloc(wordcounter * sizeof(struct worditem *));
    for (j=0, node=firstnode; node; firstnode=node, node=node->next_node, free(firstnode)) {
        for (i = 0; (i < IDX_NODE_SIZE) && (node->idx_items[i].word); i++, j++) {
            arr_idxitemsptr[j] = idxitemsptr;
            idxitemsptr =
                (struct worditem *)mempcpy(
                    (void *)idxitemsptr,
                    (void *)&(node->idx_items[i]),
                    sizeof(struct worditem));
        }
        free(node->idx_items);
    }

    qsort(arr_idxitemsptr, wordcounter, sizeof(struct worditem *), stardict_strcmp);

    long hton;
    for (i = 0; i < wordcounter ; i++) {
        fwrite(arr_idxitemsptr[i]->word, sizeof(char), strlen(arr_idxitemsptr[i]->word)+1, idxfile);
        hton = htonl(arr_idxitemsptr[i]->word_offset);
        fwrite(&hton, sizeof(long), 1, idxfile);
        hton = htonl(arr_idxitemsptr[i]->word_size);
        fwrite(&hton, sizeof(long), 1, idxfile);
    }

    fprintf(ifofile,
            "StarDict's dict ifo file\n"
            "version=2.4.2\n"
            "bookname=%s\n"
            "wordcount=%d\n"
            "idxfilesize=%ld\n"
            "sametypesequence=m\n",
            bname, wordcounter, ftell(idxfile)
        );

    if (fclose(ifofile)) perror("Error close dictfile");
    free(ifofilename);

    if (fclose(idxfile)) perror("Error close dictfile");
    free(idxfilename);

    return errno;
}


int main(int argc, char *argv[])
{
    char *outputdir = NULL, *bname, *dname, *dirc, *basec, *dslfilename, *dictfilename;
    int option_char;
    pcre_callout = callout;

    while (-1 != (option_char = getopt(argc, argv, "o:h"))) {
        switch (option_char) {
        case 'o':
            outputdir = strdup(optarg);
            break;
        case 'h':
        default:
            print_usage();
            return 0;
        }
    }

    if (!argv[optind] || argv[optind+1]) {
        print_usage();
        return 1;
    }
    dslfilename = argv[optind];

    if ( !(outputdir) ) {
        char *suffpos;
        outputdir = dslfilename;
        if ( (suffpos = strstr(outputdir, ".dsl")) ) *suffpos = 0;
    }

    dirc = strdup(outputdir);
    basec = strdup(outputdir);
    dname = (char *)dirname(dirc); //Cast to avoid warning messages from compiler
    bname = (char *)basename(basec); //Cast to avoid warning messages from compiler
    if ( !(outputdir = (char *)malloc(strlen(dname)+strlen(bname)+3)) ) {
        fprintf(stderr, "In file %s, line %d ", __FILE__, __LINE__);
        perror("error allocate memory");
        exit(errno);
    }
    sprintf(outputdir, "%s/%s/", dname, bname);
    if ( mkdir(outputdir, 0755) ) {
        perror("Error create directory");
        return errno;
    }

    dictfilename = make_dict(dslfilename, outputdir, bname);
    make_idxifo(dictfilename, outputdir, bname);

    //FIXME: correctly release memory for compiled regex and pcre_extra()

    free(dictfilename);
    free(outputdir);
    return 0;
}
