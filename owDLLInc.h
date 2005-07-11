#ifndef LINGO_OPENWIDE_INC_H
#define LINGO_OPENWIDE_INC_H


#include <windows.h>

#ifdef __LCC__
#define DLLPROC	LibMain
#else
#define DLLPROC	DllMain
#endif

#define	OW_ABOUT_CMDID		0x1010
#define	OW_EXPLORE_CMDID	0x1020
#define	OW_LISTVIEW_STYLE	(LVS_EX_FULLROWSELECT)


typedef struct OWSubClassData
{
	WNDPROC		wpOrig;
	LPARAM		lpData;
	BOOL		bSet;
} OWSubClassData, *POWSubClassData;


extern HINSTANCE 	ghInst;
extern HHOOK 		ghMsgHook, ghSysMsgHook;
extern HANDLE		ghMutex;
extern OWSharedData	gOwShared;


/* Copied on : Tue Jul 12 04:13:13 2005 */
void closeSharedMem(void);
BOOL DLLEXPORT WINAPI DLLPROC(HINSTANCE hDLLInst, DWORD fdwReason, LPVOID lpvReserved);
int getSharedData(void);
int initSharedMem(void);
int openSharedMem(void);
int  openWide(HWND hwnd);
void releaseSharedMem(void);
void CALLBACK timerProc(HWND hwnd, UINT uMsg, UINT uID, DWORD dwTime);


/* Copied on : Tue Jul 12 05:30:55 2005 */
void dbg(char *szError, ...);
int focusDlgItem(HWND hwnd, int iFocus);
WORD focusToCtlID(int iFocus);
BOOL CALLBACK fpEnumChildren(HWND hwnd, LPARAM lParam);
HWND  getChildWinFromPt(HWND hwnd);
DWORD DLLEXPORT GetDllVersion(LPCTSTR lpszDllName);
BOOL DLLEXPORT isWinXP(void);
void releaseMutex(void);
int subclass(HWND hwnd, WNDPROC wpNew, LPARAM lpData);
int unsubclass(HWND hwnd);
WORD viewToCmdID(int iView);
BOOL waitForMutex(void);

#endif
