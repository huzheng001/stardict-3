StarDict can be compiled and run in windows.

Building StarDict
=================

You may use either Dev-C++ or MS Visual Studio 2005 to build StarDict. The guide below deals with Dev-C++. For details specific to MSVS see msvc_2005\readme.txt, MS Visual Studio solution file is in the msvc_2005 directory. Dev-C++ support will be discontinued soon, use MS Visual Studio if possible.

Please install Dev-C++, they can be found in http://www.bloodshed.net/dev/
devcpp-4.9.9.2_setup.exe
Use default installation directory X:\Dev-Cpp

Then install developer packages, they can be found at http://www.gtk.org/download-windows.html
These packages are needed:
atk-dev_1.28.0-1_win32.zip cairo-dev_1.8.8-2_win32.zip gettext-runtime-dev-0.17-1.zip glib-dev_2.22.3-1_win32.zip gtk+-dev_2.16.6-1_win32.zip libiconv-1.9.1.bin.woe32.zip pango-dev_1.26.1-1_win32.zip zlib123-dll.zip
Just extract them into X:\Dev-Cpp\ is OK.
Notice, for Zlib, you need to put the zlib1.dll in X:\Dev-Cpp\bin folder.

Here is a stardict.dev
use Dev-C++ to open it, then compile it. Becase the compile command is too long, it can't be compile in win98, please use win2000 or XP.

my environment: Windows XP, Dev-Cpp 4.9.9.2

After compiled, you will find stardict.exe at src/.
You cannot start stardict.exe in place, in the src directory, since a special directory structure is needed.

You can find plug-in projects at stardict-plugins/.

You can find the hook dll projects as src/win32/TextOutSpy.dev and src/win32/TextOutHook.dev.
They implement mouseover scanning feature.

For the sapi-tts plugin, you need to compile it by vs2005, see msvc_2005/readme.txt.

Unicode
=======
It's recommended to build Unicode (versus ANSI) version of StarDict. That's done by default. You need to define UNICODE macro in compiler command line to build Unicode version.

Build the installer
===================

Grab and install NSIS: http://www.nullsoft.com/free/nsis
I was using the 2.44 version.

I use linux commands to illustrate the steps, but you cann't do it in this way :)
You may use stardict-prepare-installer.js to build win32-install-dir directory structure provided you already have all the required files in place. See comment at the head of the file for details of running the script.

Some of the required files cannot be created on Windows, you need Linux to prepare them.
In the steps below <linux-StarDict-distr> referes to a directory containing required files.

===============
cd stardict-3.0.0
mkdir win32-install-dir

cp src/stardict.exe win32-install-dir
cp src/win32/TextOutSpy.dll win32-install-dir
cp src/win32/TextOutHook.dll win32-install-dir

mkdir -p win32-install-dir/locale/zh_CN/LC_MESSAGES/
cp <linux-StarDict-distr>/po/zh_CN.gmo win32-install-dir/locale/zh_CN/LC_MESSAGES/stardict.mo
mkdir -p win32-install-dir/locale/ru/LC_MESSAGES/
cp <linux-StarDict-distr>/po/ru.gmo win32-install-dir/locale/ru/LC_MESSAGES/stardict.mo
mkdir -p win32-install-dir/locale/cs/LC_MESSAGES/
cp <linux-StarDict-distr>/po/cs.gmo win32-install-dir/locale/cs/LC_MESSAGES/stardict.mo
# repeat for all <linux-StarDict-distr>/po/*.gmo files

mkdir -p win32-install-dir/pixmaps
cp pixmaps/stardict.png win32-install-dir/pixmaps
cp src/pixmaps/* win32-install-dir/pixmaps # except docklet_*.png
mkdir -p win32-install-dir/sounds
cp src/sounds/*.wav win32-install-dir/sounds
mkdir -p win32-install-dir/dic
mkdir -p win32-install-dir/treedict
mkdir -p win32-install-dir/skins

mkdir -p win32-install-dir/help/C
cp <linux-StarDict-distr>/help/C/html/* win32-install-dir/help/C
mkdir -p win32-install-dir/help/ru
cp <linux-StarDict-distr>/help/ru/html/* win32-install-dir/help/ru
# repeat for all <linux-StarDict-distr>/help/*/html directories

