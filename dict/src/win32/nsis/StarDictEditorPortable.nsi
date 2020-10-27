;This file is based on PidginPortableU.nsi from PidginPortable project
;See http://PortableApps.com/PidginPortable
;path to file: PortableApps\PidginPortable\Other\Source\PidginPortableU.nsi

;Website: http://stardict-4.sourceforge.net

;This program is free software; you can redistribute it and/or
;modify it under the terms of the GNU General Public License
;as published by the Free Software Foundation; either version 2
;of the License, or (at your option) any later version.

;This program is distributed in the hope that it will be useful,
;but WITHOUT ANY WARRANTY; without even the implied warranty of
;MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;GNU General Public License for more details.

;You should have received a copy of the GNU General Public License
;along with this program; if not, write to the Free Software
;Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

!define NAME "StarDictEditorPortable"
!define PORTABLEAPPNAME "StarDict Editor Portable"
!define APPNAME "StarDict Editor"
!define VER "3.0.7.0"
!define WEBSITE "stardict-4.sourceforge.net"
!define DEFAULTEXE "stardict-editor.exe"
!define DEFAULTAPPDIR "StarDict"
!define DEFAULTGTKDIR "GTK"
!define DEFAULTSETTINGSDIR "settings"
!define LAUNCHERLANGUAGE "English"

;=== Program Details
Name "${PORTABLEAPPNAME}"
OutFile "${NAME}.exe"
Caption "${PORTABLEAPPNAME} | PortableApps.com"
VIProductVersion "${VER}"
VIAddVersionKey ProductName "${PORTABLEAPPNAME}"
VIAddVersionKey Comments "Allows ${APPNAME} to be run from a removable drive.  For additional details, visit ${WEBSITE}"
VIAddVersionKey CompanyName "PortableApps.com"
VIAddVersionKey LegalCopyright "John T. Haller"
VIAddVersionKey FileDescription "${PORTABLEAPPNAME}"
VIAddVersionKey FileVersion "${VER}"
VIAddVersionKey ProductVersion "${VER}"
VIAddVersionKey InternalName "${PORTABLEAPPNAME}"
VIAddVersionKey LegalTrademarks "PortableApps.com is a Trademark of Rare Ideas, LLC."
VIAddVersionKey OriginalFilename "${NAME}.exe"
;VIAddVersionKey PrivateBuild ""
;VIAddVersionKey SpecialBuild ""

;=== Runtime Switches
CRCCheck on
WindowIcon off
SilentInstall silent
AutoCloseWindow true
RequestExecutionLevel user
XPStyle on

; Best Compression
SetCompress Auto
SetCompressor /SOLID lzma
SetCompressorDictSize 32
SetDatablockOptimize On

;=== Include
;(Standard NSIS)
!include FileFunc.nsh
!insertmacro GetParameters
!insertmacro GetParent
!insertmacro GetRoot
!include LogicLib.nsh

;=== Program Icon
Icon "..\..\..\pixmaps\stardict-editor.ico"

;=== Languages
LoadLanguageFile "${NSISDIR}\Contrib\Language files\${LAUNCHERLANGUAGE}.nlf"
!include PortableApps.comLauncherLANG_${LAUNCHERLANGUAGE}.nsh

Var PROGRAMDIRECTORY
Var EXECSTRING
Var PROGRAMEXECUTABLE
Var MISSINGFILEORPATH

Section "Main"
	StrCpy "$PROGRAMDIRECTORY" "$EXEDIR\App\${DEFAULTAPPDIR}"
	StrCpy "$PROGRAMEXECUTABLE" "${DEFAULTEXE}"

	CheckForEXE:
		IfFileExists "$PROGRAMDIRECTORY\$PROGRAMEXECUTABLE" FoundProgramEXE
	
	NoProgramEXE:
		;=== Program executable not where expected
		StrCpy $MISSINGFILEORPATH $PROGRAMEXECUTABLE
		MessageBox MB_OK|MB_ICONEXCLAMATION `$(LauncherFileNotFound)`
		Goto TheRealEnd
		
	FoundProgramEXE:
		StrCpy $EXECSTRING `"$PROGRAMDIRECTORY\$PROGRAMEXECUTABLE"`

	LaunchAndExit:
		Exec $EXECSTRING
		Goto TheRealEnd

	TheRealEnd:
SectionEnd
