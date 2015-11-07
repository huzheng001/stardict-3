# How to build StarDict #



# Linux #


Build stardict-`*`.tar.bz2

```bash

./autogen.sh
./configure --prefix=/usr --sysconfdir=/etc --mandir=/usr/share/man --disable-gucharmap --disable-dictdotcn --disable-festival
make dist-bzip2
```

Build stardict-`*`.rpm

```bash

cp dict/stardict.spec /root/rpmbuild/SPECS/
cp stardict-*.tar.bz2 /root/rpmbuild/SOURCES/
cd /root/rpmbuild/SPECS/
rpmbuild -bb stardict.spec
```

Build stardict-`*`.deb

```bash

cp -r dict/debian-upstream debian
dpkg-buildpackage -rfakeroot
```


To Install StarDict on Linux(or other Unix-like platform), type:

```bash

./configure --prefix=/usr --sysconfdir=/etc --mandir=/usr/share/man
make
make install
```

If compile festival plugin failed, you can add "--disable-festival" option to configure command.

You can use:

```bash

./configure --disable-gnome-support --prefix=/usr
```

to build the "gtk library only" version of StarDict.

You can use

```bash

./configure --enable-gpe-support --prefix=/usr
```

to build the GPE version of StarDict. For GPE, see http://gpe.handhelds.org

You can use
```bash

./configure --enable-maemo-support --prefix=/usr
```

to build the Maemo version of StarDict. For Maemo, see http://www.maemo.org

StarDict does not support staged installs, that is you may not use DESTDIR variable to install the application into a custom place like this:

```bash

make DESTDIR=/custom/path install
```

StarDict needs to know at compile time all details of the installation otherwise it would not be able to find its data and it'd fail to start. If you need to change installation dirs use configure parameters like --prefix.

Notice: you can download dictionaries on the StarDict forum:
http://www.stardict.org/forum

## Ubuntu ##

_Tested on Ubuntu 11.10_

Assume we want to build development version of stardict, so the installation path will be $HOME/lab.

To build StarDict, we need following packages installed.

```bash

sudo apt-get install build-essential automake libtool libmysqlclient-dev libpcre3-dev
sudo apt-get install gnome-common libgconf2-dev libenchant-dev libgucharmap2-dev libespeak-dev libgnomeui-dev festival-dev
```

Preparing Makefiles. Looks like we might need prepare /dict seperately.

```bash

./autogen.sh
./configure --prefix=$HOME/lab
cd dict
./autogen.sh
./configure --prefix=$HOME/lab
cd ..
```

Then we can build and install.

```bash

make
make install
```
## Fedora ##

# Mac OS X #

StarDict have been ported to Mac OS X now.

Here is a abstract introduction of compiling stardict and building the package.

  1. Install Xcode.

> 2. Install fink.

http://www.finkproject.org
This should be optional.

> 3. Install jhbuild.

http://developer.imendio.com/projects/gtk-macosx

http://developer.imendio.com/projects/gtk-macosx/build-instructions

And build gtk by following the instructions.

> 4. Compile stardict.

Edit configure.in, remove the "AM\_GCONF\_SOURCE\_2" lines.

```bash

./autogen.sh
export PKG_CONFIG=/opt/gtk/bin/pkg-config
./configure --prefix=/opt/gtk --disable-festival --disable-espeak --disable-gucharmap \
--disable-spell --enable-darwin-support --mandir=/opt/gtk/share/man
make
make install
```

> 5. Create the install package.

Read documents in http://gimp-app.sourceforge.net/

You can take the existing `StarDict.pkg` for the example.

To get the gtk library files, you can do:

```bash

cd ~/Source/gnome/gtk+/;make DESTDIR=~/stardict-root install
```

> and so on.

After Created the package directory, you can use `PackageMaker` to build it.

# FreeBSD #

# Windows #

StarDict can be compiled and run in windows.

## Building StarDict ##

Use MS Visual Studio 2008 sp1 to build StarDict.

### GTK ###

You need to install developer packages into "msvc\_2008\gtk" directory.
Download All-in-one bundle `gtk+-bundle_2.22.*_win32.zip` at http://www.gtk.org/download-windows.html.

Note. bundle gtk+-bundle\_2.22.1-20101227\_win32.zip
You may notice the following warning when building StarDict:
```
zdll.lib(d000034.o) : warning LNK4078: multiple '.text' sections found with different attributes (E0300020)
```
The zdll library that comes with this bundle should be replaced. Try this  http://ftp.gnome.org/pub/gnome/binaries/win32/dependencies/zlib-dev_1.2.5-1_win32.zip. Unpack it into "msvc\_2008\gtk" directory overwritting all files.

