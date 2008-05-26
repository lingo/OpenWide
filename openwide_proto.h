/*
 * Openwide -- control Windows common dialog
 * 
 * Copyright (c) 2000 Luke Hudson
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 * 
 */


#ifndef LINGO_OPENWIDE_H
#define LINGO_OPENWIDE_H

#include <windows.h>


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


void fillFocusCB(HWND hwnd, UINT uID);
void fillViewCB(HWND hwnd, UINT uID);
int initPrefs(HWND hwnd);
int CALLBACK WINAPI initPropSheets(HWND hwnd, UINT msg, LPARAM lp);
int savePrefsToRegistry(void);
void selectCBView(HWND hwnd, UINT uID, int iView);
HWND showDlg(HWND hwParent);

#endif
