; Installer script for win32 StarDict
; Hu Zheng <huzheng001@gmail.com>

;--------------------------------
;Global Variables
Var ISSILENT

;Configuration

;General

!define STARDICT_VERSION		"3.0.7"

Name "StarDict ${STARDICT_VERSION}"

OutFile "stardict-${STARDICT_VERSION}.exe"

XPStyle on

SetCompressor /SOLID lzma
ShowInstDetails show
ShowUninstDetails show
SetDateSave on
RequestExecutionLevel highest

; $INSTDIR is set in .onInit function..

!include "MUI.nsh"
!include "Sections.nsh"

!include "FileFunc.nsh"
!insertmacro GetParent

!include "WordFunc.nsh"
!insertmacro VersionCompare

;--------------------------------
;Defines

!define MUI_ICON                     .\pixmaps\stardict.ico
!define MUI_UNICON                   .\pixmaps\stardict-uninst.ico
!define MUI_SPECIALBITMAP            .\src\win32\nsis\stardict-intro.bmp
!define MUI_HEADERBITMAP             .\src\win32\nsis\stardict-header.bmp
!define MUI_FINISHPAGE_NOAUTOCLOSE

!define STARDICT_NSIS_INCLUDE_PATH   ".\src\win32\nsis"

!define STARDICT_REG_KEY             "SOFTWARE\stardict"
!define STARDICT_UNINSTALL_KEY       "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\StarDict"
!define HKLM_APP_PATHS_KEY           "SOFTWARE\Microsoft\Windows\CurrentVersion\App Paths\stardict.exe"
!define STARDICT_UNINST_EXE          "stardict-uninst.exe"

!define GTK_RUNTIME_INSTALLER        "redist\gtk2-runtime*.exe"
!define VC_REDIST_INSTALLER          "redist\vcredist_x86.exe"
!define VC_REDIST_DIR                "Microsoft.VC90.CRT"
!define VC_REDIST_ASSEMBLY           "redist\${VC_REDIST_DIR}"
!define LIB_SIGC_DLL                 "sigc-*.dll"


;--------------------------------
;Modern UI Configuration

  !define MUI_CUSTOMPAGECOMMANDS

  !define MUI_WELCOMEPAGE
  !define MUI_LICENSEPAGE
  !define MUI_DIRECTORYPAGE
  !define MUI_FINISHPAGE
  
  !define MUI_ABORTWARNING

  !define MUI_UNINSTALLER
  !define MUI_UNCONFIRMPAGE

;--------------------------------
;Pages
  
  !insertmacro MUI_PAGE_WELCOME
  !insertmacro MUI_PAGE_LICENSE			"../COPYING"

  !insertmacro MUI_PAGE_DIRECTORY
  !insertmacro MUI_PAGE_INSTFILES
  !insertmacro MUI_PAGE_FINISH

  !insertmacro MUI_UNPAGE_CONFIRM
  !insertmacro MUI_UNPAGE_INSTFILES
  !insertmacro MUI_UNPAGE_FINISH

;--------------------------------
;Languages
 
  !insertmacro MUI_LANGUAGE "English"

;--------------------------------
;Language Strings
  LangString VC_REDIST_INSTALL_ERROR	${LANG_ENGLISH} \
		"Error installing Microsoft Visual C++ redistributable package."
 LangString VC_REDIST_NO_RIGHTS_ERROR	${LANG_ENGLISH} \
		"Not enough rights to install Microsoft Visual C++ redistributable package."

  ; StarDict Section Prompts and Texts
  LangString STARDICT_UNINSTALL_DESC		${LANG_ENGLISH} \
		"StarDict (remove only)"
  LangString STARDICT_PROMPT_WIPEOUT		${LANG_ENGLISH} \
		"You're old StarDict directory is about to be deleted. Would you like to continue?$\r$\r \
		StarDict user settings will not be affected."
  LangString STARDICT_PROMPT_DIR_EXISTS		${LANG_ENGLISH} \
		"The installation directory you specified already exists. Any contents $\r \
		it may have will be deleted. Would you like to continue?"

  ; Uninstall Section Prompts
  LangString un.STARDICT_UNINSTALL_ERROR_1         	${LANG_ENGLISH} \
		"The uninstaller could not find registry entries for StarDict.$\r \
		It is likely that another user installed this application."
  LangString un.STARDICT_UNINSTALL_ERROR_2         	${LANG_ENGLISH} \
		"You do not have permission to uninstall this application."

;--------------------------------
;Data
  
  LicenseData "../COPYING"

;--------------------------------
;Uninstall any old version of StarDict

