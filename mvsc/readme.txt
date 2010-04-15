You need to install developer packages into "gtk" directory, they can be found at http://www.gimp.org/~tml/gimp/win32/downloads.html

These packages are needed:
glib-dev-2.12.13.zip gtk+-dev-2.10.14.zip pango-dev-1.16.4.zip atk-dev-1.12.3.zip cairo-dev-1.4.8.zip gettext-dev-0.14.5.zip zlib123-dll.zip

Just extract them into "mvsc\gtk\" is OK.

Then download libsigc++ (http://libsigc.sourceforge.net/) and compile it. Copy sigc-vc80-2_0.lib to "mvsc\gtk\lib\sigc-2.0d.lib".
libsigc++ have static link problem on vs2005 presently.

For stardict_powerword_parsedata.cpp, you need to add a UTF-8 BOM in its head to fix the compile problem. Just use the notepad to open it then save.
For wordnet plugin files, they are the same.

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
1. Use my_g_fopen instead of g_fopen. See http://bugzilla.gnome.org/show_bug.cgi?id=476810
2. Use fprintf_s instead of fprintf, or it will crash. This is a little strange.

libintl.h redefines *printf functions to libintl_*printf functions.
That may be the cause of the crash.
printf outputs nothing with console attached. Use printf_s instead, or better g_print.

There are two builds in vs2005: Debug and Release, you should choose Release version in most case.

You should can compile and run stardict successfully now.
