## Process this file with automake to produce Makefile.in
COMMONLIB_CPPFLAGS = -I$(top_srcdir)/$(COMMONLIB_INCLUDE_DIR)
COMMONLIB_LIB = $(top_builddir)/$(COMMONLIB_LIBRARY)

SUBDIRS= 

bin_PROGRAMS =

noinst_PROGRAMS = stardict-editor pydict2dic olddic2newdic oxford2dic directory2dic dictd2dic \
	wquick2dic ec50 directory2treedic treedict2dir jdictionary mova xmlinout \
	soothill kanjidic2 powerword kdic 21tech 21shiji buddhist tabfile \
	cedict edict duden stardict-dict-update degb2utf frgb2utf jpgb2utf gmx2utf \
	rucn kingsoft kingsoft2 wikipedia wikipediaImage babylon stardict2txt stardict-verify \
	fest2dict i2e2dict downloadwiki ooo2dict myspell2dic exc2i2e \
	dictbuilder tabfile2sql KangXi Unihan xiaoxuetang-ja wubi ydp2dict \
	wordnet lingvosound2resdb resdatabase2dir dir2resdatabase stardict-index \
	sd2foldoc	\
	xml_meaning \
	stardict-text2bin stardict-bin2text \
	stardict-repair

AM_CPPFLAGS = $(STARDICT_CFLAGS) -I$(top_builddir) -I$(top_srcdir)

lingvosound2resdb_LDFLAGS = 
lingvosound2resdb_LDADD = $(STARDICT_LIBS)
lingvosound2resdb_SOURCES = lingvosound2resdb.cpp

dir2resdatabase_CPPFLAGS = $(AM_CPPFLAGS) $(COMMONLIB_CPPFLAGS)
dir2resdatabase_LDFLAGS = 
dir2resdatabase_LDADD = $(COMMONLIB_LIB) $(STARDICT_LIBS)
dir2resdatabase_SOURCES = dir2resdatabase.cpp

resdatabase2dir_CPPFLAGS = $(AM_CPPFLAGS) $(COMMONLIB_CPPFLAGS)
resdatabase2dir_LDFLAGS = 
resdatabase2dir_LDADD = $(COMMONLIB_LIB) $(STARDICT_LIBS)
resdatabase2dir_SOURCES = resdatabase2dir.cpp

pydict2dic_LDFLAGS = 
pydict2dic_LDADD = $(STARDICT_LIBS)
pydict2dic_SOURCES = pydict2dic.c

olddic2newdic_LDFLAGS = 
olddic2newdic_LDADD = $(STARDICT_LIBS)
olddic2newdic_SOURCES = olddic2newdic.cpp

oxford2dic_LDFLAGS = 
oxford2dic_LDADD = $(STARDICT_LIBS)
oxford2dic_SOURCES = oxford2dic.cpp

directory2dic_LDFLAGS = 
directory2dic_LDADD = $(STARDICT_LIBS)
directory2dic_SOURCES = directory2dic.cpp

dictd2dic_LDFLAGS = 
dictd2dic_LDADD = $(STARDICT_LIBS)
dictd2dic_SOURCES = dictd2dic.cpp

wquick2dic_LDFLAGS = 
wquick2dic_LDADD = $(STARDICT_LIBS)
wquick2dic_SOURCES = wquick2dic.cpp

ec50_LDFLAGS = 
ec50_LDADD = $(STARDICT_LIBS)
ec50_SOURCES = ec50.cpp

directory2treedic_LDFLAGS = 
directory2treedic_LDADD = $(STARDICT_LIBS)
directory2treedic_SOURCES = directory2treedic.cpp

treedict2dir_LDFLAGS =
treedict2dir_LDADD = $(STARDICT_LIBS)
treedict2dir_SOURCES = treedict2dir.cpp

jdictionary_LDFLAGS = 
jdictionary_LDADD = $(STARDICT_LIBS)
jdictionary_SOURCES = jdictionary.cpp

mova_LDFLAGS = 
mova_LDADD = $(STARDICT_LIBS)
mova_SOURCES = mova.cpp

xmlinout_LDFLAGS =
xmlinout_LDADD = $(STARDICT_LIBS)
xmlinout_SOURCES = xmlinout.cpp

