!include "FileFunc.nsh"
!include "LogicLib.nsh"
!include "x64.nsh"

!macro DownloadFile URL OUT
  nsisdl::download /TIMEOUT=30000 "${URL}" "${OUT}"
  Pop $0
!macroend

Function CheckAndInstallVCpp
  ; Detect VC++ 2015-2022 runtime
  ${If} "${ARCH}" == "x64"
    StrCpy $1 "x64"
    ReadRegDWORD $0 HKLM "SOFTWARE\Microsoft\VisualStudio\14.0\VC\Runtimes\x64" "Installed"
  ${Else}
    StrCpy $1 "x86"
    ReadRegDWORD $0 HKLM "SOFTWARE\Microsoft\VisualStudio\14.0\VC\Runtimes\x86" "Installed"
  ${EndIf}

  ${If} $0 != 1
    MessageBox MB_ICONINFORMATION "Installing Microsoft VC++ Redistributable ($1)..."
    ${If} ${FileExists} "$EXEDIR\vc_redist_$1.exe"
      ExecWait '"$EXEDIR\vc_redist_$1.exe" /quiet /norestart'
    ${Else}
      ${If} "$1" == "x64"
        !define __VCR_URL "https://aka.ms/vs/17/release/vc_redist.x64.exe"
        !define __VCR_OUT "$TEMP\vc_redist.x64.exe"
      ${Else}
        !define __VCR_URL "https://aka.ms/vs/17/release/vc_redist.x86.exe"
        !define __VCR_OUT "$TEMP\vc_redist.x86.exe"
      ${EndIf}
      ${DownloadFile} ${__VCR_URL} ${__VCR_OUT}
      ExecWait '"${__VCR_OUT}" /quiet /norestart'
    ${EndIf}
  ${EndIf}
FunctionEnd

Function CheckAndInstallWebView2
  ReadRegStr $0 HKLM "SOFTWARE\Microsoft\EdgeUpdate\Clients\{F3017226-FE2A-4295-8EE6-839F9D3DA9C2}" "pv"
  ${If} $0 == ""
    MessageBox MB_ICONINFORMATION "Installing Microsoft Edge WebView2 Runtime..."
    ${If} ${FileExists} "$EXEDIR\MicrosoftEdgeWebview2Setup.exe"
      ExecWait '"$EXEDIR\MicrosoftEdgeWebview2Setup.exe" /silent /install'
    ${Else}
      !define __WV_URL "https://go.microsoft.com/fwlink/p/?LinkId=2124703"
      !define __WV_OUT "$TEMP\WebView2Setup.exe"
      ${DownloadFile} ${__WV_URL} ${__WV_OUT}
      ExecWait '"${__WV_OUT}" /silent /install'
    ${EndIf}
  ${EndIf}
FunctionEnd
