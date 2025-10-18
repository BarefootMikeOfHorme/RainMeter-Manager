; rainmetermanager_installer.nsi - NSIS installer stub
; TODO: configure product details, files, registry, shortcuts

!define APP_NAME "RainmeterManager"
!define APP_VERSION "0.0.1"
!define COMPANY_NAME "RainmeterManager"

OutFile "RainmeterManager-Setup-${APP_VERSION}.exe"
InstallDir "$PROGRAMFILES64\${APP_NAME}"
ShowInstDetails show

Section "Install"
  SetOutPath "$INSTDIR"
  ; TODO: Include built binaries and assets
  ; File /r "..\build\Release\*.*"

  ; TODO: Create shortcuts
  ; CreateShortCut "$DESKTOP\${APP_NAME}.lnk" "$INSTDIR\RainmeterManager.exe"
SectionEnd

Section "Uninstall"
  ; TODO: Remove files and shortcuts
  ; Delete "$DESKTOP\${APP_NAME}.lnk"
  ; RMDir /r "$INSTDIR"
SectionEnd
