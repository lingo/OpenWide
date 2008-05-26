#ifndef	OW_UTIL_H
/**
 * @author  Luke Hudson
 * @licence GPL2
 */

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


/* Copied on : Tue Jul 12 19:04:19 2005 */
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
