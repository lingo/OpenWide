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


#ifndef	OW_UTIL_H
#define	OW_UTIL_H

#include	<windows.h>

enum	PromptFileNameMode {OPEN, SAVE};


#define		RCWIDTH(r)	((r).right - (r).left + 1)
#define		RCHEIGHT(r)	((r).bottom - (r).top + 1)

#define		WM_TRAYICON		(WM_APP+1)

extern HWND				ghwMain, ghwPropSheet;
extern HINSTANCE		ghInstance;
extern POWSharedData 	gPowData;
extern HANDLE			ghSharedMem;
extern HANDLE			ghMutex;
extern HICON			ghIconLG, ghIconSm;



int Add_TrayIcon(HICON hIcon, char *szTip, HWND hwnd, UINT uMsg, DWORD dwState);
static int AddRem_TrayIcon(HICON hIcon, char *szTip, HWND hwnd, UINT uMsg, DWORD dwState, DWORD dwMode);
int  cbAddString(HWND hwCB, const char *szStr, LPARAM lpData);
HRESULT CreateLink(LPCSTR lpszPathObj, LPCSTR lpszPathLink, LPCSTR lpszDesc);
BOOL delFile(HWND hwnd, const char *szFile);
int  dlgUnits2Pix(HWND hwnd, int units, BOOL bHorz);
void EndTrayOperation(void);
void Error(char *szError, ...);
BOOL fileExists (const char *path);
int getDlgItemRect(HWND hwnd, UINT uID, LPRECT pr);
TCHAR *lbGetItemText(HWND hwLB, int iItem);
int  pix2DlgUnits(HWND hwnd, int pix, BOOL bHorz);
char *Prompt_File_Name(int iFlags, HWND hwOwner, const char *pszFilter, const char *pszTitle);
void releaseMutex(void);
int Rem_TrayIcon(HWND hwnd, UINT uMsg, DWORD dwState);
BOOL waitForMutex(void);

#endif
