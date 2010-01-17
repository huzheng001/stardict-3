@echo off
rem prepares directory structure for NSIS installer (stardict-installer.nsi file)

if not exist stardict-installer.nsi. (
	echo This script must be started in the Stardict source directory.
	goto end
) 
if exist win32-install-dir. (
	echo Directory win32-install-dir already exists, remove it and start this script again.
	goto end
)
mkdir win32-install-dir
copy src\stardict.exe win32-install-dir
copy src\win32\TextOutSpy.dll win32-install-dir
copy src\win32\TextOutHook.dll win32-install-dir
for %%F in (po\*.gmo) do (
	rem %%~nF - pure file name without extension
	mkdir win32-install-dir\locale\%%~nF\LC_MESSAGES
	copy %%F win32-install-dir\locale\%%~nF\LC_MESSAGES\stardict.mo
)
mkdir win32-install-dir\pixmaps
copy pixmaps\stardict.png win32-install-dir\pixmaps
copy src\pixmaps\*.png win32-install-dir\pixmaps
del /f win32-install-dir\pixmaps\docklet_*.png
mkdir win32-install-dir\sounds
copy src\sounds\*.wav win32-install-dir\sounds
mkdir win32-install-dir\dic
mkdir win32-install-dir\treedict
mkdir win32-install-dir\skins

mkdir win32-install-dir\help
mkdir win32-install-dir\plugins
for /D %%D in (stardict-plugins\stardict-*-plugin) do (
	if exist %%D\*.dll. (
		copy %%D\*.dll win32-install-dir\plugins
	)
)

copy src\win32\acrobat\win32\Release\StarDict.api win32-install-dir
echo Done.

:end
