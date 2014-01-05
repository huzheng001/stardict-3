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
#include <errno.h>

void download(char *wiki, char *date)
{
    if (strcmp(wiki, "enwiki")==0 || strcmp(wiki, "dewiki")==0 || strcmp(wiki, "frwiki")==0 || strcmp(wiki, "itwiki")==0 || strcmp(wiki, "jawiki")==0 || // File too big.
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
		g_print("system() error at bzcat! Errno: %d\n", errno);
        g_print("Error desc: %s\n", strerror(errno));
		return;
	}
    //return; // If you don't want to autoconvert but to run the commands "./wikipedia xxx", tar etc for each file yourself
	command = "./wikipedia ";
	command += xmlfilename;
	command += ' ';
	command += wiki;
	command += ' ';
	command += date;
	result = system(command.c_str());
	if (result == -1) {
		g_print("system() error at wikipedia! Errno: %d\n", errno);
        g_print("Error desc: %s\n", strerror(errno));
		return;
	}
	return; // seems to need some kind of sync before tar or tar may complain of changed file
	if (g_file_test(dictdirname.c_str(), G_FILE_TEST_EXISTS)) {
	    command = "tar -cjvf ";
	    command += dictfilename;
	    command += ' ';
	    command += dictdirname;
	    result = system(command.c_str());
	    if (result == -1) {
		g_print("system() error at tar! Errno: %d\n", errno);
        g_print("Error desc: %s\n", strerror(errno));
		return;
	    }
	    command = "rm -rf ";
	    command += dictdirname;
	    result = system(command.c_str());
	    if (result == -1) {
		g_print("system() error at rm pt 1! Errno: %d\n", errno);
        g_print("Error desc: %s\n", strerror(errno));
		return;
	    }
        command = "rm -f ";
        command += xmlfilename;
	    result = system(command.c_str());
	    if (result == -1) {
		g_print("system() error at rm pt 2! Errno: %d\n", errno);
        g_print("Error desc: %s\n", strerror(errno));
		return;
	    }
        } else {
		printf("Creat %s failed!\n", dictfilename.c_str());
	}
    }
}

