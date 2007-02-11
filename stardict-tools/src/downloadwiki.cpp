#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>

#include <string>

void download(char *wiki, char *date)
{
    if (strcmp(wiki, "enwiki")==0 || strcmp(wiki, "dewiki")==0 || strcmp(wiki, "frwiki")==0) // File too big.
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
    dictdirname += '-';
    dictdirname += date;
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
		exit(0);
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
    char *wikilist = strdup("aawiki aawikibooks aawiktionary abwiki abwiktionary afwiki afwikibooks afwikiquote afwiktionary akwiki akwikibooks akwiktionary alswiki alswikibooks alswikiquote alswiktionary amwiki amwikiquote amwiktionary angwiki angwikibooks angwikiquote angwikisource angwiktionary anwiki anwiktionary arcwiki arwiki arwikibooks arwikinews arwikiquote arwikisource arwiktionary astwiki astwikibooks astwikiquote astwiktionary aswiki aswikibooks aswiktionary avwiki avwiktionary aywiki aywikibooks aywiktionary azwiki azwikibooks azwikiquote azwikisource azwiktionary barwiki bat_smgwiki bawiki bawikibooks bawiktionary betawikiversity bewiki bewikibooks bewikiquote bewiktionary bgwiki bgwikibooks bgwikinews bgwikiquote bgwikisource bgwiktionary bhwiki bhwiktionary biwiki biwikibooks biwiktionary bmwiki bmwikibooks bmwikiquote bmwiktionary bnwiki bnwikibooks bnwiktionary bowiki bowikibooks bowiktionary bpywiki brwiki brwiktionary bswiki bswikibooks bswikinews bswikiquote bswikisource bswiktionary bugwiki bxrwiki cawiki cawikibooks cawikinews cawikiquote cawikisource cawiktionary cbk_zamwiki cdowiki cebwiki cewiki chowiki chrwiki chrwiktionary chwiki chwikibooks chwikimedia chwiktionary chywiki closed_zh_twwiki commonswiki cowiki cowikibooks cowikiquote cowiktionary crwiki crwikiquote crwiktionary csbwiki csbwiktionary cswiki cswikibooks cswikiquote cswikisource cswiktionary cuwiki cvwiki cvwikibooks cywiki cywikibooks cywikiquote cywikisource cywiktionary dawiki dawikibooks dawikiquote dawikisource dawiktionary dewiki dewikibooks dewikinews dewikiquote dewikisource dewikiversity dewiktionary diqwiki dkwikibooks dkwiktionary dvwiki dvwiktionary dzwiki dzwiktionary eewiki elwiki elwikibooks elwikiquote elwikisource elwiktionary emlwiki enwiki enwikibooks enwikinews enwikiquote enwikisource enwikiversity enwiktionary eowiki eowikibooks eowikiquote eowiktionary eswiki eswikibooks eswikinews eswikiquote eswikisource eswikiversity eswiktionary etwiki etwikibooks etwikiquote etwikisource etwiktionary euwiki euwikibooks euwikiquote euwiktionary fawiki fawikibooks fawikiquote fawikisource fawiktionary ffwiki fiu_vrowiki fiwiki fiwikibooks fiwikiquote fiwikisource fiwiktionary fjwiki fjwiktionary foundationwiki fowiki fowikisource fowiktionary frpwiki frwiki frwikibooks frwikinews frwikiquote frwikisource frwiktionary furwiki fywiki fywikibooks fywiktionary gawiki gawikibooks gawikiquote gawiktionary gdwiki gdwiktionary glkwiki glwiki glwikibooks glwikiquote glwikisource glwiktionary gnwiki gnwikibooks gnwiktionary gotwiki gotwikibooks guwiki guwikibooks guwikiquote guwiktionary gvwiki gvwiktionary hawiki hawiktionary hawwiki hewiki hewikibooks hewikinews hewikiquote hewikisource hewiktionary hiwiki hiwikibooks hiwikiquote hiwiktionary howiki hrwiki hrwikibooks hrwikiquote hrwikisource hrwiktionary hsbwiki htwiki htwikisource huwiki huwikibooks huwikiquote huwikisource huwiktionary hywiki hywikibooks hywikiquote hywiktionary hzwiki iawiki iawikibooks iawiktionary idwiki idwikibooks idwikiquote idwikisource idwiktionary iewiki iewikibooks iewiktionary igwiki iiwiki ikwiki ikwiktionary ilowiki incubatorwiki iowiki iowiktionary iswiki iswikibooks iswikiquote iswikisource iswiktionary itwiki itwikibooks itwikinews itwikiquote itwikisource itwiktionary iuwiki iuwiktionary jawiki jawikibooks jawikinews jawikiquote jawikisource jawiktionary jbowiki jbowiktionary jvwiki jvwiktionary kawiki kawikibooks kawikiquote kawiktionary kgwiki kiwiki kjwiki kkwiki kkwikibooks kkwikiquote kkwiktionary klwiki klwiktionary kmwiki kmwikibooks kmwiktionary knwiki knwikibooks knwikiquote knwikisource knwiktionary kowiki kowikibooks kowikiquote kowikisource kowiktionary krwiki krwikiquote kshwiki kswiki kswikibooks kswikiquote kswiktionary kuwiki kuwikibooks kuwikiquote kuwiktionary kvwiki kwwiki kwwikiquote kwwiktionary kywiki kywikibooks kywikiquote kywiktionary ladwiki lawiki lawikibooks lawikiquote lawikisource lawiktionary lbewiki lbwiki lbwikibooks lbwikiquote lbwiktionary lgwiki lijwiki liwiki liwiktionary lmowiki lnwiki lnwikibooks lnwiktionary lowiki lowiktionary ltwiki ltwikibooks ltwikiquote ltwikisource ltwiktionary lvwiki lvwikibooks lvwiktionary map_bmswiki mediawikiwiki metawiki mgwiki mgwikibooks mgwiktionary mhwiki mhwiktionary miwiki miwikibooks miwiktionary mkwiki mkwikibooks mkwikisource mkwiktionary mlwiki mlwikibooks mlwikiquote mlwikisource mlwiktionary mnwiki mnwikibooks mnwiktionary mowiki mowiktionary mrwiki mrwikibooks mrwikiquote mrwiktionary mswiki mswikibooks mswiktionary mtwiki mtwiktionary muswiki mywiki mywikibooks mywiktionary mznwiki nahwiki nahwikibooks nahwiktionary napwiki nawiki nawikibooks nawikiquote nawiktionary nds_nlwiki ndswiki ndswikibooks ndswikiquote ndswiktionary newiki newikibooks newiktionary newwiki ngwiki nlwiki nlwikibooks nlwikimedia nlwikinews nlwikiquote nlwikisource nlwiktionary nnwiki nnwikiquote nnwiktionary nostalgiawiki novwiki nowiki nowikibooks nowikinews nowikiquote nowikisource nowiktionary nrmwiki nvwiki nywiki nzwikimedia ocwiki ocwikibooks ocwiktionary omwiki omwiktionary orwiki orwiktionary oswiki pagwiki pamwiki papwiki pawiki pawikibooks pawiktionary pdcwiki pihwiki piwiki piwiktionary plwiki plwikibooks plwikimedia plwikinews plwikiquote plwikisource plwiktionary pmswiki pswiki pswikibooks pswiktionary ptwiki ptwikibooks ptwikinews ptwikiquote ptwikisource ptwiktionary quwiki quwikibooks quwikiquote quwiktionary rmwiki rmwikibooks rmwiktionary rmywiki rnwiki rnwiktionary roa_rupwiki roa_rupwiktionary roa_tarawiki rowiki rowikibooks rowikinews rowikiquote rowikisource rowiktionary ru_sibwiki ruwiki ruwikibooks ruwikinews ruwikiquote ruwikisource ruwiktionary rwwiki rwwiktionary sawiki sawikibooks sawiktionary scnwiki scnwiktionary scowiki scwiki scwiktionary sdwiki sdwikinews sdwiktionary sep11wiki sewiki sewikibooks sgwiki sgwiktionary shwiki shwiktionary simplewiki simplewikibooks simplewikiquote simplewiktionary siwiki siwikibooks siwiktionary skwiki skwikibooks skwikiquote skwikisource skwiktionary slwiki slwikibooks slwikiquote slwikisource slwiktionary smwiki smwiktionary snwiki snwiktionary sourceswiki sowiki sowiktionary specieswiki sqwiki sqwikibooks sqwikiquote sqwiktionary srwiki srwikibooks srwikinews srwikiquote srwikisource srwiktionary sswiki sswiktionary stwiki stwiktionary suwiki suwikibooks suwikiquote suwiktionary svwiki svwikibooks svwikinews svwikiquote svwikisource svwiktionary swwiki swwikibooks swwiktionary tawiki tawikibooks tawikiquote tawiktionary testwiki tetwiki tewiki tewikibooks tewikiquote tewikisource tewiktionary tgwiki tgwikibooks tgwiktionary thwiki thwikibooks thwikinews thwikiquote thwikisource thwiktionary tiwiki tiwiktionary tkwiki tkwikibooks tkwikiquote tkwiktionary tlhwiki tlhwiktionary tlwiki tlwikibooks tlwiktionary tnwiki tnwiktionary tokiponawiki tokiponawikibooks tokiponawikiquote tokiponawiktionary towiki towiktionary tpiwiki tpiwiktionary trwiki trwikibooks trwikiquote trwikisource trwiktionary tswiki tswiktionary ttwiki ttwikibooks ttwikiquote ttwiktionary tumwiki twwiki twwiktionary tywiki udmwiki ugwiki ugwikibooks ugwikiquote ugwiktionary ukwiki ukwikibooks ukwikinews ukwikiquote ukwikisource ukwiktionary urwiki urwikibooks urwikiquote urwiktionary uzwiki uzwikibooks uzwikiquote uzwiktionary vecwiki vewiki viwiki viwikibooks viwikiquote viwikisource viwiktionary vlswiki vowiki vowikibooks vowikiquote vowiktionary warwiki wawiki wawikibooks wawiktionary wikimania2005wiki wikimania2006wiki wikimania2007wiki wowiki wowikiquote wowiktionary wuuwiki xalwiki xhwiki xhwikibooks xhwiktionary yiwiki yiwikisource yiwiktionary yowiki yowikibooks yowiktionary zawiki zawikibooks zawikiquote zawiktionary zeawiki zh_classicalwiki zh_min_nanwiki zh_min_nanwikibooks zh_min_nanwikiquote zh_min_nanwikisource zh_min_nanwiktionary zh_yuewiki zhwiki zhwikibooks zhwikinews zhwikiquote zhwikisource zhwiktionary zuwiki zuwikibooks zuwiktionary ");
    char *datelist = strdup("20061130 20061130 20061130 20061130 20061130 20061130 20061130 20061130 20061130 20061130 20061130 20061130 20061130 20061130 20061130 20061130 20061130 20061130 20061130 20061130 20061130 20061130 20061130 20061130 20061130 20061130 20061130 20061130 20061130 20061130 20061130 20061130 20061130 20061130 20061130 20061130 20061130 20061130 20061130 20061130 20061130 20061130 20061130 20061130 20061130 20061130 20061130 20061130 20061130 20061130 20061130 20061130 20061130 20061130 20061130 20061130 20061130 20061130 20061130 20061130 20061130 20061130 20061130 20061130 20061130 20061130 20061130 20061130 20061130 20061130 20061130 20061130 20061130 20061130 20061130 20061130 20061130 20061130 20061130 20061130 20061130 20061130 20061130 20061130 20061130 20061130 20061130 20061130 20061130 20061130 20061130 20061130 20061130 20061130 20061130 20061130 20061130 20061130 20061130 20061130 20061130 20061130 20061130 20061130 20061130 20061130 20061130 20061130 20061130 20061130 20061130 20061130 20061130 20061130 20061130 20061130 20061130 20061130 20061130 20061130 20061130 20061201 20061130 20061130 20061130 20061130 20061130 20061130 20061130 20061130 20061130 20061130 20061130 20061130 20061201 20061130 20061130 20061130 20061130 20061130 20061130 20061201 20061130 20061130 20061130 20061201 20061130 20061130 20061130 20061130 20061130 20061130 20061130 20061130 20061130 20061130 20061130 20061130 20061130 20061130 20061130 20061201 20061201 20061201 20061130 20061130 20061202 20061202 20061130 20061130 20061130 20061202 20061130 20061130 20061130 20061130 20061130 20061201 20061203 20061201 20061201 20061201 20061201 20061201 20061201 20061201 20061201 20061201 20061201 20061201 20061201 20061201 20061201 20061201 20061203 20061201 20061201 20061201 20061201 20061201 20061201 20061201 20061201 20061201 20061201 20061201 20061204 20061201 20061201 20061201 20061201 20061204 20061201 20061201 20061201 20061201 20061201 20061201 20061201 20061201 20061201 20061201 20061201 20061201 20061201 20061201 20061201 20061201 20061201 20061201 20061201 20061201 20061201 20061201 20061201 20061201 20061201 20061201 20061201 20061201 20061201 20061201 20061204 20061201 20061201 20061201 20061201 20061201 20061201 20061201 20061201 20061201 20061201 20061205 20061201 20061201 20061201 20061201 20061201 20061201 20061201 20061205 20061201 20061201 20061201 20061201 20061201 20061201 20061201 20061201 20061201 20061201 20061201 20061201 20061205 20061201 20061201 20061201 20061201 20061201 20061201 20061201 20061201 20061201 20061201 20061201 20061201 20061201 20061201 20061205 20061201 20061201 20061201 20061201 20061201 20061205 20061201 20061201 20061201 20061201 20061201 20061201 20061201 20061201 20061204 20061204 20061204 20061204 20061204 20061204 20061204 20061204 20061204 20061204 20061204 20061204 20061204 20061204 20061204 20061204 20061204 20061204 20061204 20061204 20061204 20061204 20061204 20061204 20061204 20061204 20061204 20061204 20061204 20061204 20061204 20061204 20061204 20061204 20061204 20061204 20061204 20061204 20061204 20061204 20061204 20061204 20061204 20061204 20061204 20061204 20061204 20061204 20061204 20061204 20061204 20061204 20061204 20061204 20061204 20061204 20061204 20061204 20061204 20061204 20061204 20061204 20061204 20061204 20061204 20061204 20061204 20061204 20061204 20061204 20061204 20061204 20061204 20061204 20061204 20061206 20061204 20061204 20061204 20061204 20061204 20061204 20061204 20061204 20061204 20061206 20061204 20061204 20061204 20061204 20061204 20061204 20061204 20061204 20061204 20061204 20061204 20061204 20061204 20061204 20061204 20061204 20061204 20061204 20061204 20061204 20061204 20061204 20061204 20061204 20061204 20061204 20061204 20061204 20061204 20061204 20061204 20061204 20061204 20061204 20061204 20061204 20061204 20061204 20061204 20061204 20061204 20061204 20061204 20061204 20061204 20061204 20061204 20061204 20061204 20061204 20061204 20061204 20061204 20061204 20061206 20061204 20061204 20061204 20061204 20061204 20061204 20061204 20061204 20061204 20061204 20061204 20061206 20061204 20061204 20061204 20061204 20061204 20061204 20061204 20061204 20061204 20061204 20061204 20061204 20061204 20061204 20061204 20061204 20061204 20061204 20061204 20061204 20061204 20061204 20061204 20061204 20061204 20061204 20061204 20061207 20061204 20061204 20061204 20061204 20061204 20061207 20061204 20061204 20061204 20061204 20061207 20061204 20061204 20061204 20061204 20061204 20061204 20061204 20061205 20061205 20061205 20061205 20061205 20061205 20061205 20061205 20061205 20061205 20061205 20061207 20061205 20061205 20061205 20061205 20061205 20061205 20061207 20061205 20061205 20061205 20061205 20061205 20061205 20061205 20061205 20061205 20061205 20061205 20061205 20061205 20061205 20061205 20061205 20061205 20061205 20061205 20061205 20061205 20061205 20061205 20061205 20061205 20061205 20061205 20061205 20061205 20061205 20061205 20061205 20061208 20061205 20061205 20061205 20061205 20061208 20061205 20061205 20061205 20061205 20061205 20061205 20061205 20061205 20061208 20061205 20061205 20061205 20061205 20061205 20061205 20061205 20061208 20061205 20061205 20061205 20061205 20061205 20061205 20061205 20061205 20061205 20061205 20061205 20061205 20061205 20061208 20061205 20061205 20061205 20061205 20061205 20061205 20061205 20061205 20061205 20061205 20061205 20061205 20061205 20061205 20061205 20061205 20061205 20061205 20061205 20061205 20061205 20061205 20061205 20061205 20061205 20061205 20061205 20061205 20061205 20061205 20061205 20061205 20061205 20061205 20061205 20061205 20061205 20061205 20061205 20061205 20061205 20061205 20061205 20061205 20061205 20061205 20061205 20061205 20061205 20061208 20061205 20061205 20061205 20061205 20061205 20061205 20061205 20061205 20061205 20061205 20061205 20061205 20061205 20061205 20061205 20061205 20061205 20061205 20061205 20061208 20061205 20061205 20061205 20061205 20061205 20061205 20061205 20061205 20061205 20061205 20061205 20061205 20061205 20061205 20061205 20061208 20061205 20061205 20061205 20061205 20061205 20061205 20061205 20061205 20061205 20061205 20061205 20061205 20061205 20061205 20061205 20061205 20061205 20061205 20061205 20061205 20061205 20061205 20061205 20061205 20061205 20061205 20061205 20061205 20061205 20061205 20061205 20061205 20061205 20061205 20061205 20061205 20061205 20061205 20061205 20061205 20061205 20061205 20061208 20061205 20061205 20061205 20061205 20061205 20061205 20061205 20061205 ");
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
    //I find "frwikiquote" can't be downloaded :(
    free(wikilist);
    free(datelist);
}
