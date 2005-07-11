#ifndef	OW_UTIL_H
#define	OW_UTIL_H

#include	<windows.h>

#define	OW_REGKEY_NAME	("Software\\Lingo\\OpenWide")

#define		RCWIDTH(r)	((r).right - (r).left + 1)
#define		RCHEIGHT(r)	((r).bottom - (r).top + 1)

#define		WM_TRAYICON		(WM_APP+1)

extern HWND		ghwMain, ghwPropSheet;
extern HINSTANCE	ghInstance;
extern POWSharedData 	gPowData;
extern HANDLE			ghSharedMem;
extern HANDLE			ghMutex;



int		setStartupApp(HWND hwnd, BOOL	bSet);
HWND 	showDlg(HWND hwParent);

/* Copied on : Tue Jul 12 02:30:33 2005 */
int Add_TrayIcon(HICON hIcon, char *szTip, HWND hwnd, UINT uMsg, DWORD dwState);
static int AddRem_TrayIcon(HICON hIcon, char *szTip, HWND hwnd, UINT uMsg, DWORD dwState, DWORD dwMode);
int  cbAddString(HWND hwCB, const char *szStr, LPARAM lpData);
HRESULT CreateLink(LPCSTR lpszPathObj, LPCSTR lpszPathLink, LPCSTR lpszDesc);
void dbg(char *szError, ...);
BOOL delFile(HWND hwnd, const char *szFile);
int  dlgUnits2Pix(HWND hwnd, int units, BOOL bHorz);
void EndTrayOperation(void);
void Error(char *szError, ...);
BOOL fileExists (const char *path);
int getDlgItemRect(HWND hwnd, UINT uID, LPRECT pr);
const char * geterrmsg(void);
int  pix2DlgUnits(HWND hwnd, int pix, BOOL bHorz);
char *Prompt_File_Name(int iFlags, HWND hwOwner, const char *pszFilter, const char *pszTitle);
void regCloseKey(HKEY hk);
HKEY regCreateKey(HKEY hkParent, const char *szSubKey);
BYTE *regReadBinaryData(HKEY hkRoot, const char *szValueName);
DWORD regReadDWORD(HKEY hkRoot, const char *szValueName, int *pSuccess);
int regWriteBinaryData(HKEY hkRoot, const char *szValue, BYTE *buf, int bufSize );
int regWriteDWORD(HKEY hkRoot, const char *szValue, DWORD dwData);
int Rem_TrayIcon(HWND hwnd, UINT uMsg, DWORD dwState);
BOOL waitForMutex(void);
void Warn(char *szError, ...);

#endif
