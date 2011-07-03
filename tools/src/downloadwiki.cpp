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
#include <glib.h>

#include <string>

void download(char *wiki, char *date)
{
    if (strcmp(wiki, "enwiki")==0 || strcmp(wiki, "dewiki")==0 || strcmp(wiki, "frwiki")==0 || strcmp(wiki, "itwiki")==0 || strcmp(wiki, "jawiki")==0) // File too big.
        return;
    std::string filename;
    filename = wiki;
    filename += '-';
    filename += date;
    filename += "-pages-articles.xml.bz2";
    std::string command;
    if (!g_file_test(filename.c_str(), G_FILE_TEST_EXISTS)) {
        printf("Downloading %s\n", filename.c_str());
        std::string url;
        url = "http://download.wikipedia.org/";
        url += wiki;
        url += '/';
        url += date;
        url += '/';
        url += filename;
        command = "wget -c ";
        command += url;
        system(command.c_str());
	if (!g_file_test(filename.c_str(), G_FILE_TEST_EXISTS)) {
	    printf("Download %s failed!\n", filename.c_str());
	    return;
	}
    }
    std::string dictdirname;
    dictdirname = "stardict-wikipedia-";
    dictdirname += wiki;
    dictdirname += "-2.4.2";
    std::string dictfilename;
    dictfilename = dictdirname + ".tar.bz2";
    if (!g_file_test(dictfilename.c_str(), G_FILE_TEST_EXISTS)) {
        printf("Creating %s\n", dictfilename.c_str());
	std::string xmlfilename;
	xmlfilename = wiki;
	xmlfilename += '-';
	xmlfilename += date;
	xmlfilename += "-pages-articles.xml";
        command = "bzcat ";
        command += filename;
	command += " > ";
	command += xmlfilename;
        system(command.c_str());
	command = "./wikipedia ";
	command += xmlfilename;
	command += ' ';
	command += wiki;
	command += ' ';
	command += date;
	system(command.c_str());
	if (g_file_test(dictdirname.c_str(), G_FILE_TEST_EXISTS)) {
	    command = "tar -cjvf ";
	    command += dictfilename;
	    command += ' ';
	    command += dictdirname;
	    system(command.c_str());
	    command = "rm -rf ";
	    command += dictdirname;
	    system(command.c_str());
            command = "rm -f ";
            command += xmlfilename;
	    system(command.c_str());
        } else {
		printf("Creat %s failed!\n", dictfilename.c_str());
	}
    }
}