int main(int argc,char * argv [])
{
    //Gets the lists by executing:
    // wget -N http://dumps.wikimedia.org/backup-index.html
	// grep -e \<li\>.\*complete\</span\>\</li\> -o backup-index.html | grep -e \"\>.\*\</a\> -o | colrm 1 2 | cut --delimiter=\< --fields=1 -s > wikilist
    // grep -e \<li\>.\*complete\</span\>\</li\> -o backup-index.html | grep -e \</a\> | grep -e 201.[0123456789]... -o > datelist
    std::string command;
	command = "wget -N http://dumps.wikimedia.org/backup-index.html";
	int result = 0;
    result = system(command.c_str());
	if (result == -1) {
		g_print("system() error at wget! Errno: %d\n", errno);
        g_print("Error desc: %s\n", strerror(errno));
		return 255;
	}
	
	command = "grep -e \\<li\\>.\\*complete\\</span\\>\\</li\\> -o backup-index.html | grep -e \\\"\\>.\\*\\</a\\> -o | colrm 1 2 | cut --delimiter=\\< --fields=1 -s > wikilist";
    result = system(command.c_str());
	if (result == -1) {
		g_print("system() error at grep 1! Errno: %d\n", errno);
        g_print("Error desc: %s\n", strerror(errno));
		return 255;
	}
	
	command = "grep -e \\<li\\>.\\*complete\\</span\\>\\</li\\> -o backup-index.html | grep -e \\</a\\> | grep -e 201.[0123456789]... -o > datelist";
    result = system(command.c_str());
	if (result == -1) {
		g_print("system() error at grep 2! Errno: %d\n", errno);
        g_print("Error desc: %s\n", strerror(errno));
		return 255;
	}
	
	//return 0;
	
	// for now, copy the results from the files wikilist and datelist to the strings below. The strings should be ended with a space.
    char *wikilist = strdup("mgwiktionary mswiki enwikiquote zhwiktionary tawiki bgwiktionary dewiktionary eswikiquote kkwiki nlwikibooks eswikibooks uzwiki jbowiki votewiki sowiki zh_min_nanwikiquote kowikibooks omwiktionary kowikiquote itwikisource hrwiki huwikibooks omwiki nowikibooks eowikibooks mowiki pagwiki ngwiki sdwiktionary kaawiki ocwikibooks usabilitywiki siwiktionary sewikimedia mnwiktionary kmwiktionary hifwiki cswikinews mnwikibooks knwikiquote klwiktionary wikimania2008wiki aawiktionary hewikiquote hiwikiquote orwiktionary incubatorwiki sahwiki kmwikibooks brwiki itwikinews astwiki gawiki svwiktionary ukwiktionary fawiktionary arwikisource elwikivoyage guwikibooks donatewiki vecwiktionary hsbwiki nrmwiki novwiki dsbwiki nostalgiawiki ganwiki nvwiki klwiki roa_rupwiki fiu_vrowiki nowikinews lvwiktionary huwikinews ukwikisource kkwiktionary ptwikiquote hrwikisource etwikisource pawikibooks nzwikimedia etwikiquote glwikiquote ptwikinews glwikibooks aswiki hrwiktionary hiwikibooks rmwiki brwikimedia nlwikisource swwiktionary euwiktionary pcdwiki fiwikiversity dkwikimedia nywiki tywiki ltwiktionary huwikisource fiwikiquote tawikisource astwiktionary fjwiki nowikiquote rnwiki huwiktionary jawikiversity ffwiki rmywiki hrwikiquote myvwiki pswiki sqwiktionary hawwiki nlwikinews fiwikibooks swwiki iowiki arwikinews idwikibooks wikimania2006wiki viwikibooks ndswiktionary hrwikibooks gvwiktionary uzwikibooks aswikibooks srwikibooks zawikibooks knwikibooks hywiktionary vewikimedia bdwikimedia vewiki pswikibooks fowiktionary sswiktionary kkwikiquote uzwikiquote guwiktionary rmwikibooks idwiki enwiktionary svwiki huwiki viwiki dawiki glwiki euwiki enwikivoyage enwikibooks mediawikiwiki iowiktionary hiwiki ruwiktionary vecwiki mhrwiki huwikiquote rowikibooks tswiki quwikiquote stwiki rmwiktionary hewikibooks nlwikiquote uzwiktionary nlwikimedia angwiki euwikibooks trwikiquote hywikiquote viwikisource ttwiktionary viwikiquote roa_tarawiki mkwiki slwiki ruwikisource eowiki dewikisource plwikiquote yiwiki itwikibooks wawiktionary fawikibooks nnwiktionary twwiki elwikiversity twwiktionary tswiktionary hewikinews nnwikiquote howiki htwikisource bclwiki piwiki fawikinews enwikiversity iawikibooks ttwikiquote wawiki frrwiki elwiktionary hzwiki plwikibooks hywikibooks kowikisource nowikimedia kowikinews mkwikimedia iiwiki liwiki rnwiktionary svwikisource ruwikiversity strategywiki pihwiki arcwiki ltwiki cebwiki plwikinews xmfwiki astwikiquote koiwiki bjnwiki srwikisource ilowiki srwikiquote mrjwiki sqwikiquote extwiki angwiktionary iswikibooks piwiktionary fjwiktionary anwiktionary arwikiversity swwikibooks bswiki sawikisource zuwiktionary tawikiquote arwikimedia vowiktionary mxwikimedia suwikiquote nsowiki bmwikiquote iawiktionary specieswiki scnwiki frwikiquote fywiktionary sowiktionary vecwikisource angwikiquote brwikiquote yiwiktionary brwikisource angwikibooks pflwiki pdcwiki gagwiki pntwiki xhwiktionary glwikisource bmwiktionary wikimania2005wiki pa_uswikimedia vowikibooks akwiki svwikiversity akwiktionary eowikinews kbdwiki zuwikibooks azwiki eowikisource ltgwiki liwikibooks sqwikibooks sahwikisource srwikinews azwikisource plwiktionary mlwiki ocwiki rowiktionary itwikiquote lowiktionary pamwiki sqwikinews zh_min_nanwikisource cvwikibooks suwikibooks wawikibooks tewiktionary rowikisource simplewiki ruwikinews enwikinews smwiktionary afwiktionary ikwiki sawiki gawikiquote gawikibooks testwiki lawiki eowikiquote slwiktionary wowiktionary yowiktionary gnwikibooks cowiki suwiktionary bhwiki bowiki lmowiki hakwiki zeawiki aywiki slwikisource kabwiki chrwiki nlwiktionary svwikiquote idwikiquote ltwikibooks nahwiktionary svwikinews azwikibooks nnwiki xhwikibooks snwiktionary rwwiki lvwikibooks akwikibooks tetwiki ltwikisource ikwiktionary itwiktionary vowiki siwiki minwiki be_x_oldwiki idwiktionary sawiktionary aywiktionary tawikibooks fywikibooks angwikisource ruwikiquote loginwiki kowikiversity plwikivoyage rowikivoyage sawikiquote afwiki zhwikisource ptwiktionary mznwiki ltwikiquote avwiki rowikinews sawikibooks nahwikibooks lbwiktionary rwwiktionary dewikibooks kjwiki tlwiki lnwiki iewiktionary gawiktionary iswiki glkwiki crwiktionary bmwiki lnwiktionary sqwiki cywiki mrwiki eswikisource bawiki bat_smgwiki thwikisource gnwiki igwiki lnwikibooks kkwikibooks arzwiki bgwikisource tgwikibooks ptwikivoyage nawikibooks aswikisource alswikibooks lbwiki eswikivoyage elwikinews aswiktionary bmwikibooks csbwiktionary dewikiquote kiwiki bgwikibooks liwiktionary wikimania2013wiki idwikisource bpywiki bnwikisource ukwiki metawiki arwiki fiwiki warwiki ruwikibooks barwiki trwiktionary napwiki tenwiki xhwiki thwiktionary mlwiktionary arwiktionary kuwikibooks bewiktionary fiwikinews gdwiktionary zh_classicalwiki rowikiquote tewikiquote wowiki lgwiki tumwiki roa_rupwiktionary zhwikinews azwiktionary skwikiquote lowiki kgwiki cowikibooks smwiki bawikibooks mrwiktionary nycwikimedia slwikiquote slwikibooks lawikisource wikimania2012wiki simplewiktionary pnbwiki wuuwiki azwikiquote lijwiki kwwiki chowiki bgwikiquote iewiki zh_min_nanwiktionary skwiktionary skwikibooks mkwikisource bxrwiki kvwiki lbwikibooks iswikiquote skwikisource iewikibooks bgwikinews kawiki etwiki lvwiki bnwiki shwiki eswiktionary tewiki bewiki dewikinews fiwiktionary kowiktionary zh_yuewiki dewikivoyage csbwiki siwikibooks sgwiki tawikinews thwikinews aywikibooks bswikinews avwiktionary sourceswiki kuwiki lbewiki brwiktionary kuwiktionary ckbwiki guwikisource bewikisource viwiktionary ukwikimedia kywiki lezwiki cuwiki mwlwiki lawiktionary kuwikiquote cswikisource slwikiversity kywiktionary cswiktionary wikimania2014wiki svwikibooks ndswiki ptwikisource sewiki zh_min_nanwikibooks thwikiquote cawikiquote bugwiki cewiki bswikibooks arwikiquote tgwiktionary bswikiquote lawikibooks scwiki kywikibooks snwiki anwiki bowiktionary kawikiquote wowikiquote knwiki iuwiktionary shwiktionary sdwiki newiki cowiktionary iswikisource xalwiki afwikiquote ruewiki bswikisource ladwiki cawikinews bnwiktionary kawikibooks mkwikibooks crwikiquote gotwikibooks bnwikibooks kwwikiquote trwikinews jvwiki acewiki pawiktionary krcwiki mkwiktionary biwikibooks sgwiktionary sdwikinews stwiktionary miwiki scwiktionary mdfwiki tewikisource cawiki trwiki nowiki hewikisource frwikibooks dewikiversity fywiki cawikisource thwikibooks scnwiktionary ruwikimedia uawikimedia alswiki newwiki cowikimedia iuwiki kswiktionary kywikiquote foundationwiki fowiki liwikiquote newiktionary jawikibooks liwikisource bowikibooks astwikibooks mrwikisource miwikibooks pmswiki hewikivoyage fiwikisource ttwiki ukwikivoyage scowiki diqwiki arwikibooks frwikinews itwikiversity mlwikisource tkwiki itwikivoyage mgwiki jawiktionary betawikiversity hywikisource jvwiktionary cvwiki vepwiki papwiki bswiktionary test2wiki pnbwiktionary cswikibooks bewikimedia yowiki cswikiquote jawikiquote hsbwiktionary lbwikiquote biwiktionary kwwiktionary towiki biwiki sewikibooks jawikisource mlwikiquote dvwiki zh_min_nanwiki cowikiquote chywiki tkwiktionary jbowiktionary bhwiktionary bewikiquote frpwiki simplewikibooks eswiki nds_nlwiki fawiki kowiki hywiki nahwiki mywiktionary amwiki oswiki cawiktionary towiktionary miwiktionary tgwiki kshwiki mywiki frwikiversity chwiki tawiktionary szlwiki bewikibooks aawikibooks svwikivoyage chwiktionary hawiktionary tnwiki trwikisource knwiktionary mhwiktionary chwikibooks tlwikibooks viwikivoyage qualitywiki cbk_zamwiki cywikisource guwiki alswikiquote zawiktionary cywiktionary quwikibooks fowikisource chrwiktionary urwiktionary tlwiktionary cdowiki tewikibooks amwiktionary tkwikibooks fiwikimedia mnwiki gnwiktionary kawiktionary dvwiktionary tyvwiki wikimania2011wiki cywikiquote mlwikibooks ndswikiquote ndswikibooks srwiktionary newikibooks elwikisource wikimania2007wiki elwikiquote mhwiki amwikiquote aawiki plwikimedia alswiktionary afwikibooks quwiktionary frwikivoyage dawiktionary ruwikivoyage nlwiki srwiki eswikinews mtwiki gvwiki pswiktionary trwikibooks sswiki zawiki nlwikivoyage abwiki urwikibooks urwikiquote nowikisource vowikiquote mswiktionary abwiktionary fawikisource cywikibooks tpiwiktionary jawikinews etwiktionary testwikidatawiki urwiki htwiki stqwiki tpiwiki cswiki elwikibooks gdwiki dawikisource nawiki udmwiki zhwikiquote commonswiki itwiki frwiktionary pawiki outreachwiki eewiki srnwiki gotwiki glwiktionary tnwiktionary quwiki hawiki ugwiki ptwikiversity nowiktionary wikidatawiki zhwiki thwiki rowiki kmwiki zhwikibooks hewiktionary yiwikisource nawikiquote eowiktionary cawikibooks mgwikibooks rswikimedia dzwiki tkwikiquote orwiki eswikiversity ukwikibooks hiwiktionary tiwiktionary yowikibooks dawikiquote nawiktionary tiwiki guwikiquote ttwikibooks map_bmswiki advisorywiki ukwikinews ukwikiquote ptwikibooks mywikibooks simplewikiquote wikimania2009wiki trwikimedia ugwiktionary euwikiquote ocwiktionary ugwikibooks ugwikiquote etwikibooks bgwiki iawiki crhwiki furwiki zuwiki muswiki mtwiktionary kswikibooks mswikibooks mrwikibooks kswikiquote krwikiquote dawikibooks etwikimedia wikimania2010wiki iswiktionary cswikiversity dzwiktionary zawikiquote elwiki plwikisource vlswiki emlwiki lawikiquote kswiki crwiki mrwikiquote mowiktionary krwiki knwikisource suwiki fawikiquote frwiki jawiki ptwiki plwiki ");
	//char *wikilist = strdup("nlwikibooks "); // for testing
    char *datelist = strdup("20140105 20140105 20140104 20140104 20140104 20140104 20140104 20140104 20140104 20140104 20140104 20140104 20140104 20140104 20140104 20140104 20140104 20140104 20140104 20140104 20140104 20140104 20140104 20140104 20140104 20140104 20140104 20140104 20140104 20140104 20140104 20140104 20140104 20140104 20140104 20140104 20140104 20140104 20140104 20140104 20140104 20140104 20140104 20140104 20140104 20140104 20140104 20140104 20140104 20140104 20140104 20140104 20140104 20140104 20140104 20140104 20140104 20140104 20140104 20140104 20140104 20140104 20140104 20140104 20140104 20140104 20140104 20140104 20140104 20140104 20140104 20140104 20140104 20140104 20140104 20140104 20140104 20140104 20140104 20140104 20140104 20140104 20140104 20140104 20140104 20140104 20140104 20140104 20140104 20140104 20140104 20140104 20140104 20140104 20140104 20140104 20140104 20140104 20140103 20140103 20140103 20140103 20140103 20140103 20140103 20140103 20140103 20140103 20140103 20140103 20140103 20140103 20140103 20140103 20140103 20140103 20140103 20140103 20140103 20140103 20140103 20140103 20140103 20140103 20140103 20140103 20140103 20140103 20140103 20140103 20140103 20140103 20140103 20140103 20140103 20140103 20140103 20140103 20140103 20140103 20140103 20140103 20140103 20140102 20140102 20140101 20140101 20140101 20140101 20140101 20140101 20140101 20140101 20140101 20140101 20140101 20140101 20140101 20140101 20140101 20140101 20140101 20140101 20140101 20140101 20140101 20140101 20140101 20140101 20140101 20140101 20140101 20140101 20140101 20140101 20140101 20140101 20140101 20140101 20140101 20140101 20140101 20140101 20140101 20140101 20140101 20140101 20140101 20140101 20140101 20140101 20140101 20140101 20140101 20140101 20140101 20140101 20140101 20140101 20140101 20140101 20140101 20140101 20140101 20140101 20140101 20140101 20140101 20140101 20140101 20140101 20140101 20140101 20140101 20140101 20140101 20140101 20140101 20140101 20131231 20131231 20131231 20131231 20131231 20131231 20131231 20131231 20131231 20131231 20131231 20131231 20131231 20131231 20131231 20131231 20131231 20131231 20131231 20131231 20131231 20131231 20131231 20131231 20131231 20131231 20131231 20131231 20131231 20131231 20131231 20131231 20131231 20131231 20131231 20131231 20131231 20131231 20131231 20131231 20131231 20131231 20131231 20131231 20131231 20131231 20131231 20131231 20131231 20131231 20131231 20131231 20131231 20131231 20131231 20131231 20131231 20131231 20131231 20131231 20131231 20131231 20131231 20131231 20131231 20131231 20131231 20131231 20131231 20131231 20131231 20131231 20131231 20131231 20131231 20131231 20131231 20131231 20131231 20131231 20131231 20131231 20131231 20131231 20131231 20131231 20131231 20131231 20131231 20131231 20131231 20131231 20131231 20131231 20131231 20131231 20131231 20131231 20131231 20131231 20131231 20131231 20131231 20131231 20131231 20131231 20131231 20131231 20131231 20131231 20131231 20131231 20131231 20131231 20131231 20131231 20131231 20131231 20131231 20131231 20131231 20131231 20131231 20131231 20131231 20131231 20131231 20131231 20131231 20131231 20131231 20131231 20131231 20131231 20131231 20131231 20131231 20131231 20131231 20131231 20131230 20131230 20131230 20131230 20131230 20131230 20131230 20131230 20131230 20131230 20131230 20131230 20131230 20131230 20131230 20131230 20131230 20131230 20131230 20131230 20131230 20131230 20131230 20131230 20131230 20131230 20131230 20131230 20131230 20131230 20131230 20131230 20131230 20131230 20131230 20131230 20131230 20131230 20131230 20131230 20131230 20131230 20131230 20131230 20131230 20131230 20131230 20131230 20131230 20131230 20131230 20131230 20131230 20131230 20131229 20131229 20131229 20131229 20131229 20131229 20131229 20131229 20131229 20131229 20131229 20131229 20131229 20131229 20131229 20131229 20131229 20131229 20131229 20131229 20131229 20131229 20131229 20131229 20131229 20131229 20131229 20131229 20131229 20131229 20131229 20131229 20131229 20131229 20131229 20131229 20131229 20131229 20131229 20131229 20131229 20131229 20131229 20131229 20131229 20131229 20131229 20131229 20131229 20131229 20131229 20131229 20131229 20131229 20131229 20131229 20131229 20131229 20131229 20131228 20131228 20131228 20131228 20131228 20131228 20131228 20131228 20131228 20131228 20131228 20131228 20131228 20131228 20131228 20131228 20131228 20131228 20131228 20131228 20131228 20131228 20131228 20131228 20131228 20131228 20131228 20131228 20131228 20131228 20131228 20131228 20131228 20131228 20131228 20131228 20131228 20131228 20131228 20131228 20131228 20131228 20131228 20131228 20131228 20131228 20131228 20131228 20131228 20131228 20131228 20131228 20131228 20131228 20131228 20131228 20131228 20131228 20131228 20131228 20131228 20131228 20131228 20131228 20131228 20131228 20131228 20131228 20131228 20131228 20131228 20131228 20131228 20131228 20131228 20131228 20131228 20131228 20131228 20131228 20131228 20131228 20131228 20131228 20131228 20131228 20131228 20131228 20131228 20131228 20131228 20131228 20131228 20131228 20131228 20131228 20131228 20131227 20131227 20131227 20131227 20131227 20131227 20131227 20131227 20131227 20131227 20131227 20131227 20131227 20131227 20131227 20131227 20131227 20131227 20131227 20131227 20131227 20131227 20131227 20131227 20131227 20131227 20131227 20131227 20131227 20131227 20131227 20131227 20131227 20131227 20131227 20131227 20131227 20131227 20131227 20131227 20131226 20131226 20131226 20131226 20131226 20131226 20131226 20131226 20131226 20131226 20131226 20131226 20131226 20131226 20131226 20131226 20131226 20131226 20131226 20131226 20131226 20131226 20131226 20131226 20131226 20131226 20131226 20131226 20131226 20131226 20131226 20131226 20131226 20131226 20131226 20131226 20131225 20131225 20131225 20131225 20131225 20131225 20131225 20131225 20131225 20131225 20131225 20131225 20131225 20131225 20131225 20131225 20131225 20131225 20131225 20131225 20131225 20131225 20131225 20131225 20131225 20131225 20131225 20131225 20131225 20131225 20131225 20131225 20131225 20131225 20131225 20131225 20131225 20131225 20131225 20131225 20131225 20131225 20131225 20131225 20131225 20131225 20131225 20131225 20131225 20131225 20131225 20131225 20131225 20131225 20131225 20131225 20131225 20131225 20131225 20131225 20131225 20131225 20131225 20131225 20131225 20131225 20131225 20131225 20131225 20131225 20131225 20131225 20131225 20131224 20131224 20131224 20131224 20131224 20131224 20131224 20131224 20131224 20131224 20131224 20131224 20131224 20131224 20131224 20131224 20131224 20131224 20131224 20131224 20131224 20131224 20131224 20131224 20131224 20131224 20131224 20131223 20131223 20131223 20131223 20131223 20131223 20131223 20131222 20131222 20131222 20131222 20131222 20131222 20131222 20131222 20131222 20131222 20131222 20131222 20131222 20131222 20131222 20131221 20131221 20131221 20131221 20131221 20131221 20131221 20131221 20131221 20131221 20131221 20131221 20131221 20131221 20131221 20131221 20131221 20131221 20131221 20131221 20131221 20131221 20131221 20131221 20131221 20131221 20131221 20131221 20131221 20131221 20131221 20131221 20131221 20131221 20131221 20131221 20131221 20131221 20131221 20131221 20131221 20131221 20131221 20131221 20131221 20131221 20131221 20131221 20131221 20131221 20131221 20131221 20131221 20131221 20131221 20131221 20131221 20131221 20131221 20131221 20131221 20131221 20131221 20131221 20131221 20131221 20131221 20131221 20131221 20131221 20131221 20131221 20131221 20131220 20131219 20131218 20131218 ");
	//char *datelist = strdup("20140104 "); // for testing
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