Section -SecUninstallOldStarDict
  ; Check install rights..
  Call CheckUserInstallRights
  Pop $R0

  StrCmp $R0 "HKLM" stardict_hklm
  StrCmp $R0 "HKCU" stardict_hkcu done

  stardict_hkcu:
    ReadRegStr $R1 HKCU ${STARDICT_REG_KEY} ""
    ReadRegStr $R2 HKCU ${STARDICT_REG_KEY} "Version"
    ReadRegStr $R3 HKCU "${STARDICT_UNINSTALL_KEY}" "UninstallString"
    Goto try_uninstall

  stardict_hklm:
    ReadRegStr $R1 HKLM ${STARDICT_REG_KEY} ""
    ReadRegStr $R2 HKLM ${STARDICT_REG_KEY} "Version"
    ReadRegStr $R3 HKLM "${STARDICT_UNINSTALL_KEY}" "UninstallString"

  ; If previous version exists .. remove
  try_uninstall:
    StrCmp $R1 "" done
      StrCmp $R2 "" uninstall_problem
        ; Check if we have uninstall string..
        IfFileExists $R3 0 uninstall_problem
          ; Have uninstall string.. go ahead and uninstall.
          SetOverwrite on
          ; Need to copy uninstaller outside of the install dir
          ClearErrors
          CopyFiles /SILENT $R3 "$TEMP\${STARDICT_UNINST_EXE}"
          SetOverwrite off
          IfErrors uninstall_problem
            ; Ready to uninstall..
            ClearErrors
            ExecWait '"$TEMP\${STARDICT_UNINST_EXE}" /S _?=$R1'
            IfErrors exec_error
              Delete "$TEMP\${STARDICT_UNINST_EXE}"
              Goto done

            exec_error:
              Delete "$TEMP\${STARDICT_UNINST_EXE}"
              Goto uninstall_problem

  uninstall_problem:
    ; In this case just wipe out previous StarDict install dir..
    MessageBox MB_YESNO $(STARDICT_PROMPT_WIPEOUT) IDYES do_wipeout IDNO cancel_install
    cancel_install:
      Quit

    do_wipeout:
      StrCmp $R0 "HKLM" stardict_del_lm_reg stardict_del_cu_reg
      stardict_del_cu_reg:
        DeleteRegKey HKCU ${STARDICT_REG_KEY}
        Goto uninstall_prob_cont
      stardict_del_lm_reg:
        DeleteRegKey HKLM ${STARDICT_REG_KEY}

      uninstall_prob_cont:
        RMDir /r "$R1"

  done:
SectionEnd

;--------------------------------
;Microsoft Visual C++ Redistributable Package

Section SecMSVCRedist
  Call CheckUserInstallRights
  Pop $R1
  StrCmp $R1 "HKLM" install_vcredist
  StrCmp $R1 "HKCU" install_private_assembly no_install_rights
  
  install_vcredist:
    SetOutPath $TEMP
    SetOverwrite on
    File /oname=vcredist.exe ${VC_REDIST_INSTALLER}
    SetOverwrite off
    ClearErrors
    ExecWait '"$TEMP\vcredist.exe" /Q'
    IfErrors vcredist_install_error vcredist_install_success
  
  vcredist_install_error:
    Delete "$TEMP\vcredist.exe"
    MessageBox MB_OK $(VC_REDIST_INSTALL_ERROR) /SD IDOK
    Quit
  
  vcredist_install_success:
    Delete "$TEMP\vcredist.exe"
    Goto done
  
  install_private_assembly:
    SetOutPath "$INSTDIR"
    SetOverwrite on
    File /r "${VC_REDIST_ASSEMBLY}"
    SetOverwrite off
    Goto done

  no_install_rights:
    MessageBox MB_OK $(VC_REDIST_NO_RIGHTS_ERROR) /SD IDOK
    Quit
  
  done:
SectionEnd ; end of Microsoft Visual C++ Redistributable Package

;--------------------------------
;StarDict Install Section