xml_meaning_LDFLAGS =
xml_meaning_LDADD = $(STARDICT_LIBS)
xml_meaning_SOURCES = xml_meaning.cpp

soothill_LDFLAGS =
soothill_LDADD = $(STARDICT_LIBS)
soothill_SOURCES = soothill.cpp

kanjidic2_LDFLAGS =
kanjidic2_LDADD = $(STARDICT_LIBS)
kanjidic2_SOURCES = kanjidic2.cpp

powerword_LDFLAGS =
powerword_LDADD = $(STARDICT_LIBS)
powerword_SOURCES = powerword.cpp

kdic_LDFLAGS =
kdic_LDADD = $(STARDICT_LIBS)
kdic_SOURCES = kdic.cpp

21tech_LDFLAGS =
21tech_LDADD = $(STARDICT_LIBS)
21tech_SOURCES = 21tech.cpp

21shiji_LDFLAGS =
21shiji_LDADD = $(STARDICT_LIBS)
21shiji_SOURCES = 21shiji.cpp

buddhist_LDFLAGS =
buddhist_LDADD = $(STARDICT_LIBS)
buddhist_SOURCES = buddhist.cpp

tabfile_CPPFLAGS = $(AM_CPPFLAGS) $(COMMONLIB_CPPFLAGS)
tabfile_LDFLAGS =
tabfile_LDADD = $(COMMONLIB_LIB) $(STARDICT_LIBS)
tabfile_SOURCES = tabfile.cpp libtabfile.cpp \
					libtabfile.h

cedict_LDFLAGS =
cedict_LDADD = $(STARDICT_LIBS)
cedict_SOURCES = cedict.cpp

edict_LDFLAGS =
edict_LDADD = $(STARDICT_LIBS)
edict_SOURCES = edict.cpp

duden_LDFLAGS =
duden_LDADD = $(STARDICT_LIBS)
duden_SOURCES = duden.cpp

degb2utf_LDFLAGS =
degb2utf_LDADD = $(STARDICT_LIBS)
degb2utf_SOURCES = degb2utf.cpp

frgb2utf_LDFLAGS =
frgb2utf_LDADD = $(STARDICT_LIBS)
frgb2utf_SOURCES = frgb2utf.cpp

jpgb2utf_LDFLAGS =
jpgb2utf_LDADD = $(STARDICT_LIBS)
jpgb2utf_SOURCES = jpgb2utf.cpp

gmx2utf_LDFLAGS =
gmx2utf_LDADD = $(STARDICT_LIBS)
gmx2utf_SOURCES = gmx2utf.cpp

rucn_LDFLAGS =
rucn_LDADD = $(STARDICT_LIBS)
rucn_SOURCES = rucn.cpp

kingsoft_LDFLAGS =
kingsoft_LDADD = $(STARDICT_LIBS)
kingsoft_SOURCES = kingsoft.cpp

kingsoft2_LDFLAGS =
kingsoft2_LDADD = $(STARDICT_LIBS)
kingsoft2_SOURCES = kingsoft2.cpp

wikipedia_CPPFLAGS = $(AM_CPPFLAGS) $(LFS_CFLAGS)
wikipedia_LDFLAGS = $(LFS_LDFLAGS)
wikipedia_LDADD = $(STARDICT_LIBS) $(LFS_LIBS)
wikipedia_SOURCES = wikipedia.cpp

wikipediaImage_CPPFLAGS = $(AM_CPPFLAGS) $(MYSQL_CFLAGS)
wikipediaImage_INCLUDES = $(MYSQL_INCLUDES)
wikipediaImage_LDADD = $(STARDICT_LIBS) $(MYSQL_LIBS)
wikipediaImage_SOURCES = wikipediaImage.cpp

tabfile2sql_CPPFLAGS = $(AM_CPPFLAGS) $(MYSQL_CFLAGS)
tabfile2sql_INCLUDES = $(MYSQL_INCLUDES)
tabfile2sql_LDADD = $(STARDICT_LIBS) $(MYSQL_LIBS)
tabfile2sql_SOURCES = tabfile2sql.cpp

babylon_CPPFLAGS = $(AM_CPPFLAGS) $(COMMONLIB_CPPFLAGS)
babylon_LDFLAGS =
babylon_LDADD = $(COMMONLIB_LIB) $(STARDICT_LIBS)
babylon_SOURCES = babylon.cpp libbabylonfile.cpp \
					libbabylonfile.h

