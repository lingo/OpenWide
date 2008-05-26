#ifndef LINGO_OPENWIDE_INC_H
/**
 * @author  Luke Hudson
 * @licence GPL2
 */

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