mkdir -p win32-install-dir/plugins
cp stardict-plugins/stardict-*-plugin/*.dll win32-install-dir/plugins

cp src/win32/acrobat/win32/Release/StarDict.api win32-install-dir

Download gtk2-runtime-*.exe from http://sourceforge.net/projects/gtk-win and put it into redist.

If you build the project with MS Visual Studio.
1. Download Microsoft Visual C++ 2005 Redistributable Package and put into redist.
	For VS 2005 non SP1 version take this
	http://www.microsoft.com/downloads/details.aspx?familyid=32BC1BEE-A3F9-4C13-9C99-220B62A191EE&displaylang=en
	and for VS 2005 SP1 take this:
	http://www.microsoft.com/downloads/details.aspx?FamilyID=200B2FD9-AE1A-4A14-984D-389C36F85647&displaylang=en
2. Uncomment '!define MSVC' in stardict-installer.nsi

Double click the stardict-installer.nsi :) NSIS will build the installer.


Debugging
=========

For debug, use these codes:
=====
FILE *file;
file=fopen("C:\\a.txt", "a+");
fprintf(file, "a\n");
fclose(file);
=====

You may simplify debugging process if you configure windows console in Stardict.
Stardict sends debug messages, warnings, errors to console.
Unfortunately, windows GUI applications do not attach to a console by default,
hence all the aforementioned messages go nowhere.

Stardict can attach itself to a windows console either opening a new instance or reusing
an existing one. To enable that feature define ATTACH_WINDOWS_CONSOLE preprocessor symbol
when compiling the project. If you use Windows XP or later, it is recommended to define
_WIN32_WINNT=0x0501 as well. That allows to connect to an existing console, otherwise
Stardict always opens a new console window. The later case has the disadvantage that
if the application crushs, the console window closes so quickly you cannot read the error message.
To reuse the existing console window, start Stardict in that console.
All messages will go into that window.
If you start Stardict not from console, a new console windows is opened.

Windows console has a limitation, it cannot show all unicode characters.
To overcome this limitation StarDict can show output in a log window.
The log window has no problems with showing unicode characters, but it can not fully
duplicate console output. StarDict can show output of g_debug, g_error, g_warning and the like, g_print, g_printf(?) functions. I'll not see output from printf in the log window, for example. To enable the log window define ENABLE_LOG_WINDOW preprocessor symbol.

To set up preprocessor symbols in Dev-C++ do the following:
main menu->Project->Project options->Parameters tab.
Add "-DATTACH_WINDOWS_CONSOLE -D_WIN32_WINNT=0x0501 -DENABLE_LOG_WINDOW" to the C and C++ compiler command line.

All that windows console stuff is not much reliable, if it does not work, retreat to printing into a file or to the log window.

Starting stardict
=================

To start Stardict just for debugging, just to make sure it works, you need:
1) GTK+ binaries,
2) minumum directory structure.

You need at least the following binary packages to start Stardict:
atk_1.28.0-1_win32.zip cairo_1.8.8-2_win32.zip expat_2.0.1-1_win32.zip fontconfig_2.8.0-1_win32.zip freetype_2.3.11-1_win32.zip gettext-runtime-0.17-1.zip glib_2.22.3-1_win32.zip gtk+_2.16.6-1_win32.zip libpng_1.2.40-1_win32.zip pango_1.26.1-1_win32.zip zlib123-dll.zip

Extract binaries packages into some directory, for instance, X:\Stardict-libs.
Move zlib1.dll into bin directory.
Add X:\Stardict-libs\bin in the path variable.

Create a empty directory to start Stardict in, for instance, X:\Stardict-start
Copy stardict.exe there.
Create X:\Stardict-start\pixmaps directory and copy there *.png files from
stardict-source\pixmaps and stardict-source\src\pixmaps
Now stardict.exe is ready to start.

============
StarDict's win32 port got many experience from ReciteWord (my another project, http://reciteword.sourceforge.net) and Pidgin: http://www.pidgin.im

Note: when use fopen(), use "rb", never "r" (unless you know what you are doing).

Hu Zheng <huzheng001@gmail.com> http://www.huzheng.org
2009.7.20
