#ifndef	C__Code_C_openwide_openwide_proto_h
#define	C__Code_C_openwide_openwide_proto_h
#pragma once



/* Prototype file C:\Code\C\openwide\openwide_proto.h *
 * generated by Pro2 from files:
 * C:\Code\C\openwide\openwide.c
 * on Wednesday, 11 May 2005 */


// functions from file C:\Code\C\openwide\openwide.c //
int  addTrayIcon(HWND hwnd);
int  cbAddString(HWND hwCB, const char *szStr, LPARAM lpData);
int createWin(void);
int doApply(HWND hwnd);
void doTrayMenu(HWND hwnd);
void fillFocusCB(HWND hwnd, UINT uID);
void fillViewCB(HWND hwnd, UINT uID);
int initApp(void);
int initFavourites(POWSharedData powData);
int initSharedMem(HWND hwnd);
int initWin(HWND hwnd);
BOOL isChanged(void);
BOOL isStartupApp(HWND hwnd);
int lockSharedMem(void);
void releaseSharedMem(void);
void remTrayIcon(HWND hwnd);
void selectCBView(HWND hwnd, UINT uID, int iView);
void setChanged(HWND hwnd, BOOL bChanged);
int  setStartupApp(HWND hwnd, BOOL bSet);
void shutdownApp(void);
int unlockSharedMem(void);
int WINAPI WinMain(HINSTANCE hi, HINSTANCE hiPrv, LPSTR fakeCmdLine, int iShow);
BOOL WINAPI CALLBACK wpMain(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
BOOL WINAPI CALLBACK wpPlacement(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);

#endif	// C__Code_C_openwide_openwide_proto_h