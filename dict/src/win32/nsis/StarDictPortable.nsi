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

!define NAME "StarDictPortable"
!define PORTABLEAPPNAME "StarDict Portable"
!define APPNAME "StarDict"
!define VER "3.0.7.0"
!define WEBSITE "stardict-4.sourceforge.net"
!define DEFAULTEXE "stardict.exe"
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
Icon "..\..\..\pixmaps\stardict.ico"

;=== Languages
LoadLanguageFile "${NSISDIR}\Contrib\Language files\${LAUNCHERLANGUAGE}.nlf"
!include PortableApps.comLauncherLANG_${LAUNCHERLANGUAGE}.nsh

Var PROGRAMDIRECTORY
Var SETTINGSDIRECTORY
Var ADDITIONALPARAMETERS
Var EXECSTRING
Var PROGRAMEXECUTABLE
Var ISDEFAULTDIRECTORY
Var PIDGINLANGUAGE
Var MISSINGFILEORPATH
Var SECONDARYLAUNCH
Var ALLOWMULTIPLEINSTANCES

Section "Main"
	StrCpy "$ADDITIONALPARAMETERS" ""
	StrCpy "$ALLOWMULTIPLEINSTANCES" "false"
	
	StrCpy "$PROGRAMDIRECTORY" "$EXEDIR\App\${DEFAULTAPPDIR}"
	StrCpy "$SETTINGSDIRECTORY" "$EXEDIR\Data\${DEFAULTSETTINGSDIR}"
	StrCpy "$ISDEFAULTDIRECTORY" "true"
	StrCpy "$PROGRAMEXECUTABLE" "${DEFAULTEXE}"

		;=== Check if already running
		System::Call 'kernel32::CreateMutexA(i 0, i 0, t "${NAME}") i .r1 ?e'
		Pop $0
		StrCmp $0 0 CheckIfOtherRunning
			StrCpy $SECONDARYLAUNCH "true"
			Goto CheckForEXE
			
	CheckIfOtherRunning:
		${If} $ALLOWMULTIPLEINSTANCES == "false"
			FindProcDLL::FindProc "stardict.exe"
			${If} $R0 == "1" 
				MessageBox MB_OK|MB_ICONINFORMATION `$(LauncherAlreadyRunning)`
				Goto TheRealEnd
			${EndIf}
			FindProcDLL::FindProc "stardict-portable.exe"
			${If} $R0 == "1" 
				MessageBox MB_OK|MB_ICONINFORMATION `$(LauncherAlreadyRunning)`
				Goto TheRealEnd
			${EndIf}
		${EndIf}
		
		
	CheckForEXE:
		IfFileExists "$PROGRAMDIRECTORY\$PROGRAMEXECUTABLE" FoundProgramEXE
	
	NoProgramEXE:
		;=== Program executable not where expected
		StrCpy $MISSINGFILEORPATH $PROGRAMEXECUTABLE
		MessageBox MB_OK|MB_ICONEXCLAMATION `$(LauncherFileNotFound)`
		Goto TheRealEnd
		
	FoundProgramEXE:
		Goto SkipSplashScreen

	SkipSplashScreen:
		;=== Get any passed parameters
		${GetParameters} $0
		StrCmp "'$0'" "''" "" LaunchProgramParameters

		;=== No parameters
		StrCpy $EXECSTRING `"$PROGRAMDIRECTORY\$PROGRAMEXECUTABLE" --portable-mode --dirs-config=..\..\Data\settings\stardict-dirs.cfg`
		Goto AdditionalParameters

	LaunchProgramParameters:
		StrCpy $EXECSTRING `"$PROGRAMDIRECTORY\$PROGRAMEXECUTABLE" --portable-mode --dirs-config=..\..\Data\settings\stardict-dirs.cfg $0`

	AdditionalParameters:
		StrCmp $ADDITIONALPARAMETERS "" PidginEnvironment

		;=== Additional Parameters
		StrCpy $EXECSTRING `$EXECSTRING $ADDITIONALPARAMETERS`

	PidginEnvironment:
		StrCmp $SETTINGSDIRECTORY "" PidginSettingsNotFound
		IfFileExists "$SETTINGSDIRECTORY\*.*" CheckForGTKRC
	
	PidginSettingsNotFound:
		StrCmp $ISDEFAULTDIRECTORY "true" SetupDefaultSettings
		StrCpy $MISSINGFILEORPATH $SETTINGSDIRECTORY
		MessageBox MB_OK|MB_ICONEXCLAMATION `$(LauncherFileNotFound)`
		Abort

	SetupDefaultSettings:
		CreateDirectory "$EXEDIR\Data\settings"
		CopyFiles /SILENT $EXEDIR\App\DefaultData\settings\*.* $EXEDIR\Data\settings
		
	CheckForGTKRC:
		IfFileExists "$SETTINGSDIRECTORY\gtkrc" CheckForStarDictDirsCfg
			CopyFiles /SILENT $EXEDIR\App\DefaultData\settings\gtkrc $EXEDIR\Data\settings\gtkrc
	
	CheckForStarDictDirsCfg:
		IfFileExists "$SETTINGSDIRECTORY\stardict-dirs.cfg" PidginSettingsFound
			CopyFiles /SILENT $EXEDIR\App\DefaultData\settings\stardict-dirs.cfg $EXEDIR\Data\settings\stardict-dirs.cfg
	
	PidginSettingsFound:
		StrCmp $SECONDARYLAUNCH "true" LaunchNow
		ReadEnvStr $PIDGINLANGUAGE "PortableApps.comLocaleglibc"
		StrCmp $PIDGINLANGUAGE "" PidginLanguageSettingsFile
		StrCmp $PIDGINLANGUAGE "en_US" SetPidginLanuageVariable
		IfFileExists "$PROGRAMDIRECTORY\locale\$PIDGINLANGUAGE\*.*" SetPidginLanuageVariable PidginLanguageSettingsFile

	PidginLanguageSettingsFile:
		ReadINIStr $PIDGINLANGUAGE "$SETTINGSDIRECTORY\${NAME}Settings.ini" "Language" "STARDICTLANG"
		ClearErrors ;if any
		StrCmp $PIDGINLANGUAGE "" LaunchNow
		StrCmp $PIDGINLANGUAGE "en_US" SetPidginLanuageVariable
		IfFileExists "$PROGRAMDIRECTORY\locale\$PIDGINLANGUAGE\*.*" SetPidginLanuageVariable LaunchNow

	SetPidginLanuageVariable:
		System::Call 'Kernel32::SetEnvironmentVariable(t, t) i("STARDICTLANG", "$PIDGINLANGUAGE").r0'
	
	LaunchNow:
		StrCmp $SECONDARYLAUNCH "true" LaunchAndExit LaunchAndWait

	LaunchAndWait:
		Rename "$SETTINGSDIRECTORY\gtkrc" "$PROGRAMDIRECTORY\Gtk\etc\gtk-2.0\gtkrc"
		ExecWait $EXECSTRING
		Goto TheEnd

	LaunchAndExit:
		Exec $EXECSTRING
		Goto TheRealEnd

	TheEnd:
	
	TheRealEnd:
SectionEnd
