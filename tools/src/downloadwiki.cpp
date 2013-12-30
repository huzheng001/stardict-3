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
    if /*(strcmp(wiki, "enwiki")==0 || strcmp(wiki, "dewiki")==0 || strcmp(wiki, "frwiki")==0 || strcmp(wiki, "itwiki")==0 || strcmp(wiki, "jawiki")==0 ||*/ // File too big.
strcmp(wiki,"wikidatawiki")==0)  // Not very useful without further parsing
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
        url = "http://dumps.wikimedia.org/";
        url += wiki;
        url += '/';
        url += date;
        url += '/';
        url += filename;
        command = "wget -c "; // -c: continue dl of partial files
        //command = "wget -nc "; // -nc: don't re-download (g_file_test does this?)
        command += url;
	int result;
        result = system(command.c_str());
	if (result == -1) {
		g_print("system() error!\n");
	}
	if (!g_file_test(filename.c_str(), G_FILE_TEST_EXISTS)) {
	    printf("Download %s failed!\n", filename.c_str());
	    return;
	}
    }
    return; // Don't autoconvert for now / J. Korhonen
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
	int result;
        result = system(command.c_str());
	if (result == -1) {
		g_print("system() error!\n");
	}
	command = "./wikipedia ";
	command += xmlfilename;
	command += ' ';
	command += wiki;
	command += ' ';
	command += date;
	result = system(command.c_str());
	if (result == -1) {
		g_print("system() error!\n");
	}
	if (g_file_test(dictdirname.c_str(), G_FILE_TEST_EXISTS)) {
	    command = "tar -cjvf ";
	    command += dictfilename;
	    command += ' ';
	    command += dictdirname;
	    result = system(command.c_str());
	    if (result == -1) {
		g_print("system() error!\n");
	    }
	    command = "rm -rf ";
	    command += dictdirname;
	    result = system(command.c_str());
	    if (result == -1) {
		g_print("system() error!\n");
	    }
            command = "rm -f ";
            command += xmlfilename;
	    result = system(command.c_str());
	    if (result == -1) {
		g_print("system() error!\n");
	    }
        } else {
		printf("Creat %s failed!\n", dictfilename.c_str());
	}
    }
}

