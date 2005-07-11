; Script generated by the HM NIS Edit Script Wizard.

; HM NIS Edit Wizard helper defines
!define PRODUCT_NAME "OpenWide"
!define PRODUCT_VERSION "1.1b3"
!define PRODUCT_PUBLISHER "Lingo"
!define PRODUCT_WEB_SITE "http://lingo.atspace.com"
!define PRODUCT_DIR_REGKEY "Software\Microsoft\Windows\CurrentVersion\App Paths\openwide.exe"
!define PRODUCT_UNINST_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}"
!define PRODUCT_UNINST_ROOT_KEY "HKLM"
!define OW_SETTINGS_REGKEY "Software\HiveMind\OpenWide"
SetCompressor lzma

Name "${PRODUCT_NAME} ${PRODUCT_VERSION}"
OutFile "OpenWide1_1b3Setup.exe"
LoadLanguageFile "${NSISDIR}\Contrib\Language files\English.nlf"
InstallDir "$PROGRAMFILES\OpenWide"
Icon "${NSISDIR}\Contrib\Graphics\Icons\arrow2-install.ico"
UninstallIcon "${NSISDIR}\Contrib\Graphics\Icons\arrow2-uninstall.ico"
InstallDirRegKey HKLM "${PRODUCT_DIR_REGKEY}" ""
DirText "Setup will install $(^Name) in the following folder.$\r$\n$\r$\nTo install in a different folder, click Browse and select another folder."
LicenseText "If you accept all the terms of the agreement, choose I Agree to continue. You must accept the agreement to install $(^Name)."
LicenseData "readme.txt"
ShowInstDetails show
ShowUnInstDetails show

Section "MainSection" SEC01
  SetOutPath "$INSTDIR"
  SetOverwrite ifnewer
  File "openwide.exe"
  CreateDirectory "$SMPROGRAMS\OpenWide"
  CreateShortCut "$SMPROGRAMS\OpenWide\OpenWide.lnk" "$INSTDIR\openwide.exe"
  CreateShortCut "$DESKTOP\OpenWide.lnk" "$INSTDIR\openwide.exe"
  File "openwidedll.dll"
  File "readme.txt"
  CreateShortCut "$SMPROGRAMS\OpenWide\ReadMe.lnk" "$INSTDIR\readme.txt"
SectionEnd

Section -AdditionalIcons
  WriteIniStr "$INSTDIR\${PRODUCT_NAME}.url" "InternetShortcut" "URL" "${PRODUCT_WEB_SITE}"
  CreateShortCut "$SMPROGRAMS\OpenWide\Website.lnk" "$INSTDIR\${PRODUCT_NAME}.url"
  CreateShortCut "$SMPROGRAMS\OpenWide\Uninstall.lnk" "$INSTDIR\uninst.exe" "" "${NSISDIR}\Contrib\Graphics\Icons\arrow2-uninstall.ico"
SectionEnd

Section -Post
  WriteUninstaller "$INSTDIR\uninst.exe"
  WriteRegStr HKLM "${PRODUCT_DIR_REGKEY}" "" "$INSTDIR\openwide.exe"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayName" "$(^Name)"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "UninstallString" "$INSTDIR\uninst.exe"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayIcon" "$INSTDIR\openwide.exe"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayVersion" "${PRODUCT_VERSION}"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "URLInfoAbout" "${PRODUCT_WEB_SITE}"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "Publisher" "${PRODUCT_PUBLISHER}"
SectionEnd


Function un.onUninstSuccess
  HideWindow
  MessageBox MB_ICONINFORMATION|MB_OK "OpenWide was successfully removed from your computer."
FunctionEnd

Function un.onInit
  MessageBox MB_ICONQUESTION|MB_YESNO|MB_DEFBUTTON2 "Are you sure you want to completely remove OpenWide and all of its components?" IDYES +2
  Abort
FunctionEnd

Section Uninstall
  Delete "$INSTDIR\${PRODUCT_NAME}.url"
  Delete "$INSTDIR\uninst.exe"
  Delete "$INSTDIR\readme.txt"
  Delete "$INSTDIR\openwidedll.dll"
  Delete "$INSTDIR\openwide.exe"

  Delete "$SMPROGRAMS\OpenWide\Uninstall.lnk"
  Delete "$SMPROGRAMS\OpenWide\Website.lnk"
  Delete "$SMPROGRAMS\OpenWide\ReadMe.lnk"
  Delete "$DESKTOP\OpenWide.lnk"
  Delete "$SMPROGRAMS\OpenWide\OpenWide.lnk"
  
  RMDir "$SMPROGRAMS\OpenWide"
  RMDir "$INSTDIR"
  RMDir ""

  DeleteRegKey ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}"
  DeleteRegKey HKLM "${PRODUCT_DIR_REGKEY}"
  DeleteRegKey HKCU "${OW_SETTINGS_REGKEY}"
  SetAutoClose false
SectionEnd