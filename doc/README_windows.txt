StarDict can be compiled and run in windows.

Building StarDict
=================

Use MS Visual Studio 2005 to build StarDict.

gtk
----

You need to install developer packages into "gtk" directory, they can be found at http://www.gimp.org/~tml/gimp/win32/downloads.html

These packages are needed:
atk-dev_1.30.0-1_win32.zip
cairo-dev_1.8.10-3_win32.zip
gettext-runtime-dev-0.17-1.zip
glib-dev_2.24.0-2_win32.zip
gtk+-dev_2.16.6-2_win32.zip
libiconv-1.9.1.bin.woe32.zip
pango-dev_1.28.0-1_win32.zip
zlib-dev_1.2.4-2_win32.zip

Just extract them into "msvc_2005\gtk\" is OK.

libxml library
--------------

stardict-editor project depends on on libxml library (http://xmlsoft.org/).
If you plan to build only StarDict, you do not need this library.

Download libxml2-*.win32.zip from ftp://ftp.zlatkovic.com/libxml/
Unpack into a temporary directory.
Copy ibxml2-*.win32/include/libxml directory into msvc_2005/libxml
Now stardict-editor need not be linked with libxml library, it only uses header files.

Building libsigc++
------------------

1. Download libsigc++ (http://libsigc.sourceforge.net/), unpack it into msvc_2005 directory. You'll get a directory like "msvc_2005\libsigc++-2.2.4.2". 
2. Rename it to "msvc_2005\libsigc++-src". 
3. Build the libsigc++ project with MS Visual Studio 2005 (do Release and Debug builds). 
4. Copy 
	Release\sigc-vc80-2_0.lib, 
	Release\sigc-vc80-2_0.dll, 
	Debug\sigc-vc80-d-2_0.lib,
	Debug\sigc-vc80-d-2_0.dll 
	into "msvc_2005\libsigc++\".

libsigc++ have static link problem on vs2005 presently.

Notes
-----

For stardict_powerword_parsedata.cpp, you need to add a UTF-8 BOM in its head to fix the compile problem. Just use the notepad to open it then save.
For wordnet plugin files, they are the same.
(8 May 2010. Compilation succeeds without the fix. It is not needed anymore?)

For sapi-tts plugin, you need to install Microsoft Speech SDK. Download SpeechSDK51.exe file. Install it into "C:\Program Files\Microsoft Speech SDK 5.1". Fix these compile errors in sphelper.h:
=====
1) add to the top of the file to prevent many errors
#pragma warning( disable : 4430 )
#pragma warning( disable : 4996 )
2) line 2372
	replace
        for (const WCHAR * psz = (const WCHAR *)lParam; *psz; psz++) {}
	with
        const WCHAR * psz;
        for (psz = (const WCHAR *)lParam; *psz; psz++) {}
3) line 2560
	replace
    SPPHONEID* pphoneId = dsPhoneId;
	with
    SPPHONEID* pphoneId = (SPPHONEID*)((WCHAR *)dsPhoneId);
4) line 2634
	replace 
	pphoneId += wcslen(pphoneId) + 1;
	with
    pphoneId += wcslen((const wchar_t *)pphoneId) + 1;
=====

There are two crash bug with vs2005, which you need to notice.
1. Use stardict_g_fopen instead of g_fopen. See http://bugzilla.gnome.org/show_bug.cgi?id=476810
2. Use fprintf_s instead of fprintf, or it will crash. This is a little strange.

libintl.h redefines *printf functions to libintl_*printf functions.
That may be the cause of the crash.
printf outputs nothing with console attached. Use printf_s instead, or better g_print.

There are two builds in vs2005: Debug and Release, you should choose Release version if you plan to distribute the result, you should choose Debug version for debugging the project.

You should can compile and run stardict successfully now. You cannot start stardict.exe in place, since a special directory structure is needed.

You cannot start stardict.exe in place, in the src directory, since a special directory structure is needed.

Unicode
=======
It's recommended to build Unicode (versus ANSI) version of StarDict. That's done by default. You need to define UNICODE macro in compiler command line to build Unicode version.

Build the installer
===================

Grab and install NSIS: http://www.nullsoft.com/free/nsis
I was using the 2.44 version.

Use stardict-prepare-installer.js to build win32-install-dir directory structure. You must have all the required files in place before running the script. See comment at the head of the file for details of running the script.

Some of the required files cannot be created on Windows, you need Linux to prepare them.

Download gtk2-runtime-*.exe from http://sourceforge.net/projects/gtk-win and put it into redist.

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

Running StarDict on another computer
====================================

You may encounter a problem running StarDict project compiled with VS 2005 on other computer not having VS 2005. You may see the following rather unclear error message "This application has failed to start because the application configuration is incorrect. Reinstalling the application may fix this problem." The error happens because StarDict application, plugin DLLs, hook DLLs are linked with CRT (C runtime library) dynamically and you may not have the appropriate version on that dll on the target computer. 

If you build Release version of the project, you need to download and install Microsoft Visual C++ 2005 Redistributable Package (x86). 
For VS 2005 non SP1 version take this
http://www.microsoft.com/downloads/details.aspx?familyid=32BC1BEE-A3F9-4C13-9C99-220B62A191EE&displaylang=en
and for VS 2005 SP1 take this:
http://www.microsoft.com/downloads/details.aspx?FamilyID=200B2FD9-AE1A-4A14-984D-389C36F85647&displaylang=en

If you build Debug version of the project, you need a debug version of CRT which not redistributable. I see at least one reason why you may need to use the debug version of StarDict - to debug the project remotely. Microsoft prohibit do distribute the debug version of CRT, but I assume a developer may do that for debuging purpose. Anyway this how to install the debuging version of CRT onto the target machine.
1. In Visual Studio create new Setup Project, name it Setup-CRTDebug.
2. Right-click the project in the Solution Explorer, select Add -> Merge module.
3. In the file selection dialog select "C:\Program Files\Common Files\Merge Modules\Microsoft_VC80_DebugCRT_x86.msm".
4. Build the project.
5. You've created Setup-CRTDebug.msi setup file. Run it on the target machine. It installs debug version of CRT. In the course of executing you'll be asked to specify an installation directory somewhere under "C:\Program Files". Never mind. The installer does not copy anything there. It does not even create that folder.
6. Now you should be able to start StarDict.

StarDict version
----------------

StarDict version for MSVC build can be specified in the msvc_2005\stardictrc.rc.

============
StarDict's win32 port got many experience from ReciteWord (my another project, http://reciteword.sourceforge.net) and Pidgin: http://www.pidgin.im

Note: when use fopen(), use "rb", never "r" (unless you know what you are doing).

Hu Zheng <huzheng001@gmail.com> http://www.huzheng.org
2009.7.20
