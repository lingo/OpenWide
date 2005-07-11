#ifndef LINGO_OPENWIDE_H
#define LINGO_OPENWIDE_H

#include <windows.h>


/* Copied on : Tue Jul 12 02:30:58 2005 */
int  addTrayIcon(HWND hwnd);
HWND createListenerWindow(void);
int createWin(void);
int initSharedMem(HWND hwnd);
int initWin(HWND hwnd);
BOOL isStartupApp(HWND hwnd);
static void onCommand(HWND hwnd, WPARAM wp, LPARAM lp);
int ow_init(void);
void ow_shutdown(void);
void releaseSharedMem(void);
void remTrayIcon(HWND hwnd);
int  setStartupApp(HWND hwnd, BOOL bSet);
BOOL WINAPI CALLBACK wpMain(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
BOOL WINAPI CALLBACK wpPlacement(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
LRESULT WINAPI CALLBACK wpListener(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
#endif
