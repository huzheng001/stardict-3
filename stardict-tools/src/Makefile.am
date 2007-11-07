## Process this file with automake to produce Makefile.in
SUBDIRS= 

bin_PROGRAMS = stardict-editor

noinst_PROGRAMS = pydict2dic olddic2newdic oxford2dic directory2dic dictd2dic wquick2dic ec50 directory2treedic treedict2dir jdictionary mova xmlinout soothill kanjidic2 powerword kdic 21tech 21shiji buddhist tabfile dsl2dict cedict edict duden stardict_dict_update degb2utf frgb2utf jpgb2utf gmx2utf rucn kingsoft wikipedia wikipediaImage babylon stardict2txt stardict_verify fest2dict i2e2dict downloadwiki testutf8 ooo2dict myspell2dic exc2i2e dictbuilder tabfile2sql KangXi Unihan xiaoxuetang-ja wubi bgl2txt ydp2dict wordnet

noinst_LIBRARIES = libbgl2txt.a

libbgl2txt_a_SOURCES = libbgl2txt.h libbgl2txt.cpp bgl_babylon.h bgl_babylon.cpp bgl_dictreader.h bgl_dictbuilder.h bgl_babylonreader.h bgl_babylonreader.cpp bgl_stardictbuilder.h bgl_stardictbuilder.cpp

INCLUDES = $(STARDICT_CFLAGS)

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

tabfile_LDFLAGS =
tabfile_LDADD = $(STARDICT_LIBS)
tabfile_SOURCES = tabfile.cpp libtabfile.cpp libtabfile.h libcommon.cpp libcommon.h

cedict_LDFLAGS =
cedict_LDADD = $(STARDICT_LIBS)
cedict_SOURCES = cedict.cpp

edict_LDFLAGS =
edict_LDADD = $(STARDICT_LIBS)
edict_SOURCES = edict.cpp

dsl2dict_LDFLAGS =
dsl2dict_LDADD = $(STARDICT_LIBS) $(DSL2DICT_LIBS)
dsl2dict_SOURCES = dsl2dict.c

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

wikipedia_CPPFLAGS = $(LFS_CFLAGS)
wikipedia_LDFLAGS = $(LFS_LDFLAGS)
wikipedia_LDADD = $(STARDICT_LIBS) $(LFS_LIBS)
wikipedia_SOURCES = wikipedia.cpp

wikipediaImage_CPPFLAGS = -I/usr/include/mysql
wikipediaImage_LDFLAGS = -L/usr/lib/mysql
wikipediaImage_LDADD = $(STARDICT_LIBS) -lmysqlclient
wikipediaImage_SOURCES = wikipediaImage.cpp

tabfile2sql_CPPFLAGS = -I/usr/include/mysql
tabfile2sql_LDFLAGS = -L/usr/lib/mysql
tabfile2sql_LDADD = $(STARDICT_LIBS) -lmysqlclient
tabfile2sql_SOURCES = tabfile2sql.cpp

babylon_LDFLAGS =
babylon_LDADD = $(STARDICT_LIBS)
babylon_SOURCES = babylon.cpp libbabylonfile.cpp libbabylonfile.h libcommon.cpp libcommon.h

stardict2txt_LDFLAGS =
stardict2txt_LDADD = $(STARDICT_LIBS)
stardict2txt_SOURCES = stardict2txt.cpp libstardict2txt.cpp libstardict2txt.h libcommon.cpp libcommon.h

stardict_dict_update_LDFLAGS = 
stardict_dict_update_LDADD = $(STARDICT_LIBS)			
stardict_dict_update_SOURCES = stardict_dict_update.cpp

fest2dict_LDFLAGS = 
fest2dict_LDADD = 
fest2dict_SOURCES = fest2dict.c

testutf8_LDFLAGS = 
testutf8_LDADD = $(STARDICT_LIBS)
testutf8_SOURCES = testutf8.c

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
dictbuilder_SOURCES = dictbuilder.cpp dictbuilder-tree.h

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

bgl2txt_DEPENDENCIES = libbgl2txt.a
bgl2txt_LDFLAGS =
bgl2txt_LDADD = $(STARDICT_LIBS) -lz libbgl2txt.a
bgl2txt_SOURCES = bgl2txt.cpp

stardict_verify_LDFLAGS =
stardict_verify_LDADD = $(STARDICT_LIBS)
stardict_verify_SOURCES = stardict_verify.cpp libstardictverify.cpp libstardictverify.h libcommon.cpp libcommon.h

stardict_editor_DEPENDENCIES = libbgl2txt.a
stardict_editor_LDFLAGS =
stardict_editor_LDADD = $(STARDICT_LIBS) -lz libbgl2txt.a
stardict_editor_SOURCES = stardict-editor.cpp libtabfile.cpp libtabfile.h libcommon.cpp libcommon.h libbabylonfile.cpp libbabylonfile.h libstardict2txt.cpp libstardict2txt.h libstardictverify.cpp libstardictverify.h

EXTRA_DIST = example.ifo example_treedict.tar.bz2 hanzim2dict.py jm2stardict.py uyghur2dict.py ncce2stardict.pl parse-oxford.perl dictgen.php DeKDic.exe KSDrip.exe Po2Tab.zip dictd2dic.sh ooo.sh myspell.sh exc.sh i2e.sh dictbuilder.readme dictbuilder.example KangXiZiDian-djvu2tiff.py \
extractKangXi.py makevietdict.py mkguangyunst.py stmerge.py lingea-trd-decoder.py
