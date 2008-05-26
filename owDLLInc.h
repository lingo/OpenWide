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


#ifndef LINGO_OPENWIDE_INC_H
#define LINGO_OPENWIDE_INC_H


#include <windows.h>

#ifdef __LCC__
#define DLLPROC	LibMain
#else
#define DLLPROC	DllMain
#endif

typedef struct OWSubClassData
{
	WNDPROC		wpOrig;
	LPARAM		lpData;
	BOOL		bSet;
} OWSubClassData, *POWSubClassData;


typedef struct FindChildData {
	const char *szClass;
	UINT uID;
	HWND hwFound;
} FindChildData, *PFindChildData;


typedef struct FavLink
{
	//struct FavLink * next;
	int			idCmd;
	int			iLen;
	char		szFav[MAX_PATH+4];
} FavLink, *PFavLink;

typedef int (*FPFaveIter)(PFavLink pFav, void * pData);

extern HINSTANCE 	ghInst;
extern HHOOK 		ghMsgHook, ghSysMsgHook;
extern HANDLE		ghMutex;
extern OWSharedData	gOwShared;



void closeSharedMem(void);
BOOL DLLEXPORT WINAPI DLLPROC(HINSTANCE hDLLInst, DWORD fdwReason, LPVOID lpvReserved);
int getSharedData(void);
int initSharedMem(void);
int openSharedMem(void);
int  openWide(HWND hwnd);
void releaseSharedMem(void);
void CALLBACK timerProc(HWND hwnd, UINT uMsg, UINT uID, DWORD dwTime);



HWND findChildWindow(HWND hwParent, UINT uID, const char *szClass);
int focusDlgItem(HWND hwnd, int iFocus);
WORD focusToCtlID(int iFocus);
BOOL CALLBACK fpEnumChildren(HWND hwnd, LPARAM lParam);
HWND  getChildWinFromPt(HWND hwnd);
void releaseMutex(void);
int subclass(HWND hwnd, WNDPROC wpNew, LPARAM lpData);
int unsubclass(HWND hwnd);
WORD viewToCmdID(int iView);
BOOL waitForMutex(void);

#endif