stardict2txt_CPPFLAGS = $(AM_CPPFLAGS) $(COMMONLIB_CPPFLAGS)
stardict2txt_LDFLAGS =
stardict2txt_LDADD = $(COMMONLIB_LIB) $(STARDICT_LIBS)
stardict2txt_SOURCES = stardict2txt.cpp libstardict2txt.cpp \
						libstardict2txt.h

stardict_dict_update_LDFLAGS = 
stardict_dict_update_LDADD = $(STARDICT_LIBS)			
stardict_dict_update_SOURCES = stardict_dict_update.cpp

fest2dict_LDFLAGS = 
fest2dict_LDADD = 
fest2dict_SOURCES = fest2dict.c

i2e2dict_LDFLAGS = 
i2e2dict_LDADD = $(STARDICT_LIBS)
i2e2dict_SOURCES = i2e2dict.c

ooo2dict_LDFLAGS = 
ooo2dict_LDADD = $(STARDICT_LIBS)
ooo2dict_SOURCES = ooo2dict.c

myspell2dic_LDFLAGS = 
myspell2dic_LDADD = $(STARDICT_LIBS)
myspell2dic_SOURCES = myspell2dic.c

exc2i2e_LDFLAGS = 
exc2i2e_LDADD = $(STARDICT_LIBS)
exc2i2e_SOURCES = exc2i2e.c

dictbuilder_LDFLAGS = 
dictbuilder_LDADD = $(STARDICT_LIBS)
dictbuilder_SOURCES = dictbuilder.cpp \
						dictbuilder-tree.h

downloadwiki_LDFLAGS = 
downloadwiki_LDADD = $(STARDICT_LIBS)
downloadwiki_SOURCES = downloadwiki.cpp

KangXi_LDFLAGS =
KangXi_LDADD = $(STARDICT_LIBS)
KangXi_SOURCES = KangXi.cpp

Unihan_LDFLAGS =
Unihan_LDADD = $(STARDICT_LIBS)
Unihan_SOURCES = Unihan.cpp

xiaoxuetang_ja_LDFLAGS =
xiaoxuetang_ja_LDADD = $(STARDICT_LIBS)
xiaoxuetang_ja_SOURCES = xiaoxuetang-ja.cpp

wubi_LDFLAGS =
wubi_LDADD = $(STARDICT_LIBS)
wubi_SOURCES = wubi.cpp

ydp2dict_LDFLAGS =
ydp2dict_LDADD = $(STARDICT_LIBS)
ydp2dict_SOURCES = ydp2dict.c

wordnet_LDFLAGS =
wordnet_LDADD = $(STARDICT_LIBS)
wordnet_SOURCES = wordnet.cpp

sd2foldoc_LDFLAGS =
sd2foldoc_LDADD = $(STARDICT_LIBS)
sd2foldoc_SOURCES = sd2foldoc.cpp libsd2foldoc.cpp \
						libsd2foldoc.h

stardict_index_CPPFLAGS = $(AM_CPPFLAGS) $(COMMONLIB_CPPFLAGS)
stardict_index_LDFLAGS =
stardict_index_LDADD = $(COMMONLIB_LIB) $(STARDICT_LIBS)
stardict_index_SOURCES = stardict_index.cpp

stardict_verify_CPPFLAGS = $(AM_CPPFLAGS) $(LIBXML_CFLAGS) $(COMMONLIB_CPPFLAGS)
stardict_verify_LDFLAGS =
stardict_verify_LDADD = $(COMMONLIB_LIB) $(STARDICT_LIBS)
stardict_verify_SOURCES = stardict_verify.cpp

stardict_bin2text_CPPFLAGS = $(AM_CPPFLAGS) $(LIBXML_CFLAGS) $(COMMONLIB_CPPFLAGS)
stardict_bin2text_LDFLAGS =
stardict_bin2text_LDADD = $(COMMONLIB_LIB) $(STARDICT_LIBS)
stardict_bin2text_SOURCES = stardict_bin2text.cpp \
	lib_stardict_bin2text.cpp lib_stardict_bin2text.h \
	lib_common_dict.cpp lib_common_dict.h \
	lib_binary_parser_unify.h lib_binary_parser_unify.cpp \
	lib_dict_repair.cpp \
	lib_dict_repair.h

