You need to install developer packages into "gtk" directory, they can be found at http://www.gimp.org/~tml/gimp/win32/downloads.html

These packages are needed:
glib-dev-2.12.13.zip gtk+-dev-2.10.14.zip pango-dev-1.16.4.zip atk-dev-1.12.3.zip cairo-dev-1.4.8.zip gettext-dev-0.14.5.zip zlib123-dll.zip

Just extract them into "mvsc\gtk\" is OK.

Building libsigc++
------------------

1. Download libsigc++ (http://libsigc.sourceforge.net/), unpack it into mvsc directory. You'll get a directory like "mvsc\libsigc++-2.2.4.2". 
2. Rename it to "mvsc\libsigc++-src". 
3. Build the libsigc++ project with MS Visual Studio 2005 (do Release and Debug builds). 
4. Copy 
	Release\sigc-vc80-2_0.lib, 
	Release\sigc-vc80-2_0.dll, 
	Debug\sigc-vc80-d-2_0.lib,
	Debug\sigc-vc80-d-2_0.dll 
	into "mvsc\libsigc++\".

libsigc++ have static link problem on vs2005 presently.

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
1. Use my_g_fopen instead of g_fopen. See http://bugzilla.gnome.org/show_bug.cgi?id=476810
2. Use fprintf_s instead of fprintf, or it will crash. This is a little strange.

libintl.h redefines *printf functions to libintl_*printf functions.
That may be the cause of the crash.
printf outputs nothing with console attached. Use printf_s instead, or better g_print.

There are two builds in vs2005: Debug and Release, you should choose Release version if you plan to distribute the result, you should choose Debug version for debugging the project.

You should can compile and run stardict successfully now. You cannot start stardict.exe in place, since a special directory structure is needed.

Running StarDict on another computer
------------------------------------

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

StarDict version for MSVC build can be specified in the mvsc\stardictrc.rc.
