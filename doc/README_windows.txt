StarDict can be compiled and run in windows.

Please install Dev-C++,they can be found in http://www.bloodshed.net/dev/
devcpp-4.9.9.2_setup.exe

Then install developer packages, they can be found at http://www.gtk.org/download-windows.html
These packages are needed:
atk-dev_1.26.0-1_win32.zip cairo-dev_1.8.6-1_win32.zip gettext-runtime-dev-0.17-1.zip glib-dev_2.20.4-1_win32.zip gtk+-dev_2.16.4-1_win32.zip libiconv-1.9.1.bin.woe32.zip pango-dev_1.24.2-1_win32.zip zlib123-dll.zip
Just extract them into X:\Dev-Cpp\ is OK.
Notice, for Zlib, you need to put the zlib1.dll in X:\Dev-Cpp\bin folder.

Here is a stardict.dev
use Dev-C++ to open it, then compile it. Becase the compile command is too long,it can't be compile in win98, please use win2000 or XP.

my environment: Windows XP, Dev-Cpp 4.9.9.2

After compiled, you will find stardict.exe at src/.

You can find plug-in projects at stardict-plugins/.

You can find the hook dll projects as src/win32/TextOutSpy.dev and src/win32/TextOutHook.dev.

For the sapi-tts plugin, you need to compile it by vs2005, see mvsc/readme.txt.

========
To build the installer.

Grab and install NSIS: http://www.nullsoft.com/free/nsis
I was using the 2.44 version.

I use linux commmand to illustrate the steps, but you cann't do it in this way :)

===============
cd stardict-3.0.0
mkdir win32-install-dir

cp src/stardict.exe win32-install-dir
cp src/win32/TextOutSpy.dll win32-install-dir
cp src/win32/TextOutHook.dll win32-install-dir

mkdir -p win32-install-dir/locale/zh_CN/LC_MESSAGES/
cp po/zh_CN.gmo win32-install-dir/locale/zh_CN/LC_MESSAGES/stardict.mo
mkdir -p win32-install-dir/locale/ru/LC_MESSAGES/
cp po/ru.gmo win32-install-dir/locale/ru/LC_MESSAGES/stardict.mo
mkdir -p win32-install-dir/locale/cs/LC_MESSAGES/
cp po/cs.gmo win32-install-dir/locale/cs/LC_MESSAGES/stardict.mo

mkdir -p win32-install-dir/pixmaps
cp pixmaps/stardict.png win32-install-dir/pixmaps
cp src/pixmaps/* win32-install-dir/pixmaps //Notice: docklet_*.png needn't copy!!!
mkdir -p win32-install-dir/sounds
cp src/pixmaps/*.wav win32-install-dir/sounds
mkdir -p win32-install-dir/dic
mkdir -p win32-install-dir/treedict

mkdir -p win32-install-dir/help

You can create stardict.html file by "yelp-pregenerate stardict.xml", yelp-pregenerate is included in yelp-2.4.2, the newer version don't have this tool anymore. Then do some string replacing:
replace "file:///usr/share/sgml/docbook/yelp/docbook/images/" to "../"
replace "ghelp:stardict.xml?" to "./stardict.html#"
replace "ghelp:stardict.xml" to "./stardict.html#id2772190"  //id2772190 is "<b>Table of Contents</b>" 's previous name.
replace "file://./figures/" to "./figures/"
And build the chm file by Visual CHM.

Download gtk-runtime-*.exe from pidgin project and put it into gtk_installer/.

Double click the stardict-installer.nsi :) NSIS will build the installer.


For debug, use these codes:
=====
FILE *file;
file=fopen("C:\\a.txt", "a+");
fprintf(file, "a\n");
fclose(file);
=====

StarDict's win32 port got many experience from ReciteWord(my another project, http://reciteword.sourceforge.net) and Pidgin: http://www.pidgin.im

Note: when use fopen(), use "rb", never "r" (unless you know what you are doing).

Hu Zheng <huzheng001@gmail.com> http://www.huzheng.org
2009.7.20
