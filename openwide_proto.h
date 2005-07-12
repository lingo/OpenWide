#ifndef LINGO_OPENWIDE_H
#define LINGO_OPENWIDE_H

#include <windows.h>

/* Copied on : Tue Jul 12 19:04:35 2005 */
int  addTrayIcon(HWND hwnd);
HWND createListenerWindow(void);
int createWin(void);
void doQuit(void);
void doTrayMenu(HWND hwnd);
int initListener(HWND hwnd);
int initSharedMem(HWND hwnd);
BOOL isStartupApp(HWND hwnd);
int ow_init(void);
void ow_shutdown(void);
void releaseSharedMem(void);
void remTrayIcon(HWND hwnd);
int  setStartupApp(HWND hwnd, BOOL bSet);
void showSettingsDlg(HWND hwnd);
int WINAPI WinMain(HINSTANCE hi, HINSTANCE hiPrv, LPSTR fakeCmdLine, int iShow);
LRESULT WINAPI CALLBACK wpListener(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
BOOL WINAPI CALLBACK wpPlacement(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);

/* Copied on : Tue Jul 12 19:05:40 2005 */
void fillFocusCB(HWND hwnd, UINT uID);
void fillViewCB(HWND hwnd, UINT uID);
int initPrefs(HWND hwnd);
int CALLBACK WINAPI initPropSheets(HWND hwnd, UINT msg, LPARAM lp);
int savePrefsToRegistry(void);
void selectCBView(HWND hwnd, UINT uID, int iView);
HWND showDlg(HWND hwParent);

#endif