int main(int argc,char * argv [])
{
    //Get the lists by
    //wget http://download.wikipedia.org/backup-index.html
    //grep -e \"\>.\*\</a\> backup-index.html -o > wikilist
    //grep -e \</a\> backup-index.html | grep -e 200.[0123456789]... -o > datelist
    //then replace and edit with vi.
    //the strings should be end with a space.
    char *wikilist = strdup("enwiktionary zhwiki xhwiktionary xhwikibooks fiwiki xhwiki wuuwiki wowiktionary wowiki wikimania2005wiki warwiki wawiktionary wawikibooks vowiktionary vowiki plwiktionary vowikibooks vlswiki snwiki viwikibooks vecwiki viwikisource viwikiquote thwiki ptwiki uzwiki vewiki urwiki uzwiktionary uzwikiquote uzwikibooks urwiktionary urwikiquote urwikibooks ukwiktionary ukwikisource ltwiki ukwikiquote ukwikinews frwiktionary trwiktionary ukwikibooks ugwiktionary ugwikiquote ugwikibooks ugwiki udmwiki tywiki idwiki twwiktionary twwiki tumwiki ttwiktionary ttwikiquote ttwikibooks ttwiki tswiktionary tswiki iowiktionary trwikisource hrwiki trwikiquote trwikibooks tpiwiktionary tpiwiki towiktionary tokiponawikibooks towiki tokiponawiktionary tokiponawikiquote tlwikibooks tokiponawiki tnwiktionary tnwiki tlwiktionary tlwiki tkwiktionary tlhwiktionary tlhwiki commonswiki tkwikiquote tkwikibooks tkwiki tiwiktionary tiwiki thwiktionary thwikisource thwikiquote thwikinews thwikibooks viwiktionary tgwiki tewikibooks tgwiktionary tgwikibooks tawiki tewiktionary tewikisource tewikiquote tewiki specieswiki testwiki tetwiki tawiktionary svwiktionary tawikiquote tawikinews tawikibooks ruwiktionary swwiktionary zh_yuewiki swwikibooks zh_min_nanwiktionary swwiki be_x_oldwiki zh_min_nanwikisource svwikisource svwikiquote zh_min_nanwikibooks svwikinews svwikibooks zawiktionary suwiktionary suwikiquote suwiki zawiki suwikibooks sqwiki yiwiktionary stwiktionary stwiki wikimania2006wiki sswiktionary sswiki srwiktionary xalwiki etwiki enwiki srwikisource wowikiquote srwikiquote srwikinews wikimania2007wiki srwikibooks sqwiktionary sqwikiquote wawiki sqwikibooks vowikiquote sowiktionary sowiki simplewiki snwiktionary smwiktionary slwiktionary smwiki slwikisource slwikiquote skwikisource slwikibooks skwiktionary skwikiquote simplewiktionary skwikibooks siwiktionary siwikibooks siwiki shwiki simplewikiquote simplewikibooks shwiktionary sgwiktionary sgwiki sewiki sewikibooks sep11wiki sdwiktionary sdwiki scowiki sdwikinews scwiktionary itwiki bnwikisource hsbwiktionary mrwiki scwiki scnwiki scnwiktionary ruwikibooks sawiktionary mrwiktionary sawiki sawikibooks rwwiktionary ruwikisource rwwiki ptwiktionary ruwikiquote rowikisource ruwikinews rowiktionary rowikiquote rowikinews dewiki roa_tarawiki rowikibooks roa_rupwiki roa_rupwiktionary rnwiktionary rmywiki rnwiki quwiki rmwiktionary rmwikibooks ptwikisource rmwiki quwiktionary quwikiquote quwikibooks ptwikibooks ptwikiquote plwikisource ptwikinews enwikiquote pswiki nnwiki pswiktionary pswikibooks plwikiquote pmswiki plwikinews plwikimedia plwikibooks pamwiki piwiktionary pdcwiki piwiki pihwiki ocwiki pawiktionary pawikibooks pawiki papwiki nlwiktionary pagwiki oswiki orwiktionary orwiki omwiktionary omwiki ocwiktionary ocwikibooks nrmwiki nywiki nzwikimedia nowiktionary nvwiki nowikiquote nowikisource nowikinews novwiki nowikibooks nlwikibooks nostalgiawiki nnwiktionary nnwikiquote nlwikisource nlwikiquote newwiki nlwikinews nlwikimedia ndswiktionary ngwiki newiktionary kowiki newikibooks newiki ndswiki ndswikiquote ndswikibooks napwiki nds_nlwiki nawiktionary nawikiquote nahwiki nawikibooks nawiki nahwiktionary mswiki nahwikibooks mznwiki mywiktionary jvwiki mywikibooks jbowiki mtwiki mywiki muswiki mtwiktionary mswiktionary mrwikiquote mswikibooks mlwiki mrwikibooks mowiki mowiktionary mnwiki mnwiktionary mnwikibooks mlwiktionary mkwiki mlwikisource mlwikiquote mlwikibooks lbwiki mkwiktionary mkwikisource mkwikibooks miwiki miwiktionary miwikibooks mhwiktionary mhwiki mgwiktionary mgwiki mgwikibooks mediawikiwiki lvwiki map_bmswiki lvwiktionary lvwikibooks ltwiktionary ltwikisource ltwikibooks ltwikiquote lowiktionary lmowiki lowiki lnwiktionary lnwikibooks lnwiki kshwiki liwiktionary liwiki lijwiki lgwiki lbwiktionary lbwikiquote lbwikibooks lawiktionary lbewiki lawikisource lawiki lawikiquote lawikibooks kawiki ladwiki kywiktionary kywikiquote kywikibooks kywiki kwwiktionary kwwikiquote kwwiki kvwiki kuwiktionary kuwikiquote kuwikibooks kuwiki kswiktionary kswikiquote kswikibooks kswiki kowiktionary krwikiquote krwiki kowikisource knwiki kowikiquote kowikibooks eowiki knwiktionary knwikisource knwikiquote knwikibooks kkwiki kmwiktionary kmwikibooks kmwiki klwiktionary klwiki kkwiktionary kkwikiquote kkwikibooks wikimania2008wiki enwikibooks kjwiki kiwiki kgwiki kawiktionary kawikiquote kawikibooks jawiktionary jvwiktionary jbowiktionary jawikisource jawikiquote jawikinews jawikibooks cswiki enwikinews advisorywiki dawiki dewikinews frwikisource dewiktionary itwikisource iswiki itwiktionary ruwiki itwikiquote iuwiktionary iuwiki itwikibooks huwiktionary itwikinews glwiki iswiktionary iswikisource iswikiquote iowiki iswikibooks incubatorwiki iawiktionary eowikiquote ilowiki ikwiktionary ikwiki iiwiki iewiktionary igwiki idwiktionary iewikibooks iewiki hywiki idwikisource idwikiquote idwikibooks iawikibooks iawiki hywiktionary hzwiki huwikibooks hywikiquote hywikibooks huwikisource huwikiquote hrwikisource htwiki hsbwiki htwikisource hiwiki hrwiktionary nlwiki hewiktionary hrwikiquote hrwikibooks howiki hiwiktionary hiwikiquote hiwikibooks bgwiktionary hewikisource hewikiquote hewikibooks hewikinews hawwiki hawiktionary hawiki gvwiktionary guwiki gvwiki guwiktionary guwikiquote guwikibooks gotwikibooks gotwiki gnwiktionary cawiki gnwikibooks gnwiki glwiktionary glwikisource glwikiquote glwikibooks gawiki glkwiki gdwiktionary gdwiki gawiktionary gawikiquote gawikibooks fywiktionary fywiki fywikibooks frwikiversity furwiki enwikisource bgwiki frwikiquote fiwiktionary frwikinews frwikibooks frpwiki fowiktionary fowikisource fawiki fowiki foundationwiki fjwiktionary fjwiki fiwikibooks fiwikisource fiwikiquote fiu_vrowiki ffwiki fawiktionary fawikisource fawikiquote fawikibooks eswikisource euwiki frwiki euwiktionary euwikiquote euwikibooks pa_uswikimedia etwiktionary etwikisource etwikiquote etwikibooks eswiktionary eswikiversity eswikiquote eswikibooks eswikinews eowiktionary enwikiversity eowikibooks arwiki eswiki rswikimedia elwiki elwiktionary emlwiki dewikisource elwikisource elwikiquote elwikibooks dzwiktionary eewiki dzwiki dewikibooks dvwiktionary dvwiki dkwiktionary dkwikibooks diqwiki dewikiversity dewikiquote cywiki dawiktionary dawikisource cvwiki dawikiquote dawikibooks cywikisource cywiktionary cywikiquote cywikibooks cswikisource cswiktionary cvwikibooks csbwiki cuwiki cswikiquote cswikibooks cowiki cebwiki csbwiktionary crwiktionary cowiktionary crwikiquote crwiki cowikiquote cowikibooks bswiki chywiki chwiktionary chwikimedia chwikibooks hewiki chwiki chrwiktionary chrwiki chowiki cewiki cdowiki cawiktionary cbk_zamwiki cawikisource cawikiquote cawikinews bswikisource cawikibooks bxrwiki bugwiki bswiktionary bpywiki bswikiquote bswikinews bswikibooks bnwiki brwiktionary bowikibooks brwiki bowiktionary bowiki tawikisource bnwiktionary bnwikibooks bmwiktionary bmwikiquote bmwiki biwiktionary bmwikibooks biwikibooks biwiki bhwiktionary bhwiki bgwikiquote bgwikibooks bgwikisource bewiktionary bgwikinews bewikiquote betawikiversity bewikibooks bat_smgwiki bawiktionary bawikibooks azwiki bawiki zhwiktionary barwiki azwiktionary azwikisource azwikiquote azwikibooks aywiktionary aywiki aywikibooks avwiktionary avwiki aswiktionary aswikibooks astwiktionary aswiki astwikiquote arwikisource astwikibooks astwiki arwiktionary anwiki arwikiquote arwikinews arwikibooks arcwiki anwiktionary angwiktionary angwikisource angwiki angwikiquote angwikibooks amwiki amwiktionary amwikiquote alswiki alswiktionary alswikiquote alswikibooks plwiki akwiktionary akwikibooks akwiki afwiktionary afwikiquote afwikibooks afwiki abwiktionary abwiki aawiktionary aawikibooks aawiki zuwiktionary zuwikibooks zuwiki zhwikisource zhwikiquote zhwikinews zhwikibooks itwikiversity svwiki trwiki ukwiki nowiki viwiki bewiki srwiki kabwiki metawiki slwiki hakwiki sourceswiki jawiki skwiki rowiki zh_min_nanwiki zh_classicalwiki zh_min_nanwikiquote huwiki zeawiki zawikiquote zawikibooks yowiktionary yowiki yowikibooks yiwikisource yiwiki ");
    char *datelist = strdup("20071120 20071014 20071120 20071120 20071119 20071119 20071119 20071119 20071119 20071119 20071119 20071119 20071119 20071119 20071119 20071119 20071119 20071119 20071119 20071119 20071119 20071119 20071119 20071119 20071116 20071119 20071119 20071119 20071119 20071119 20071119 20071119 20071119 20071119 20071119 20071119 20071119 20071119 20071119 20071119 20071119 20071119 20071119 20071119 20071119 20071119 20071119 20071119 20071119 20071119 20071119 20071119 20071119 20071119 20071119 20071118 20071118 20071118 20071118 20071118 20071118 20071118 20071118 20071118 20071118 20071118 20071118 20071118 20071118 20071118 20071118 20071118 20071118 20071118 20071118 20071118 20071118 20071118 20071118 20071118 20071118 20071118 20071118 20071118 20071118 20071118 20071118 20071118 20071118 20071118 20071118 20071118 20071118 20071118 20071118 20071118 20071118 20071118 20071118 20071118 20071118 20071118 20071118 20071118 20071118 20071118 20071118 20071118 20071117 20071117 20071117 20071117 20071117 20071117 20071117 20071117 20071117 20071117 20071117 20071117 20071117 20071117 20071117 20071117 20071117 20071117 20071117 20071117 20071117 20071117 20071117 20071117 20071117 20071117 20071117 20071117 20071117 20071018 20071116 20071116 20071116 20071116 20071116 20071116 20071116 20071116 20071116 20071116 20071116 20071116 20071116 20071116 20071116 20071116 20071116 20071116 20071116 20071116 20071116 20071116 20071116 20071116 20071116 20071116 20071116 20071116 20071116 20071116 20071116 20071116 20071116 20071116 20071116 20071116 20071116 20071116 20071116 20071116 20071116 20071116 20071116 20071115 20071115 20071115 20071115 20071115 20071114 20071114 20071114 20071114 20071114 20071114 20071114 20071114 20071114 20071114 20071114 20071114 20071114 20071114 20071114 20071114 20071114 20071112 20071114 20071114 20071114 20071114 20071114 20071114 20071114 20071114 20071114 20071114 20071114 20071114 20071114 20071114 20071114 20071114 20071114 20071114 20071114 20071114 20071114 20071114 20071114 20071114 20071114 20071114 20071114 20071114 20071114 20071114 20071114 20071114 20071114 20071114 20071114 20071114 20071114 20071114 20071114 20071114 20071114 20071114 20071114 20071114 20071114 20071114 20071114 20071114 20071114 20071114 20071114 20071113 20071113 20071113 20071113 20071113 20071113 20071113 20071113 20071113 20071113 20071113 20071113 20071113 20071113 20071113 20071113 20071113 20071113 20071113 20071113 20071113 20071113 20071113 20071113 20071113 20071113 20071113 20071113 20071113 20071113 20071113 20071113 20071113 20071113 20071113 20071113 20071113 20071113 20071113 20071113 20071113 20071113 20071113 20071113 20071113 20071113 20071113 20071113 20071113 20071113 20071113 20071113 20071113 20071113 20071113 20071113 20071113 20071113 20071113 20071113 20071113 20071113 20071113 20071113 20071113 20071113 20071113 20071113 20071113 20071113 20071113 20071113 20071113 20071113 20071113 20071113 20071113 20071113 20071113 20071113 20071113 20071113 20071113 20071113 20071113 20071113 20071113 20071113 20071113 20071113 20071113 20071113 20071113 20071113 20071112 20071112 20071112 20071112 20071112 20071112 20071112 20071112 20071112 20071112 20071112 20071112 20071112 20071112 20071112 20071112 20071112 20071112 20071112 20071112 20071112 20071112 20071112 20071112 20071112 20071112 20071112 20071112 20071112 20071112 20071112 20071112 20071112 20071112 20071112 20071112 20071112 20071112 20071112 20071112 20071112 20071112 20071112 20071112 20071112 20071112 20071112 20071112 20071112 20071112 20071112 20071112 20071112 20071112 20071112 20071112 20071112 20071112 20071112 20071112 20071111 20071111 20071111 20071111 20071111 20071110 20071110 20071110 20071110 20071110 20071107 20071107 20071107 20071107 20071107 20071107 20071107 20071107 20071107 20071107 20071107 20071107 20071107 20071107 20071107 20071107 20071107 20071107 20071107 20071107 20071107 20071107 20071107 20071107 20071107 20071107 20071107 20071107 20071107 20071107 20071107 20071107 20071107 20071107 20071107 20071107 20071107 20071107 20071106 20071106 20071106 20071106 20071106 20071106 20071104 20071104 20071104 20071104 20071104 20071104 20071104 20071104 20071103 20071103 20071103 20071103 20071103 20071103 20071103 20071103 20071103 20071103 20071103 20071103 20071103 20071103 20071103 20071103 20071103 20071103 20071103 20071103 20071103 20071103 20071103 20071103 20071103 20071103 20071103 20071103 20071103 20071103 20071103 20071103 20071103 20071103 20071103 20071103 20071103 20071102 20071102 20071102 20071102 20071102 20071102 20071102 20071102 20071102 20071102 20071102 20071102 20071102 20071102 20071102 20071102 20071102 20071102 20071102 20071102 20071102 20071102 20071102 20071102 20071026 20071102 20071102 20071102 20071026 20071026 20071026 20071026 20071026 20071026 20071026 20071026 20071026 20071026 20071026 20071026 20071026 20071025 20071024 20071024 20071023 20071023 20071023 20071023 20071023 20071023 20071023 20071023 20071023 20071023 20071023 20071023 20071023 20071023 20071023 20071023 20071023 20071023 20071023 20071023 20071023 20071023 20071023 20071023 20071023 20071023 20071023 20071023 20071023 20071023 20071023 20071023 20071023 20071023 20071023 20071023 20071023 20071023 20071023 20071023 20071023 20071023 20071023 20071023 20071023 20071023 20071023 20071023 20071023 20071022 20071022 20071022 20071022 20071022 20071022 20071022 20071022 20071022 20071022 20071022 20071022 20071022 20071022 20071022 20071022 20071022 20071022 20071022 20071022 20071022 20071022 20071022 20071022 20071022 20071022 20071022 20071022 20071022 20071022 20071022 20071022 20071022 20071022 20071022 20071022 20071022 20071022 20071022 20071022 20071022 20071022 20071022 20071022 20071022 20071022 20071022 20071022 20071022 20071022 20071022 20071022 20071022 20071022 20071022 20071022 20071022 20071022 20071022 20071022 20071022 20071022 20071022 20071022 20071022 20071022 20071022 20071022 20071022 20071022 20071022 20071022 20071022 20071022 20071022 20071022 20071022 20071022 20071022 20071022 20071022 20071022 20071022 20071022 20071022 20071022 20071022 20071022 20071022 20071022 20071021 20071021 20071021 20071021 20071021 20071021 20071021 20071021 20071021 20071021 20071021 20071021 20071021 20071021 20071021 20071021 20071021 20071021 20071021 20071021 20071021 20071020 20071019 20071019 20071019 20071018 20071018 20071018 20071018 20071018 20071018 20071018 20071018 20071013 20071018 20071018 20071018 20071018 20071018 20071017 20071017 20071017 20071017 20071017 20071017 20071017 20071017 20071017 ");
    char *p, *p1, *q, *q1;
    p1 = wikilist;
    q1 = datelist;
    while (true) {
        p = strchr(p1, ' ');
        q = strchr(q1, ' ');
        if (p && q) {
            *p = '\0';
            *q = '\0';
            download(p1, q1);
            p1 = p+1;
            q1 = q+1;
        } else {
            break;
        }
    }
    free(wikilist);
    free(datelist);
}