stardict_text2bin_CPPFLAGS = $(AM_CPPFLAGS) $(LIBXML_CFLAGS) $(COMMONLIB_CPPFLAGS)
stardict_text2bin_LDFLAGS =
stardict_text2bin_LDADD = $(COMMONLIB_LIB) $(STARDICT_LIBS) $(LIBXML_LIBS)
stardict_text2bin_SOURCES = stardict_text2bin.cpp \
	lib_stardict_text2bin.cpp lib_stardict_text2bin.h \
	lib_common_dict.cpp lib_common_dict.h \
	lib_textual_dict_parser.cpp lib_textual_dict_parser.h \
	lib_binary_dict_generator.cpp lib_binary_dict_generator.h \
	lib_dict_repair.cpp \
	lib_dict_repair.h

stardict_repair_CPPFLAGS = $(AM_CPPFLAGS) $(LIBXML_CFLAGS) $(COMMONLIB_CPPFLAGS)
stardict_repair_LDFLAGS =
stardict_repair_LDADD = $(COMMONLIB_LIB) $(STARDICT_LIBS) $(LIBXML_LIBS)
stardict_repair_SOURCES = stardict_repair.cpp \
	lib_stardict_repair.cpp lib_stardict_repair.h \
	lib_common_dict.cpp lib_common_dict.h \
	lib_binary_parser_unify.cpp lib_binary_parser_unify.h \
	lib_binary_dict_generator.cpp lib_binary_dict_generator.h \
	lib_dict_repair.cpp \
	lib_dict_repair.h

stardict_editor_CPPFLAGS = $(AM_CPPFLAGS) $(LIBXML_CFLAGS) $(EXPAT_CFLAGS) $(COMMONLIB_CPPFLAGS)
stardict_editor_LDFLAGS =
stardict_editor_LDADD = $(COMMONLIB_LIB) $(STARDICT_LIBS) $(LIBXML_LIBS) $(EXPAT_LIBS)
stardict_editor_SOURCES = stardict-editor.cpp libtabfile.cpp libtabfile.h \
	libbabylonfile.cpp libbabylonfile.h \
	libstardict2txt.cpp libstardict2txt.h  \
	lib_stardict_bin2text.cpp lib_stardict_bin2text.h \
	lib_common_dict.cpp lib_common_dict.h \
	lib_binary_parser_unify.h lib_binary_parser_unify.cpp \
	lib_stardict_text2bin.cpp lib_stardict_text2bin.h \
	lib_textual_dict_parser.cpp lib_textual_dict_parser.h \
	lib_binary_dict_generator.cpp lib_binary_dict_generator.h \
	lib_dict_repair.cpp lib_dict_repair.h \
	bgl_babylon.cpp bgl_babylon.h \
	bgl_babylonreader.cpp bgl_babylonreader.h \
	bgl_dictbuilder.h bgl_dictreader.h \
	bgl_stardictbuilder.cpp bgl_stardictbuilder.h \
	libbgl2txt.cpp libbgl2txt.h \
	charset_conv.cpp charset_conv.h compiler.h connector.cpp connector.h \
	dsl_ipa.cpp dsl_ipa.h dsl_parser.cpp file.cpp file.h \
	generator.cpp generator.h log.cpp log.h mapfile.cpp mapfile.h \
	normalize_tags.cpp normalize_tags.h parser.cpp parser.h \
	process.cpp process.h repository.h resource.h stardict_generator.cpp \
	utils.cpp utils.h xml.cpp \
	xml.h

EXTRA_DIST = example.ifo example_treedict.tar.bz2 hanzim2dict.py \
	jm2stardict.py uyghur2dict.py ncce2stardict.pl parse-oxford.perl \
	dictgen.php DeKDic.exe KSDrip.exe Po2Tab.zip ooo.sh myspell.sh \
	exc.sh i2e.sh dictbuilder.readme dictbuilder.example KangXiZiDian-djvu2tiff.py \
	extractKangXi.py makevietdict.py mkguangyunst.py stmerge.py \
	lingea-trd-decoder.py \
	dicts-dump.pl \
	stardict_images.sh \
	stardict_images.py