Download libiconv-1.9-w32.2.bin.woe32.zip at http://sourceforge.net/projects/gettext/files/.
Extract them into "msvc\_2008\gtk\".

You may need to download the gettext-tools-0.13.1.bin.woe32.zip file too!

### libxml library ###

stardict-editor project depends on libxml library (http://xmlsoft.org/).
If you plan to build only StarDict, you do not need this library.

To build stardict-editor with libxml2 library you have two options: you may use either prebuild libxml2 binaries by  Igor Zlatković. or build libxml2 from source. With prebuild binaries you'll be only able to build release version of stardict-editor. If you build libxml library from source, you'll be able to build both Release and Debug versions of stardict-editor.

See http://www.zlatkovic.com/libxml.en.html for details about libxml2 library on Windows.

#### I. building with prebuild binaries ####

1. Download `libxml2-*.win32.zip` from [ftp://ftp.zlatkovic.com/libxml/](ftp://ftp.zlatkovic.com/libxml/)

2. Unpack into msvc\_2008, you'll get a `msvc_2008/libxml2-*.win32` folder.

3. Rename `libxml2-*.win32` to libxml2.

4. Rename msvc\_2008/libxml2/lib to msvc\_2008/libxml2/lib.release

#### II. building libxml library from source ####

1. download libxml archive (for example libxml2-2.7.7.tar.gz) from [ftp://xmlsoft.org/libxml2/](ftp://xmlsoft.org/libxml2/)

> Note, I recommend against libxml2-2.7.8.tar.gz, it seems to be broken.

2. extract the archive into msvc\_2008, rename `msvc_2008\libxml2-*` to msvc\_2008\libxml2-src

3. Windows menu->All Programs->Microsoft Visual Studio 2008->Visual Studio Tools->Visual Studio 2008 Command Prompt

4. cd to msvc\_2008\libxml2-src\win32 in the opened console window.

5. Run "cscript configure.js help" to get a listing of all the build options (for your information only).

6. Building release version with default options.
```
cscript configure.js include=../../gtk/include lib=../../gtk/lib
nmake /f Makefile.msvc
```
you may see multiple warning like:
```
c14n.c
c:\stardict\msvc_2008\libxml2-src\libxml.h(94) : warning C4005: 'LIBXML_STATIC' : macro redefinition
        command-line arguments : see previous definition of 'LIBXML_STATIC'
```

I think they can be ignored.

7. build results are in msvc\_2008\libxml2-src\win32\bin.msvc

8. copy msvc\_2008\libxml2-src\win32\bin.msvc\libxml2\_a.lib to msvc\_2008\libxml2\lib.release\libxml2\_a.lib

9. remove msvc\_2008\libxml2-src folder, than repeat step 2 to recreate this folder

10. buiding a debug version
```
cscript configure.js cruntime=/MDd include=../../gtk/include lib=../../gtk/lib
nmake /f Makefile.msvc
```

11. copy msvc\_2008\libxml2-src\win32\bin.msvc\libxml2\_a.lib to msvc\_2008\libxml2\lib.debug\libxml2\_a.lib

12. copy msvc\_2008\libxml2-src\include to msvc\libxml2\include

### stardict-editor project configuration for libxml ###


stardict-editor MS Visual C++ project is already configured to build with libxml2. You do not need to change settings unless you want a custom build.

stardict-editor links libxml statically. You need to adjust project settings as follows to build the project.

1. link with libxml2\_a.lib library

2. define the following macros LIBXML\_STATIC, LIBXSLT\_STATIC, LIBEXSLT\_STATIC and XMLSEC\_STATIC in project settings. These macros must be globally defined!

3. libxml2 library and the project must use the same version of the c-runtime library. It may be multi-threaded vs. single-threaded, release vs. debug, linked statically or dynamically.

### Building libsigc++ ###


1. Download libsigc++ (http://libsigc.sourceforge.net/), unpack it into msvc\_2008 directory. You'll get a directory like "msvc\_2008\libsigc++-2.2.4.2".

2. Rename it to "msvc\_2008\libsigc++-src".

3. Build the libsigc++ project with MS Visual Studio 2008 (do Release and Debug builds).

4. Copy
> Release\sigc-vc90-2\_0.lib,

> Release\sigc-vc90-2\_0.dll,

> Debug\sigc-vc90-d-2\_0.lib,

> Debug\sigc-vc90-d-2\_0.dll

> into "msvc\_2008\libsigc++\".

libsigc++ have static link problem on vs2008 (?).

### sapi-tts plugin ###

For sapi-tts plugin, you need to install Microsoft Speech SDK. Download SpeechSDK51.exe file. Install it into "C:\Program Files\Microsoft Speech SDK 5.1" (into "C:\Program Files (x86)\Microsoft Speech SDK 5.1" on x64 platform).

Fix these compile errors in sphelper.h (only for x86):

1. add to the top of the file to prevent many errors
```cpp

#pragma warning( disable : 4430 )
#pragma warning( disable : 4996 )
```

2. line 2372

> replace

```cpp

for (const WCHAR * psz = (const WCHAR *)lParam; *psz; psz++) {}
```

> with

```cpp

const WCHAR * psz;
for (psz = (const WCHAR *)lParam; *psz; psz++) {}
```

3. line 2560

> replace

```cpp

SPPHONEID* pphoneId = dsPhoneId;
```

> with

```cpp

SPPHONEID* pphoneId = (SPPHONEID*)((WCHAR *)dsPhoneId);
```

4. line 2634

> replace

```cpp

pphoneId += wcslen(pphoneId) + 1;
```

> with

```cpp

pphoneId += wcslen((const wchar_t *)pphoneId) + 1;
```


Fix these compile warnings in sphelper.h (only for x64):

1. line 1194

> replace

```cpp

if( ( ::GetVersionEx( &ver ) == TRUE ) && ( ver.dwMajorVersion >= 6 ) )
```

> with

```cpp

if( ( ::GetVersionEx( &ver ) ) && ( ver.dwMajorVersion >= 6 ) )
```

### Acrobat wordpick plugin ###

See stardict\src\win32\acrobat\readme.txt file.

### Notes ###

For stardict\_powerword\_parsedata.cpp, you need to add a UTF-8 BOM in its head to fix the compile problem. Just use the notepad to open it then save.

For wordnet plugin files, they are the same.

(8 May 2010. Compilation succeeds without the fix. It is not needed anymore?)

There are two crash bug with vs2005 (and likely with vs2008), which you need to notice.

1. Use stardict\_g\_fopen instead of g\_fopen. See http://bugzilla.gnome.org/show_bug.cgi?id=476810

2. Use fprintf\_s instead of fprintf, or it will crash. This is a little strange.

libintl.h redefines `*printf` functions to `libintl_*printf` functions.
That may be the cause of the crash.
printf outputs nothing with console attached. Use printf\_s instead, or better g\_print.

There are two builds in vs2008: Debug and Release, you should choose Release version if you plan to distribute the result, you should choose Debug version for debugging the project.

You should can compile and run stardict successfully now. You cannot start stardict.exe in place, since a special directory structure is needed.

## Solution structure ##

The StarDict solution consists of the following projects:

  * stardict (stardict.dll) - a GUI part of StarDict packed into a dynamic library
  * stardict-loader (stardict.exe) - an executable file that starts StarDict
  * stardict-lib (stardict.lib) - a static library containing a non-GUI part of StarDict
  * textouthook and textoutspy (textouthook.dll and textoutspy.dll) - implement scan words feature
  * plugins - plugins loaded by StarDict and extending it functionality
  * tests - test for StarDict
  * tools/stardict-editor (stardict-editor.exe) - a GUI application providing auxiliary tools
  * acrobat-wordpick-plugin - Adobe Acrobat plugin for scan words feature

Code of StarDict application is divided into a number of projects: stardict-lib, stardict, stardict-loader. stardict-lib mainly contains non-GUI code. It was put into a library in order to facilitate testing. The tests solution folder holds test projects, they normaly link the stardict-lib library. The GUI part of the code was put in the stardict project. The division into GUI and non-GUI parts is not strict. Feel free to move code from stardict to stardict-lib and back if needed. stardict and stardict-lib project contains almost all code of the application excluding plugins and auxiliary libraries. stardict-loader is a small project which primary job is to load the StarDict application. stardict-loader inspects OS environment, set up directories, set environment variables and finally loads stardict.dll. stardict-loader should have as few dependencies as possible, it must not depend on GTK for example. Now stardict-loader only uses C Runtime Library and Windows API functions. One of the tasks of stardict-loader is to find GTK directory and add to the dll load path, so stardict.dll can be successfully started. This project produces stardict-loader.exe executable file, it will be renamed to stardict.exe in release version. Configuring the project to output stardict.exe leads to debugging problems is Visual Studio.


## Unicode ##

It's recommended to build Unicode (versus ANSI) version of StarDict. That's done by default. You need to define UNICODE macro in compiler command line to build Unicode version.

## Build the installer ##

Grab and install NSIS: http://nsis.sourceforge.net/Download
I was using the 2.46 version.

Use stardict-prepare-installer.js to build win32-install-dir directory structure. You must have all the required files in place before running the script. See comment at the head of the file for details of running the script.

Some of the required files cannot be created on Windows, you need Linux to prepare them.

1. Download `gtk2-runtime-*.exe` from http://sourceforge.net/projects/gtk-win and put it into stardict\redist.

2. Download Microsoft Visual C++ 2008 Redistributable Package and put into redist. The file is named vcredist\_x86.exe.
> VS 2008 pre-SP1 version:
> http://www.microsoft.com/downloads/en/details.aspx?FamilyID=9b2da534-3e03-4391-8a4d-074b9f2bc1bf&displaylang=en
> VS 2008 SP1 version:
> http://www.microsoft.com/downloads/en/details.aspx?familyid=A5C84275-3B97-4AB7-A40D-3802B2AF5FC2&displaylang=en

3. From %PROGDIR%\Microsoft Visual Studio 8\VC\Redist\x86, copy Microsoft.VC80.CRT to redist.

Double click the stardict-installer.nsi :) NSIS will build the installer.

### Warning ###

The libraries for Visual Studio often are updated after a release. For example, this can occur when you install a service pack. Visual Studio SP1 installs new version of C Runtime library as well as other libraries. Redistributable files in %PROGDIR%\Microsoft Visual Studio 8\VC\Redist\x86 are updated too. However, by default Visual Studio still bind applications to the original release version of libraries available. That is, after installing SP1, application built with Visual Studio require the libraries that were originally installed with Visual Studio 2008! For details see "Redistributing an Application and Binding It to Specific Libraries" at http://msdn.microsoft.com/en-us/library/cc664727.aspx.

In practice that means that after installing SP1.

1. You should continue to use vcredist\_x86.exe for Visual Studio 2008.

2. %PROGDIR%\Microsoft Visual Studio 8\VC\Redist\x86 folder now contains files that do not satisfy dependency requirements of produced applications. You need the pre-SP1 files.

### Note ###

vcredist\_x86.exe for Visual Studio 2008 is affected by an annoying bug. It extracts temporary files into the root of the drive and does not remove them.

You will see the following files in the root of the drive after executing vcredist\_x86.exe:
```
eula.1028.txt
eula.1031.txt
eula.1033.txt
eula.1036.txt
eula.1040.txt
eula.1041.txt
eula.1042.txt
eula.2052.txt
eula.3082.txt
globdata.ini
install.exe
install.ini
install.res.1028.dll
install.res.1031.dll
install.res.1033.dll
install.res.1036.dll
install.res.1040.dll
install.res.1041.dll
install.res.1042.dll
install.res.2052.dll
install.res.3082.dll
VC_RED.cab
VC_RED.MSI
vcredist.bmp
```

This bug was fixed in vcredist\_x86.exe for Visual Studio 2008 SP1 (vcredist\_x86-sp1.exe for short). vcredist\_x86-sp1.exe installs libraries required to run applications built with Visual Studio 2008 no-SP1. Thus we may safely use vcredist\_x86-sp1.exe instead of vcredist\_x86.exe. The only drawback of that replacement is extra space needed for new libraries that are not used, vcredist\_x86-sp1.exe (4.0 MiB) is larger than vcredist\_x86.exe (1.7 MiB).


Despite the extra space required I recommend to use vcredist\_x86-sp1.exe.

### Deploying Visual C++ library with StarDict ###

StarDict application as well as related tools and DLLs depend upon Visual C++ libraries that must be installed on the target computer in order for application to start. The following libraries are needed: C Runtime Library, Standard C++ Library. They both are part of the Microsoft.VC80.CRT assembly.

There are two way to distribute Visual C++ DLLs.

1. Using Visual C++ Redistributable Package (VCRedist\_x86.exe) to install libraries into global assembly cache. This methods requires administrative rights.

2. Installing Microsoft.VC80.CRT as a private assembly in application's folder.

Whenever possible we should install Visual C++ libraries into the native assembly cache (WinSxS folder). That makes the libraries available to all applications no matter where they are installed. In some cases we do not have administrative rights or we should not alter the target system as with the portable version of StarDict. We have to use the second method. Privite assembly are subject of restrictions. We are not free to choose the folder where the private assembly is placed into. That must either the application folder or a subfolder of it named after the assembly name. Of two variants a subfolder in StarDict folder seams the best one - no extra DLLs near stardict.exe. We may have a directory structure like this:

```
$INSTALLDIR\stardict.exe
$INSTALLDIR\stardict-editor.exe
$INSTALLDIR\sigc-vc90-2_0.dll
$INSTALLDIR\textouthook.dll
$INSTALLDIR\textoutspy.dll
...
$INSTALLDIR\Microsoft.VC90.CRT\Microsoft.VC90.CRT.manifest
$INSTALLDIR\Microsoft.VC90.CRT\msvcm90.dll
$INSTALLDIR\Microsoft.VC90.CRT\msvcp90.dll
$INSTALLDIR\Microsoft.VC90.CRT\msvcr90.dll
...
```

Having Microsoft.VC90.CRT installed this way, Visual C++ libraries will be successfully loaded by stardict.exe, stardict-editor.exe and other DLLs in the "$INSTALLDIR" folder, but not by stardict plugins residing in $INSTALLDIR\plugins folder. The reason for this is "Assembly Searching Sequence", see
http://msdn.microsoft.com/en-us/library/aa374224(VS.85).aspx for more details.

There are a number of possibilites to make plugins load successfully.

1. Copy the Microsoft.VC90.CRT folder into plugins. That is we'll have two copies of the Microsoft.VC90.CRT assembly: the first in the $INSTALLDIR folder and the second in the $INSTALLDIR\plugins folder. This make StarDict to load two different copies of the C Runtime and Standard C++ Libraries. The first copy will be loaded by StarDict itself, and the second will be loaded by plugins, hopefully all plugins will share one copy of each library.

This solution is not acceptable for two reasons. 1) Multiple copies of the sample library in process address space, 2) extra disk space required to store two copies of the same file.

2. Link all plugins with C Runtime and Standard C++ libraries statically. In this case each plugin will have its own copy of the library. That is not acceptable again.

3. Move all plugins from the plugins folder directly to $INSTALLDIR. This clutters StarDict folder, but should work OK with a single copy of assembly.

4. Remove reference to Microsoft.VC90.CRT assembly out of plugin manifests. Since Microsoft.VC90.CRT is the only assembly in plugin minifests we can drop the manifest entirely. This make plugin use the copy of C Runtime and Standard C++ Libraries loaded by StarDict.exe. Plugins are only loaded by stardict.exe so we can be sure both libraries present in the process address space.

To disable embedding a manifest into a DLL follow these steps:

  * In Visual Studio open project properties.
  * In configuration select "All Configurations"
  * Navigate to Configuration Properties -> Linker -> Manifest File in the tree on the left.
  * Set "Generate Manifest" property to "No".

### StarDict Portable installer ###

To build StarDict Portable installer you need to install additional programs and components.

  * Install [PortableApps.com Platform](http://portableapps.com/download). You need only Platform, but you may install any Suite if you want.
    * Use "`C:\PortableApps`" as installation directory. You may use you other installation directory, but in that case you need to pass portinst parameter to stardict-prepare-install.js script.
  * Install [PortableApps.com Installer](http://portableapps.com/apps/development/portableapps.com_installer)
  * Install [FindProcDLL NSIS plug-in](http://nsis.sourceforge.net/FindProcDLL_plug-in)
    * Download `KillProcDll&FindProcDll.zip` from http://nsis.sourceforge.net/FindProcDLL_plug-in
    * Unpack the archive into "C:\Program Files\NSIS\Plugins" folder. The archive contains two DLLs: `FindProcDLL.dll` and `KillProcDLL.dll`.

To build StarDict Portable installer, execute stardict-prepare-install.js with portable=yes parameter.

## Debugging ##

For debug, use these codes:

```cpp

FILE *file;
file=fopen("C:\\a.txt", "a+");
fprintf(file, "a\n");
fclose(file);
```

StarDict sends debug messages, warnings, errors to console. Windows GUI applications do not attach to a console by default, hence all the aforementioned messages go nowhere.

There are to ways to make the output visible. You may attach StarDict to a windows console or open a log window. Both release and debug versions of StarDict allow to use console, while the log window is only available in the debug version.

To open window console pass --message-level option with non-zero parameter. The parameter specifies the amount of output to produce. The larger the value, the more verbose output will be. See StarDict help for more details.
The --help option opens console as well. It prints a usage message and exits the application.

If you start StarDict with a shortcut, a new console windows is opened. It is closed with the application. If you start StarDict in an existing console window, the program do not open a new window, but prints all output to the existing one. Opening a new console has the disadvantage that
if the application crushs, the console window closes so quickly you cannot read the error message.

Windows console has a limitation, it cannot show all unicode characters.
To overcome this limitation StarDict can show output in a log window.
The log window has no problems with showing unicode characters, but it can not fully
duplicate console output. StarDict can show output of g\_debug, g\_error, g\_warning and the like, g\_print, g\_printf(?) functions. You'll not see output from printf in the log window, for example. To enable the log window, build StarDict with ENABLE\_LOG\_WINDOW macro symbol.

All that windows console stuff is not much reliable, if it does not work, retreat to printing into a file or to the log window.


## Running StarDict on another computer ##

You may encounter a problem running StarDict project compiled with VS 2008 on other computer not having VS 2008. You may see the following rather unclear error message "This application has failed to start because the application configuration is incorrect. Reinstalling the application may fix this problem." The error happens because StarDict application, plugin DLLs, hook DLLs are linked with CRT (C runtime library) dynamically and you may not have the appropriate version on that dll on the target computer.

If you build Release version of the project, you need to download and install Microsoft Visual C++ 2008 Redistributable Package (x86).

If you build Debug version of the project, you need a debug version of CRT which not redistributable. I see at least one reason why you may need to use the debug version of StarDict - to debug the project remotely. Microsoft prohibit do distribute the debug version of CRT, but I assume a developer may do that for debuging purpose. Anyway this how to install the debuging version of CRT onto the target machine.

1. In Visual Studio create new Setup Project, name it Setup-CRTDebug.

2. Right-click the project in the Solution Explorer, select Add -> Merge module.

3. In the file selection dialog select "C:\Program Files\Common Files\Merge Modules\Microsoft\_VC80\_DebugCRT\_x86.msm".

4. Build the project.

5. You've created Setup-CRTDebug.msi setup file. Run it on the target machine. It installs debug version of CRT. In the course of executing you'll be asked to specify an installation directory somewhere under "C:\Program Files". Never mind. The installer does not copy anything there. It does not even create that folder.

6. Now you should be able to start StarDict.

## StarDict version ##

StarDict version for MSVC build can be specified in the msvc\_2008\stardictrc.rc.

## Two copies of CRT ##

When StarDict is build with MS Visual Studio,
the resulting application has two copies (maybe more) of C Runtime library.
The first copy comes with GTK+ libraries, the second is supplied by Visual Studio.
The C Runtime library from Visual Studio resides in msvcr90.dll, that is a part of
Microsoft.VC90.CRT assembly. Where is C Runtime Library that comes with GTK+ library,
I do not know, but it surely can not be in msvcr90.dll. Having two copies of
the library is a headache. There are two copies of environment variables.
Setting an environment variable with function from one library, does not make
that variable set in the other library. For example,

`_wputenv(L"LANG=ru")` - sets LANG variable to ru in MS C Runtime Library

g\_getenv("LANG") - returns the value of the LANG variable in GTK+ C Runtime Library.

This may be any value, _wputenv does not change the environment variable in
GTK+ C Runtime Library._

We may live with two copies of the library pretty fine aside from special cases.
In particular we need to synchronize LANG environment variable.
StarDict Loader sets LANG environment variable to customize GUI interface.
It specifies the language to use in interface. StarDict loader defines LANG
variable in MS CRT only, the GTK+ CRT is not available yet.
If we do nothing about the environment variable LANG in GTK+ CRT library,
gettext library will use the default language for user interface.
In order for StarDict loader to influence the GUI translation we must set
the LANG variable in the GTK+ CRT too.


## Notes ##

StarDict's win32 port got many experience from `ReciteWord` (my another project, http://reciteword.sourceforge.net) and Pidgin: http://www.pidgin.im

Note: when use fopen(), use "rb", never "r" (unless you know what you are doing).

Hu Zheng <huzheng001@gmail.com> http://www.huzheng.org

2009.7.20