Section SecStarDict
  SectionIn 1 RO

  ; Check install rights..
  Call CheckUserInstallRights
  Pop $R0

  StrCmp $R0 "NONE" stardict_none
  StrCmp $R0 "HKLM" stardict_hklm stardict_hkcu

  stardict_hklm:
    WriteRegStr HKLM ${STARDICT_REG_KEY} "" "$INSTDIR"
    WriteRegStr HKLM ${STARDICT_REG_KEY} "Version" "${STARDICT_VERSION}"
    WriteRegStr HKLM "${STARDICT_UNINSTALL_KEY}" "DisplayName" $(STARDICT_UNINSTALL_DESC)
    WriteRegStr HKLM "${STARDICT_UNINSTALL_KEY}" "UninstallString" "$INSTDIR\${STARDICT_UNINST_EXE}"
    ; Sets scope of the desktop and Start Menu entries for all users.
    SetShellVarContext "all"
    Goto stardict_install_files

  stardict_hkcu:
    WriteRegStr HKCU ${STARDICT_REG_KEY} "" "$INSTDIR"
    WriteRegStr HKCU ${STARDICT_REG_KEY} "Version" "${STARDICT_VERSION}"
    WriteRegStr HKCU "${STARDICT_UNINSTALL_KEY}" "DisplayName" $(STARDICT_UNINSTALL_DESC)
    WriteRegStr HKCU "${STARDICT_UNINSTALL_KEY}" "UninstallString" "$INSTDIR\${STARDICT_UNINST_EXE}"
    Goto stardict_install_files

  stardict_none:

  stardict_install_files:
    SetOutPath "$INSTDIR"
    ; StarDict files
    SetOverwrite on
    File /r /x StarDict.api .\win32-install-dir\*.*

    Call InstallWordPickAcrobatPlugin

    CreateDirectory "$SMPROGRAMS\StarDict"
    CreateShortCut "$SMPROGRAMS\StarDict\StarDict.lnk" "$INSTDIR\stardict.exe"
    CreateShortCut "$SMPROGRAMS\StarDict\StarDict-editor.lnk" "$INSTDIR\stardict-editor.exe"
    CreateShortCut "$DESKTOP\StarDict.lnk" "$INSTDIR\stardict.exe"

    ; If we don't have install rights.. we're done
    StrCmp $R0 "NONE" done
    CreateShortCut "$SMPROGRAMS\StarDict\Uninstall.lnk" "$INSTDIR\${STARDICT_UNINST_EXE}"
    SetOverwrite off

    ; write out uninstaller
    SetOverwrite on
    WriteUninstaller "$INSTDIR\${STARDICT_UNINST_EXE}"
    SetOverwrite off

  done:
SectionEnd ; end of default StarDict section


;--------------------------------
;Uninstaller Section


Section Uninstall
  Call un.CheckUserInstallRights
  Pop $R0
  StrCmp $R0 "NONE" no_rights
  StrCmp $R0 "HKCU" try_hkcu try_hklm

  try_hkcu:
    ReadRegStr $R0 HKCU ${STARDICT_REG_KEY} ""
    StrCmp $R0 $INSTDIR 0 cant_uninstall
      ; HKCU install path matches our INSTDIR.. so uninstall
      DeleteRegKey HKCU ${STARDICT_REG_KEY}
      DeleteRegKey HKCU "${STARDICT_UNINSTALL_KEY}"
      Goto cont_uninstall

  try_hklm:
    ReadRegStr $R0 HKLM ${STARDICT_REG_KEY} ""
    StrCmp $R0 $INSTDIR 0 try_hkcu
      ; HKLM install path matches our INSTDIR.. so uninstall
      DeleteRegKey HKLM ${STARDICT_REG_KEY}
      DeleteRegKey HKLM "${STARDICT_UNINSTALL_KEY}"
      DeleteRegKey HKLM "${HKLM_APP_PATHS_KEY}"
      ; Sets start menu and desktop scope to all users..
      SetShellVarContext "all"

  cont_uninstall:
    RMDir /r "$INSTDIR\dic\stardict-dict"
    RMDir "$INSTDIR\dic"
    RMDir "$INSTDIR\treedict"
    RMDir /r "$INSTDIR\locale"
    RMDir /r "$INSTDIR\sounds"
    RMDir /r "$INSTDIR\help"
    RMDir /r "$INSTDIR\pixmaps"
    RMDir /r "$INSTDIR\plugins"
    RMDir /r "$INSTDIR\data"
    RMDir /r "$INSTDIR\skins"
    RMDir /r "$INSTDIR\${VC_REDIST_DIR}"
    RMDir /r "$INSTDIR\Gtk"
    Delete "$INSTDIR\stardict.exe"
    Delete "$INSTDIR\stardict.dll"
    Delete "$INSTDIR\stardict-editor.dll"
    Delete "$INSTDIR\stardict-editor.exe"
    Delete "$INSTDIR\TextOutSpy.dll"
    Delete "$INSTDIR\TextOutHook.dll"
    Delete "$INSTDIR\${LIB_SIGC_DLL}"
    Delete "$INSTDIR\${STARDICT_UNINST_EXE}"
    ;Try to remove StarDict install dir .. if empty
    RMDir "$INSTDIR"

    Call un.DeleteWordPickAcrobatPlugin

    ; Shortcuts...
    RMDir /r "$SMPROGRAMS\StarDict"
    Delete "$DESKTOP\StarDict.lnk"

    Goto done

  cant_uninstall:
    MessageBox MB_OK $(un.STARDICT_UNINSTALL_ERROR_1) IDOK
    Quit

  no_rights:
    MessageBox MB_OK $(un.STARDICT_UNINSTALL_ERROR_2) IDOK
    Quit

  done:
