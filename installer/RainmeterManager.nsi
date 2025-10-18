!include "MUI2.nsh"
!include "LogicLib.nsh"
!include "FileFunc.nsh"
!include "x64.nsh"

!define APP_NAME "RainmeterManager"
!ifndef APP_VERSION
!define APP_VERSION "0.0.0-dev"
!endif
!define APP_PUBLISHER "Michael Shortland (BarefootMikeOfHorme)"
!define APP_GUID "{F3A9E0A0-5A1B-4B77-9E8A-123456789ABC}"

!ifndef ARCH
!define ARCH "x64" ; pass /DARCH=x86 or /DARCH=x64 at build time
!endif

!addincludedir "includes"
!include "Prereqs.nsh"

Name "${APP_NAME} ${APP_VERSION}"
OutFile "RainmeterManager-Setup-${APP_VERSION}-${ARCH}.exe"

; Install under Program Files (x64) by default; fallback to Program Files on 32-bit
InstallDir "$PROGRAMFILES64\${APP_NAME}"
RequestExecutionLevel admin
SetCompressor /SOLID lzma
BrandingText "${APP_NAME} Installer"

Var StartMenuFolder

!define MUI_ABORTWARNING
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "..\LICENSE.txt"
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_LANGUAGE "English"

Function .onInit
  SetShellVarContext all
  ${IfNot} ${RunningX64}
    StrCpy $InstDir "$PROGRAMFILES\${APP_NAME}"
  ${EndIf}
FunctionEnd

Section "Core Files" SEC_CORE
  SetOutPath "$INSTDIR"
  ; App binaries staged by CI to artifacts/app/{ARCH}/Release
  File /r "..\artifacts\app\${ARCH}\Release\*.*"

  ; RenderProcess publish output to a subfolder
  SetOutPath "$INSTDIR\RenderProcess"
  File /r "..\artifacts\render\${ARCH}\publish\*.*"

  ; Write uninstaller
  SetOutPath "$INSTDIR"
  WriteUninstaller "$INSTDIR\Uninstall.exe"

  ; Add ARP (Add/Remove Programs) entry
  ${If} ${RunningX64}
    SetRegView 64
  ${EndIf}
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_GUID}" "DisplayName" "${APP_NAME}"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_GUID}" "Publisher" "${APP_PUBLISHER}"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_GUID}" "DisplayVersion" "${APP_VERSION}"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_GUID}" "InstallLocation" "$INSTDIR"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_GUID}" "UninstallString" "$INSTDIR\Uninstall.exe"
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_GUID}" "NoModify" 1
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_GUID}" "NoRepair" 1
SectionEnd

Section "Prerequisites" SEC_PREREQS
  Call CheckAndInstallVCpp
  Call CheckAndInstallWebView2
SectionEnd

Section "Shortcuts" SEC_SHORTCUTS
  CreateDirectory "$SMPROGRAMS\${APP_NAME}"
  CreateShortCut "$SMPROGRAMS\${APP_NAME}\${APP_NAME}.lnk" "$INSTDIR\RainmeterManager.exe"
  CreateShortCut "$DESKTOP\${APP_NAME}.lnk" "$INSTDIR\RainmeterManager.exe"
SectionEnd

Section "Uninstall"
  ${If} ${RunningX64}
    SetRegView 64
  ${EndIf}
  ; Remove shortcuts
  Delete "$SMPROGRAMS\${APP_NAME}\${APP_NAME}.lnk"
  RMDir "$SMPROGRAMS\${APP_NAME}"
  Delete "$DESKTOP\${APP_NAME}.lnk"

  ; Remove files
  RMDir /r "$INSTDIR\RenderProcess"
  Delete "$INSTDIR\Uninstall.exe"
  RMDir /r "$INSTDIR"

  ; Remove ARP entry
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_GUID}"
SectionEnd