int main(int argc,char * argv [])
{
    //Get the lists by
    //wget http://dumps.wikimedia.org/backup-index.html
    //grep -e \"\>.\*\</a\> backup-index.html -o > wikilist
    //grep -e \</a\> backup-index.html | grep -e 201.[0123456789]... -o > datelist
    //then replace and edit with vi.
    //the strings should be end with a space.
    char *wikilist = strdup("ltwiki bswiki specieswiki scnwiki fywiktionary sowiktionary angwikiquote vecwikisource brwikiquote yiwiktionary angwikibooks frwikiquote brwikisource pflwiki pdcwiki xhwiktionary pntwiki gagwiki bmwiktionary glwikisource wikimania2005wiki pa_uswikimedia vowikibooks akwiki akwiktionary svwikiversity zuwikibooks eowikinews kbdwiki eowikisource ltgwiki liwikibooks sahwikisource sqwikibooks azwikisource cebwiki srwikinews ocwiki azwiki rowiktionary plwiktionary lowiktionary pamwiki sqwikinews zh_min_nanwikisource cvwikibooks suwikibooks wawikibooks tewiktionary itwikiquote rowikisource mlwiki smwiktionary afwiktionary ruwikinews ikwiki gawikiquote gawikibooks testwiki sawiki eowikiquote slwiktionary wowiktionary yowiktionary gnwikibooks cowiki suwiktionary bhwiki bowiki lmowiki hakwiki zeawiki aywiki enwikinews kabwiki chrwiki slwikisource lawiki svwikiquote idwikiquote ltwikibooks nahwiktionary svwikinews azwikibooks xhwikibooks snwiktionary rwwiki lvwikibooks akwikibooks tetwiki ltwikisource ikwiktionary nlwiktionary itwiktionary simplewiki siwiki minwiki nnwiki idwiktionary aywiktionary sawiktionary fywikibooks tawikibooks angwikisource vowiki ruwikiquote kowikiversity plwikivoyage loginwiki rowikivoyage sawikiquote be_x_oldwiki afwiki zhwikisource mznwiki ltwikiquote avwiki rowikinews sawikibooks nahwikibooks rwwiktionary lbwiktionary kjwiki ptwiktionary lnwiki iewiktionary gawiktionary dewikibooks glkwiki crwiktionary bmwiki lnwiktionary tlwiki iswiki sqwiki cywiki mrwiki bawiki eswikisource bat_smgwiki thwikisource gnwiki igwiki lnwikibooks kkwikibooks bgwikisource tgwikibooks arzwiki ptwikivoyage nawikibooks aswikisource alswikibooks lbwiki eswikivoyage elwikinews aswiktionary bmwikibooks csbwiktionary dewikiquote kiwiki bgwikibooks liwiktionary wikimania2013wiki idwikisource bpywiki bnwikisource ruwiki ukwiki metawiki arwiki fiwiki ruwikibooks barwiki napwiki tenwiki xhwiki trwiktionary thwiktionary arwiktionary mlwiktionary kuwikibooks bewiktionary gdwiktionary fiwikinews rowikiquote tewikiquote wowiki lgwiki tumwiki roa_rupwiktionary zh_classicalwiki zhwikinews azwiktionary lowiki skwikiquote cowikibooks kgwiki smwiki bawikibooks mrwiktionary nycwikimedia slwikiquote slwikibooks lawikisource wikimania2012wiki warwiki simplewiktionary pnbwiki wuuwiki azwikiquote lijwiki kwwiki chowiki bgwikiquote iewiki zh_min_nanwiktionary skwiktionary skwikibooks mkwikisource bxrwiki kvwiki lbwikibooks iswikiquote skwikisource iewikibooks bgwikinews kawiki etwiki lvwiki eswiktionary bnwiki shwiki dewikinews tewiki bewiki fiwiktionary zh_yuewiki kowiktionary csbwiki sgwiki siwikibooks thwikinews tawikinews aywikibooks avwiktionary bswikinews dewikivoyage lbewiki sourceswiki brwiktionary kuwiki guwikisource bewikisource ckbwiki ukwikimedia kuwiktionary kywiki lezwiki cuwiki lawiktionary kuwikiquote mwlwiki slwikiversity kywiktionary cswikisource wikimania2014wiki svwikibooks viwiktionary cswiktionary sewiki zh_min_nanwikibooks thwikiquote cawikiquote ptwikisource bugwiki bswikibooks arwikiquote tgwiktionary cewiki bswikiquote lawikibooks kywikibooks ndswiki snwiki bowiktionary scwiki wowikiquote kawikiquote iuwiktionary shwiktionary sdwiki knwiki anwiki cowiktionary iswikisource xalwiki afwikiquote ruewiki bswikisource newiki ladwiki cawikinews bnwiktionary kawikibooks mkwikibooks crwikiquote gotwikibooks bnwikibooks kwwikiquote trwikinews jvwiki acewiki pawiktionary krcwiki mkwiktionary biwikibooks sgwiktionary sdwikinews stwiktionary miwiki scwiktionary mdfwiki tewikisource cawiki trwiki nowiki frwikibooks hewikisource dewikiversity cawikisource thwikibooks ruwikimedia scnwiktionary uawikimedia fywiki cowikimedia iuwiki kswiktionary kywikiquote foundationwiki alswiki liwikiquote newiktionary newwiki liwikisource bowikibooks astwikibooks mrwikisource miwikibooks fowiki jawikibooks hewikivoyage pmswiki ukwikivoyage fiwikisource ttwiki scowiki diqwiki arwikibooks itwikiversity frwikinews mlwikisource tkwiki itwikivoyage mgwiki jawiktionary betawikiversity hywikisource jvwiktionary cvwiki vepwiki papwiki bswiktionary test2wiki pnbwiktionary cswikibooks bewikimedia yowiki cswikiquote jawikiquote hsbwiktionary lbwikiquote biwiktionary kwwiktionary towiki biwiki sewikibooks jawikisource mlwikiquote dvwiki zh_min_nanwiki cowikiquote chywiki tkwiktionary jbowiktionary bhwiktionary bewikiquote frpwiki simplewikibooks fawiki kowiki hywiki nahwiki mywiktionary amwiki cawiktionary towiktionary miwiktionary oswiki tgwiki kshwiki mywiki chwiki frwikiversity szlwiki bewikibooks aawikibooks svwikivoyage chwiktionary hawiktionary tnwiki trwikisource dewiki eswiki knwiktionary tawiktionary mhwiktionary chwikibooks tlwikibooks qualitywiki viwikivoyage cywikisource cbk_zamwiki alswikiquote zawiktionary cywiktionary quwikibooks fowikisource chrwiktionary urwiktionary tlwiktionary cdowiki guwiki tewikibooks amwiktionary tkwikibooks fiwikimedia gnwiktionary kawiktionary dvwiktionary tyvwiki wikimania2011wiki cywikiquote mlwikibooks ndswikiquote ndswikibooks srwiktionary newikibooks mnwiki wikimania2007wiki elwikiquote mhwiki amwikiquote aawiki plwikimedia alswiktionary afwikibooks quwiktionary elwikisource dawiktionary frwikivoyage ruwikivoyage nds_nlwiki eswikinews mtwiki gvwiki pswiktionary trwikibooks sswiki zawiki nlwikivoyage abwiki urwikibooks urwikiquote nowikisource vowikiquote mswiktionary abwiktionary fawikisource cywikibooks tpiwiktionary jawikinews etwiktionary testwikidatawiki urwiki htwiki stqwiki tpiwiki srwiki nlwiki cswiki elwikibooks gdwiki dawikisource nawiki udmwiki zhwikiquote frwiktionary pawiki outreachwiki eewiki srnwiki gotwiki glwiktionary tnwiktionary quwiki hawiki ugwiki ptwikiversity nowiktionary commonswiki thwiki kmwiki zhwikibooks hewiktionary yiwikisource nawikiquote eowiktionary cawikibooks mgwikibooks rswikimedia dzwiki tkwikiquote orwiki eswikiversity ukwikibooks hiwiktionary tiwiktionary yowikibooks dawikiquote nawiktionary tiwiki guwikiquote ttwikibooks map_bmswiki advisorywiki ukwikinews ukwikiquote ptwikibooks mywikibooks simplewikiquote wikimania2009wiki trwikimedia ugwiktionary euwikiquote ocwiktionary ugwikibooks ugwikiquote etwikibooks rowiki iawiki crhwiki furwiki zuwiki muswiki mtwiktionary kswikibooks mswikibooks mrwikibooks kswikiquote krwikiquote dawikibooks etwikimedia wikimania2010wiki iswiktionary cswikiversity dzwiktionary zawikiquote wikidatawiki zhwiki itwiki plwikisource vlswiki emlwiki lawikiquote kswiki crwiki mrwikiquote mowiktionary bgwiki krwiki knwikisource suwiki fawikiquote elwiki enwikisource skwiki mswiki zhwiktionary frwiki frwikisource tawiki mgwiktionary enwikiquote nlwikibooks eswikiquote eswikibooks jbowiki votewiki sowiki zh_min_nanwikiquote kowikibooks omwiktionary kowikiquote bgwiktionary kkwiki dewiktionary huwikibooks omwiki eowikibooks nowikibooks mowiki pagwiki ngwiki sdwiktionary ocwikibooks kaawiki siwiktionary usabilitywiki mnwiktionary sewikimedia kmwiktionary cswikinews mnwikibooks knwikiquote klwiktionary hifwiki aawiktionary wikimania2008wiki hiwikiquote orwiktionary hewikiquote uzwiki kmwikibooks sahwiki itwikisource brwiki itwikinews incubatorwiki astwiki ukwiktionary gawiki fawiktionary elwikivoyage guwikibooks donatewiki vecwiktionary hsbwiki svwiktionary arwikisource nrmwiki dsbwiki novwiki nostalgiawiki nvwiki klwiki roa_rupwiki nowikinews lvwiktionary ganwiki huwikinews fiu_vrowiki kkwiktionary hrwikisource ukwikisource etwikisource pawikibooks nzwikimedia etwikiquote glwikiquote ptwikiquote glwikibooks ptwikinews hrwiktionary hiwikibooks aswiki brwikimedia rmwiki nlwikisource swwiktionary pcdwiki fiwikiversity dkwikimedia euwiktionary nywiki tywiki huwikisource fiwikiquote tawikisource jawiki hrwiki astwiktionary fjwiki nowikiquote rnwiki ltwiktionary jawikiversity ffwiki rmywiki hrwikiquote myvwiki pswiki sqwiktionary hawwiki nlwikinews huwiktionary fiwikibooks iowiki swwiki arwikinews wikimania2006wiki idwikibooks viwikibooks ndswiktionary hrwikibooks gvwiktionary uzwikibooks aswikibooks srwikibooks zawikibooks knwikibooks hywiktionary vewikimedia bdwikimedia vewiki pswikibooks fowiktionary sswiktionary kkwikiquote uzwikiquote guwiktionary rmwikibooks idwiki enwiki hewiki ptwiki enwiktionary svwiki glwiki huwiki plwiki viwiki euwiki mediawikiwiki dawiki iowiktionary mhrwiki huwikiquote rowikibooks tswiki quwikiquote stwiki rmwiktionary vecwiki nlwikiquote hewikibooks uzwiktionary nlwikimedia euwikibooks trwikiquote angwiki hywikiquote ttwiktionary viwikiquote viwikisource roa_tarawiki enwikibooks hiwiki slwiki ruwiktionary ruwikisource plwikiquote yiwiki wawiktionary itwikibooks fawikibooks nnwiktionary twwiki twwiktionary tswiktionary elwikiversity nnwikiquote howiki hewikinews htwikisource piwiki bclwiki iawikibooks ttwikiquote fawikinews frrwiki wawiki hzwiki plwikibooks hywikibooks kowikisource nowikimedia kowikinews mkwikimedia iiwiki liwiki rnwiktionary enwikivoyage mkwiki enwikiversity ruwikiversity svwikisource strategywiki pihwiki arcwiki eowiki astwikiquote plwikinews xmfwiki bjnwiki koiwiki srwikisource srwikiquote sqwikiquote ilowiki angwiktionary mrjwiki piwiktionary iswikibooks extwiki anwiktionary arwikiversity swwikibooks fjwiktionary zuwiktionary tawikiquote arwikimedia sawikisource mxwikimedia suwikiquote nsowiki vowiktionary bmwikiquote iawiktionary dewikisource elwiktionary ");
    char *datelist = strdup("20131216 20131216 20131216 20131216 20131216 20131216 20131216 20131216 20131216 20131216 20131216 20131216 20131216 20131216 20131216 20131216 20131216 20131216 20131216 20131216 20131216 20131216 20131216 20131216 20131216 20131216 20131216 20131216 20131216 20131216 20131216 20131216 20131216 20131216 20131215 20131215 20131215 20131215 20131215 20131215 20131215 20131215 20131215 20131215 20131215 20131215 20131215 20131215 20131215 20131215 20131215 20131215 20131215 20131215 20131215 20131215 20131215 20131215 20131215 20131215 20131215 20131215 20131215 20131215 20131215 20131215 20131215 20131215 20131215 20131215 20131215 20131215 20131215 20131215 20131215 20131215 20131215 20131215 20131215 20131215 20131215 20131215 20131215 20131215 20131215 20131215 20131215 20131215 20131215 20131215 20131215 20131215 20131215 20131215 20131215 20131215 20131215 20131215 20131215 20131215 20131215 20131215 20131215 20131215 20131215 20131215 20131215 20131215 20131215 20131215 20131215 20131215 20131215 20131215 20131215 20131215 20131215 20131215 20131215 20131215 20131215 20131215 20131215 20131215 20131215 20131215 20131215 20131215 20131215 20131215 20131215 20131215 20131215 20131215 20131215 20131214 20131214 20131214 20131214 20131214 20131214 20131214 20131214 20131214 20131214 20131214 20131214 20131214 20131214 20131214 20131214 20131214 20131214 20131214 20131214 20131214 20131214 20131214 20131214 20131214 20131214 20131214 20131214 20131214 20131214 20131213 20131213 20131213 20131213 20131213 20131213 20131213 20131213 20131213 20131213 20131213 20131213 20131213 20131213 20131213 20131213 20131213 20131213 20131213 20131213 20131213 20131213 20131213 20131213 20131213 20131213 20131213 20131213 20131213 20131213 20131213 20131213 20131213 20131213 20131213 20131213 20131213 20131213 20131213 20131213 20131213 20131213 20131213 20131213 20131213 20131213 20131213 20131213 20131213 20131213 20131213 20131213 20131213 20131213 20131213 20131213 20131213 20131213 20131213 20131213 20131213 20131213 20131213 20131213 20131213 20131213 20131213 20131213 20131213 20131213 20131213 20131213 20131213 20131213 20131213 20131213 20131213 20131213 20131213 20131213 20131213 20131213 20131213 20131213 20131213 20131213 20131213 20131213 20131213 20131213 20131213 20131213 20131213 20131213 20131213 20131213 20131213 20131213 20131213 20131213 20131213 20131212 20131212 20131212 20131212 20131212 20131212 20131212 20131212 20131212 20131212 20131212 20131212 20131212 20131212 20131212 20131212 20131212 20131212 20131212 20131212 20131212 20131212 20131212 20131212 20131212 20131212 20131212 20131212 20131212 20131212 20131212 20131212 20131212 20131212 20131212 20131212 20131212 20131212 20131212 20131212 20131212 20131212 20131212 20131212 20131212 20131212 20131212 20131212 20131212 20131212 20131212 20131212 20131212 20131212 20131212 20131212 20131211 20131211 20131211 20131211 20131211 20131211 20131211 20131211 20131211 20131211 20131211 20131211 20131211 20131211 20131211 20131211 20131211 20131211 20131211 20131211 20131211 20131211 20131211 20131211 20131211 20131211 20131211 20131211 20131211 20131211 20131211 20131211 20131211 20131211 20131211 20131211 20131211 20131211 20131211 20131211 20131211 20131211 20131211 20131211 20131211 20131211 20131211 20131211 20131211 20131211 20131211 20131211 20131211 20131211 20131211 20131211 20131211 20131211 20131211 20131211 20131211 20131211 20131211 20131211 20131211 20131211 20131211 20131211 20131211 20131211 20131211 20131211 20131211 20131211 20131211 20131211 20131210 20131210 20131210 20131210 20131210 20131210 20131210 20131210 20131210 20131210 20131210 20131210 20131210 20131210 20131210 20131210 20131210 20131210 20131210 20131210 20131210 20131210 20131210 20131209 20131209 20131209 20131209 20131209 20131209 20131209 20131209 20131209 20131209 20131209 20131209 20131209 20131209 20131209 20131209 20131209 20131209 20131209 20131209 20131209 20131209 20131209 20131209 20131209 20131209 20131209 20131209 20131209 20131209 20131209 20131209 20131209 20131209 20131209 20131209 20131209 20131209 20131209 20131209 20131209 20131209 20131209 20131209 20131209 20131209 20131209 20131209 20131209 20131209 20131209 20131209 20131209 20131209 20131209 20131209 20131209 20131209 20131209 20131209 20131209 20131209 20131209 20131209 20131209 20131209 20131209 20131209 20131209 20131209 20131209 20131209 20131209 20131209 20131209 20131209 20131208 20131208 20131208 20131208 20131208 20131208 20131208 20131208 20131208 20131207 20131207 20131207 20131207 20131207 20131207 20131207 20131207 20131207 20131207 20131207 20131207 20131207 20131206 20131206 20131206 20131206 20131206 20131206 20131206 20131206 20131206 20131206 20131206 20131206 20131206 20131206 20131206 20131206 20131206 20131206 20131206 20131206 20131206 20131206 20131206 20131206 20131206 20131206 20131206 20131206 20131206 20131206 20131206 20131206 20131206 20131206 20131206 20131206 20131206 20131206 20131206 20131206 20131206 20131206 20131206 20131206 20131206 20131206 20131206 20131206 20131206 20131206 20131206 20131206 20131206 20131206 20131206 20131206 20131206 20131206 20131205 20131205 20131205 20131205 20131205 20131205 20131205 20131205 20131205 20131205 20131205 20131205 20131205 20131205 20131205 20131205 20131205 20131205 20131205 20131205 20131205 20131204 20131204 20131204 20131204 20131204 20131204 20131204 20131204 20131204 20131204 20131204 20131204 20131204 20131204 20131204 20131204 20131204 20131204 20131204 20131204 20131204 20131204 20131204 20131204 20131204 20131204 20131204 20131204 20131204 20131204 20131204 20131204 20131204 20131204 20131204 20131204 20131204 20131204 20131204 20131204 20131204 20131204 20131204 20131204 20131204 20131204 20131204 20131204 20131204 20131204 20131204 20131204 20131204 20131204 20131204 20131204 20131204 20131204 20131204 20131204 20131204 20131204 20131204 20131204 20131204 20131204 20131204 20131204 20131204 20131204 20131204 20131204 20131204 20131204 20131204 20131204 20131204 20131204 20131204 20131204 20131204 20131204 20131204 20131204 20131204 20131204 20131204 20131204 20131204 20131204 20131204 20131204 20131204 20131204 20131204 20131204 20131204 20131204 20131204 20131204 20131203 20131203 20131203 20131203 20131203 20131203 20131203 20131203 20131203 20131203 20131203 20131203 20131203 20131203 20131203 20131203 20131203 20131203 20131203 20131203 20131203 20131203 20131203 20131203 20131203 20131203 20131203 20131203 20131203 20131203 20131203 20131203 20131203 20131203 20131203 20131203 20131203 20131203 20131203 20131203 20131203 20131203 20131203 20131203 20131202 20131202 20131202 20131202 20131202 20131202 20131201 20131201 20131201 20131201 20131201 20131201 20131201 20131201 20131201 20131201 20131201 20131201 20131201 20131201 20131201 20131201 20131201 20131201 20131201 20131201 20131201 20131201 20131201 20131201 20131201 20131201 20131201 20131201 20131201 20131201 20131201 20131201 20131201 20131201 20131201 20131201 20131201 20131201 20131201 20131201 20131201 20131201 20131201 20131201 20131201 20131201 20131201 20131201 20131201 20131201 20131201 20131201 20131201 20131201 20131201 20131201 20131201 20131201 20131201 20131201 20131201 20131201 20131201 20131130 20131130 20131130 20131130 20131130 20131130 20131130 20131130 20131130 20131130 20131130 20131130 20131130 20131130 20131130 20131130 20131130 20131130 20131130 20131130 20131130 20131130 20131130 20131130 20131130 20131130 20131130 20131130 20131130 20131130 20131130 20131130 20131130 20131130 20131130 20131130 20131130 20131130 20131130 ");
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