SectionEnd ; end of uninstall section


;--------------------------------
;Functions

;
; Usage:
; Call CheckUserInstallRights
; First Pop:
;   "HKLM"
;   "HKCU"
;   "NONE"
Function CheckUserInstallRights
  ClearErrors
  UserInfo::GetName
  IfErrors Win9x
  Pop $0
  UserInfo::GetAccountType
  Pop $1

  StrCmp $1 "Admin" 0 +3
    StrCpy $1 "HKLM"
    Goto done
  StrCmp $1 "Power" 0 +3
    StrCpy $1 "HKLM"
    Goto done
  StrCmp $1 "User" 0 +3
    StrCpy $1 "HKCU"
    Goto done
  StrCmp $1 "Guest" 0 +3
    StrCpy $1 "NONE"
    Goto done
  ; Unknown error
  StrCpy $1 "NONE"
    Goto done

  Win9x:
    StrCpy $1 "HKLM"

  done:
    Push $1
FunctionEnd

Function un.CheckUserInstallRights
	ClearErrors
	UserInfo::GetName
	IfErrors Win9x
	Pop $0
	UserInfo::GetAccountType
	Pop $1
	StrCmp $1 "Admin" 0 +3
                StrCpy $1 "HKLM"
		Goto done
	StrCmp $1 "Power" 0 +3
                StrCpy $1 "HKLM"
		Goto done
	StrCmp $1 "User" 0 +3
		StrCpy $1 "HKCU"
		Goto done
	StrCmp $1 "Guest" 0 +3
		StrCpy $1 "NONE"
		Goto done
	; Unknown error
	StrCpy $1 "NONE"
        Goto done

	Win9x:
		StrCpy $1 "HKLM"

	done:
        Push $1
FunctionEnd

Function .onVerifyInstDir
  Push $INSTDIR
  Call VerifyDir
  Pop $0
  StrCmp $0 "0" 0 dir_good
    Abort
  dir_good:
FunctionEnd

;
; Usage:
;   Push $0 ; Path string
;   Call VerifyDir
;   Pop $0 ; 0 - Bad path  1 - Good path
;
Function VerifyDir
  Exch $0
  Push $1
  Push $2
  Loop:
    IfFileExists $0 dir_exists
    StrCpy $1 $0 ; save last
    ${GetParent} $0 $0
    StrLen $2 $0
    ; IfFileExists "C:" on xp returns true and on win2k returns false
    ; So we're done in such a case..
    IntCmp $2 2 loop_done
    ; GetParent of "C:" returns ""
    IntCmp $2 0 loop_done
    Goto Loop

  loop_done:
    StrCpy $1 "$0\GaImFooB"
    ; Check if we can create dir on this drive..
    ClearErrors
    CreateDirectory $1
    IfErrors DirBad DirGood

  dir_exists:
    ClearErrors
    FileOpen $1 "$0\pidginfoo.bar" w
    IfErrors PathBad PathGood

    DirGood:
      RMDir $1
      Goto PathGood1

    DirBad:
      RMDir $1
      Goto PathBad1

    PathBad:
      FileClose $1
      Delete "$0\pidginfoo.bar"
      PathBad1:
      StrCpy $0 "0"
      Push $0
      Goto done

    PathGood:
      FileClose $1
      Delete "$0\pidginfoo.bar"
      PathGood1:
      StrCpy $0 "1"
      Push $0

  done:
  Exch 3 ; The top of the stack contains the output variable
  Pop $0
  Pop $2
  Pop $1
FunctionEnd

Function .onInit
  StrCpy $ISSILENT "/S"
  Call CheckUserInstallRights
  Pop $0

  StrCmp $0 "HKLM" 0 user_dir
    StrCpy $INSTDIR "$PROGRAMFILES\StarDict"
    Goto instdir_done
  user_dir:
    Push $SMPROGRAMS
    ${GetParent} $SMPROGRAMS $R2
    ${GetParent} $R2 $R2
    StrCpy $INSTDIR "$R2\StarDict"

  instdir_done:

FunctionEnd

Function InstallWordPickAcrobatPlugin
  ReadRegStr $R9 HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\App Paths\Acrobat.exe" "Path"
  StrCmp $R9 "" next2 copyfile2
  copyfile2:
    SetOutPath "$R9\plug_ins"
    File .\win32-install-dir\StarDict.api
  next2:

  SetOutPath "$INSTDIR"
FunctionEnd

Function un.DeleteWordPickAcrobatPlugin
  ReadRegStr $R9 HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\App Paths\Acrobat.exe" "Path"
  StrCmp $R9 "" next2 deletefile2
  deletefile2:
    Delete "$R9\plug_ins\StarDict.api"
  next2:

FunctionEnd
