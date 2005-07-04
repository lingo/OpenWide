/* --- The following code comes from c:\lcc\lib\wizard\dll.tpl. */
#include <windows.h>
#include <commctrl.h>
#include	<shlwapi.h>
#include	<shellapi.h>
#include	<stdarg.h>
#include	<stdio.h>
#include	<dlgs.h>
#include	"openwidedll.h"
#include	"openwide.h"
//#include	"dbgwm.h"

#ifdef __LCC__
#define DLLPROC	LibMain
#else
#define DLLPROC	DllMain
#endif

static HINSTANCE ghInst = NULL;

static HHOOK ghMsgHook = NULL, ghSysMsgHook = NULL;
static BOOL gbHooked = FALSE;

static HANDLE	ghMutex = NULL;
static OWSharedData	gOwShared;

#define	OW_ABOUT_CMDID		0x1010
#define	OW_EXPLORE_CMDID	0x1020

#define	OW_LISTVIEW_STYLE	(LVS_EX_FULLROWSELECT)

typedef struct OWSubClassData
{
	WNDPROC		wpOrig;
	LPARAM		lpData;
	BOOL		bSet;
} OWSubClassData, *POWSubClassData;

//static BOOL gbDbg = FALSE;

////////////////////////////////////////////////////////////////////////////////
//  Prototypes
/* Copied by pro2 : (c)2004 Luke Hudson */
static LRESULT CALLBACK CBTProc(int nCode, WPARAM wParam, LPARAM lParam);
void dbg(char *szError, ...);
WORD focusToCtlID(int iFocus);
DWORD DLLEXPORT GetDllVersion(LPCTSTR lpszDllName);
int getSharedData(void);
int initSharedMem(void);
BOOL DLLEXPORT isWinXP(void);
BOOL DLLEXPORT WINAPI DLLPROC(HINSTANCE hDLLInst, DWORD fdwReason, LPVOID lpvReserved);
void releaseSharedMem(void);
int DLLEXPORT rmvHook(void);
int DLLEXPORT setHook(HWND hwLB);
int subclass(HWND hwnd, WNDPROC wpNew, LPARAM lpData);
static LRESULT CALLBACK SysMsgProc(int nCode, WPARAM wParam, LPARAM lParam);
void CALLBACK timerProc(HWND hwnd, UINT uMsg, UINT uID, DWORD dwTime);
int unsubclass(HWND hwnd);
WORD viewToCmdID(int iView);
LRESULT CALLBACK WINAPI wpSubMain(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
LRESULT CALLBACK WINAPI wpSubShellCtl(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
int focusDlgItem(HWND hwnd, int iFocus);
LRESULT CALLBACK WINAPI wpOverlay(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
/*------------------------------------------------------------------------
 Procedure:     LibMain ID:1
 Purpose:       Dll entry point.Called when a dll is loaded or
                unloaded by a process, and when new threads are
                created or destroyed.
 Input:         hDllInst: Instance handle of the dll
                fdwReason: event: attach/detach
                lpvReserved: not used
 Output:        The return value is used only when the fdwReason is
                DLL_PROCESS_ATTACH. True means that the dll has
                sucesfully loaded, False means that the dll is unable
                to initialize and should be unloaded immediately.
 Errors:
------------------------------------------------------------------------*/
BOOL DLLEXPORT WINAPI DLLPROC(HINSTANCE hDLLInst, DWORD fdwReason, LPVOID lpvReserved)
{
    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
            // The DLL is being loaded for the first time by a given process.
            // Perform per-process initialization here.  If the initialization
            // is successful, return TRUE; if unsuccessful, return FALSE.
			ghInst = hDLLInst;
            break;
        case DLL_PROCESS_DETACH:
            // The DLL is being unloaded by a given process.  Do any
            // per-process clean up here, such as undoing what was done in
            // DLL_PROCESS_ATTACH.  The return value is ignored.
			ghInst = NULL;
	        break;
        case DLL_THREAD_ATTACH:
            // A thread is being created in a process that has already loaded
            // this DLL.  Perform any per-thread initialization here.  The
            // return value is ignored.

            break;
        case DLL_THREAD_DETACH:
            // A thread is exiting cleanly in a process that has already
            // loaded this DLL.  Perform any per-thread clean up here.  The
            // return value is ignored.

            break;
    }
    return TRUE;
}

void dbg(char *szError, ...)
{
	char		szBuff[256];
	va_list		vl;
	va_start(vl, szError);
    vsnprintf(szBuff, 256, szError, vl);	// print error message to string
	OutputDebugString(szBuff);
	va_end(vl);
}

int	subclass(HWND hwnd, WNDPROC wpNew, LPARAM lpData)
{
	if( GetProp(hwnd, OW_PROP_NAME) != NULL )
		return 0;
	POWSubClassData pow = (POWSubClassData)malloc(sizeof(OWSubClassData));
	if(!pow)
		return 0;
	ZeroMemory(pow, sizeof(OWSubClassData));
	if( !SetProp(hwnd, OW_PROP_NAME, pow) )
	{
		free(pow);
		return 0;
	}
	pow->lpData = lpData;
	pow->wpOrig = (WNDPROC)SetWindowLong(hwnd, GWL_WNDPROC, (LONG)wpNew);
	return 1;
}

int	unsubclass(HWND hwnd)
{
	POWSubClassData pow = (POWSubClassData)GetProp(hwnd, OW_PROP_NAME);
	if(pow)
	{
		SetWindowLong(hwnd, GWL_WNDPROC, (LONG)pow->wpOrig);
		RemoveProp(hwnd, OW_PROP_NAME);
		free(pow);
		return 1;
	}
	return 0;
}

HWND	createOverlayWindow(HWND hwParent)
{
	RECT rc;
	GetClientRect(hwParent, &rc);

	WNDCLASSEX wc = {0};
	wc.cbSize = sizeof(wc);
	wc.hbrBackground = NULL;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hInstance = ghInst;
	wc.lpfnWndProc = wpOverlay;
	wc.lpszClassName = OW_OVERLAY_CLASS;
	wc.style = CS_DBLCLKS;
	RegisterClassEx(&wc);

	return CreateWindowEx(WS_EX_TRANSPARENT | WS_EX_NOACTIVATE, OW_OVERLAY_CLASS, NULL,
		WS_VISIBLE | WS_CHILD,
		0,0, rc.right, rc.bottom,
		hwParent, (HMENU)CID_OVERLAY, ghInst, NULL);
}

HWND 	getChildWinFromPt(HWND hwnd)
{
	POINT pt;
	GetCursorPos(&pt);
	ScreenToClient(GetParent(hwnd), &pt);

	EnableWindow(hwnd, FALSE);
	HWND hw = ChildWindowFromPointEx(GetParent(hwnd), pt, CWP_SKIPDISABLED | CWP_SKIPINVISIBLE);
	EnableWindow(hwnd, TRUE);
	HWND hwSV = GetDlgItem(GetParent(hwnd), CID_DIRLISTPARENT);
	if( hw == hwSV )
	{
		ClientToScreen(GetParent(hwnd), &pt);
		ScreenToClient(hw, &pt);
		hw = ChildWindowFromPointEx(hw, pt, CWP_SKIPDISABLED | CWP_SKIPINVISIBLE);
	}
	HWND hwEd = GetDlgItem(GetParent(hwnd), CID_FNAME);
	if( hw == hwEd )
	{
		ClientToScreen(GetParent(hwnd), &pt);
		ScreenToClient(hw, &pt);
		hw = ChildWindowFromPointEx(hw, pt, CWP_SKIPDISABLED | CWP_SKIPINVISIBLE);
	}
	return hw;
}

LRESULT CALLBACK WINAPI wpOverlay(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
//	POWSubClassData pow = (POWSubClassData)GetProp(hwnd, OW_PROP_NAME);
///	if( !pow )
//		return DefWindowProc(hwnd, msg, wp, lp);

	switch(msg)
	{
		case WM_CREATE:
			return 0;
		case WM_NCCREATE:
			return TRUE;
		case WM_MOUSEACTIVATE:
			return MA_NOACTIVATE;
		case WM_DROPFILES:
			{
				HANDLE hDrop = (HANDLE)wp;
				int nFiles = DragQueryFile(hDrop, (UINT)-1, NULL, 0);
				if( nFiles == 1 )
				{
					static char buf[MAX_PATH+1];
					if( DragQueryFile(hDrop, 0, buf, MAX_PATH) )
					{
						if( PathIsDirectory(buf) )
						{
							HWND hw = GetParent(hwnd);
							SetDlgItemText(hw, CID_FNAME, buf);
							SendDlgItemMessage(hw, CID_FNAME, EM_SETSEL, -1, -1);
							SendDlgItemMessage(hw, CID_FNAME, EM_REPLACESEL, FALSE, (LPARAM)"\\");
							SendMessage(hw, WM_COMMAND, MAKEWPARAM(IDOK, BN_CLICKED), (LPARAM)GetDlgItem(hwnd, IDOK));
							SetDlgItemText(hw, CID_FNAME, "");
							if( getSharedData() )
							{
								focusDlgItem(hw, gOwShared.iFocus);
							}
						}
					}
				}
			}
			return 0;
		case WM_ERASEBKGND:
			return 1;
		case WM_PAINT:
			ValidateRect(hwnd, NULL);
			return 0;
		default:
			return DefWindowProc(hwnd, msg, wp, lp);
	}
	return 0;
}


int		openWide(HWND hwnd)
{
	// set placement
	int w = gOwShared.szDim.cx;
	int h = gOwShared.szDim.cy;
	int x = gOwShared.ptOrg.x;
	int y = gOwShared.ptOrg.y;
	SetWindowPos(hwnd, NULL, x, y, w, h, SWP_NOZORDER);

	// set view mode
	HWND hwDirCtl = GetDlgItem(hwnd, CID_DIRLISTPARENT);
	WORD	vCmdID = viewToCmdID(gOwShared.iView);
//	dbg("Sending message to set view, cmd id %d", vCmdID);
	SendMessage(hwDirCtl, WM_COMMAND, MAKEWPARAM(vCmdID, 0), 0);

	focusDlgItem(hwnd, gOwShared.iFocus);

	// debug hook, to find menu cmd IDs
#ifdef	DEBUG
	dbg("Hooking SYSMSG...");
	ghSysMsgHook = SetWindowsHookEx(
						WH_SYSMSGFILTER,
				        SysMsgProc,
				        ghInst,
				        0 //GetCurrentThreadId()            // Only install for THIS thread!!!
				);
	dbg("Hooking returned %p", ghSysMsgHook);
#endif
	DragAcceptFiles(hwnd, TRUE);
	//SetWindowLong(hwnd, GWL_STYLE, GetWindowLong(hwnd, GWL_STYLE) | WS_CAPTION | WS_SYSMENU);

	HMENU hm = GetSystemMenu(hwnd, FALSE);
	AppendMenu(hm, MF_SEPARATOR | MF_DISABLED, 0, NULL);
	AppendMenu(hm, MF_STRING, OW_EXPLORE_CMDID, "&Locate current folder with Explorer...");
	AppendMenu(hm, MF_STRING, OW_ABOUT_CMDID, "&About OpenWide...");

/*
	HWND hwShellCtl = GetDlgItem(hwnd, CID_DIRLISTPARENT);
	hwShellCtl = GetDlgItem(hwShellCtl, 1);
	ListView_SetExtendedListViewStyleEx(hwShellCtl, OW_LISTVIEW_STYLE, OW_LISTVIEW_STYLE );

	HWND hwOv = createOverlayWindow(hwnd);
	dbg("Created overlay window: %p", hwOv);
	SetWindowPos(hwOv, HWND_TOP, 0,0,0,0, SWP_NOMOVE | SWP_NOSIZE);
	SetActiveWindow(hwnd);
	DragAcceptFiles(hwOv, TRUE);*/
	return 1;
}

LRESULT CALLBACK WINAPI wpSubMain(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
/*	if( gbDbg )
	{
		dbgWM(hwnd, msg, wp, lp);
	}*/
	POWSubClassData pow = (POWSubClassData)GetProp(hwnd, OW_PROP_NAME);
	if( !pow )
		return DefWindowProc(hwnd, msg, wp, lp);

	static char buffer[MAX_PATH+1];

	switch(msg)
	{
		case WM_INITDIALOG:
			{
				LRESULT lRes = CallWindowProc(pow->wpOrig, hwnd, msg, wp, lp);
				ShowWindow(hwnd, SW_HIDE);
				return lRes;
			}
			break;
		case WM_SHOWWINDOW:
			if( wp && !pow->bSet )
			{
				pow->bSet = TRUE;
				openWide(hwnd);
			}
			break;
		/*case WM_SIZE:
			{
				LRESULT lRes = CallWindowProc(pow->wpOrig, hwnd, msg, wp, lp);
				int w = LOWORD(lp);
				int h = HIWORD(lp);
				MoveWindow( GetDlgItem(hwnd, CID_OVERLAY), 0,0, w,h, FALSE);
				return lRes;
			}
			break;*/
//		case WM_COMMAND:
//			dbg("Got WM_COMMAND: id %d, code %d", LOWORD(wp), HIWORD(wp));
//			break;
		case WM_PARENTNOTIFY:
			if( LOWORD(wp) ==  WM_CREATE)
			{
				static char buf[33];
				GetClassName((HWND)lp, buf, 32);

				if( strcmp(buf, "SHELLDLL_DefView") == 0 )
				{
//					dbg("Shell defview ctl created");
//					subclass((HWND)lp, wpSubShellCtl, (LPARAM)hwnd);
					HWND hwLV = GetDlgItem((HWND)lp, 1);
					DragAcceptFiles(hwLV, TRUE);
					if( hwLV )
					{
						if( GetWindowLong(hwLV, GWL_STYLE) & LVS_REPORT )
						{
							//dbg("hwLV is in report mode -- setting extended style");
							ListView_SetExtendedListViewStyleEx(hwLV, OW_LISTVIEW_STYLE, OW_LISTVIEW_STYLE );
						}
					}
					SetTimer(hwnd, 251177, 1, NULL);
				}
			}
			break;
		case WM_TIMER:
			if( wp == 251177 )
			{
				KillTimer(hwnd, 251177);
				HWND hwDirCtl = GetDlgItem(hwnd, CID_DIRLISTPARENT);
				if( getSharedData() )
				{
					if( gOwShared.iView == V_THUMBS || gOwShared.iView == V_TILES )
					{
//						dbg("posting cmd message to reset view?");
						WORD	vCmdID = viewToCmdID(gOwShared.iView);
						PostMessage(hwDirCtl, WM_COMMAND, MAKEWPARAM(vCmdID, 0), 0);
					}
				}
			}
			break;
		case WM_SYSCOMMAND:
			{
				int cmdId = wp & 0xFFF0;
				if( cmdId == OW_ABOUT_CMDID )
				{
					MessageBox(hwnd, "OpenWide is written by Luke Hudson. (c)2005", "About OpenWide", MB_OK);
					return 0;
				}
				else if( cmdId == OW_EXPLORE_CMDID )
				{
					char * szParm = "/select,";
					wsprintf(buffer, szParm);
					int len = strlen(szParm);
					len = SendMessage(hwnd, CDM_GETFOLDERPATH, MAX_PATH-len, (LPARAM)(buffer+len));
					if( len )
					{
						ShellExecute(hwnd, NULL, "explorer.exe", buffer, NULL, SW_SHOWNORMAL);
					}
				}
			}
			break;
/*		case WM_NOTIFY:
			{
				NMHDR * phdr = (NMHDR *)lp;
				HWND hwSV = GetDlgItem(hwnd, CID_DIRLISTPARENT);
				hwSV = GetDlgItem(hwSV, CID_DIRLIST);
				if( phdr->hwndFrom == hwSV )
				{
					dbg("Got notify %d from listview", phdr->code);
				}
			}
			break;*/
/*		case WM_NCPAINT:
			{
				HDC hdc = GetWindowDC(hwnd);
				HBRUSH hbrOld;
				HPEN hpOld;
				hbrOld = SelectObject(hdc, GetStockObject(BLACK_BRUSH));
				hpOld = SelectObject(hdc, GetStockObject(NULL_PEN));
				RECT r;
				GetWindowRect(hwnd, &r);
				r.right-=r.left;
				r.left -= r.left;
				r.bottom-=r.top;
				r.top -= r.top;
				RoundRect(hdc, r.left, r.top, r.right, r.bottom, 16,16);
				SelectObject(hdc, hbrOld);
				SelectObject(hdc, hpOld);
				ReleaseDC(hwnd, hdc);
			}
			break;*/
		case WM_DROPFILES:
			{
				HANDLE hDrop = (HANDLE)wp;
				int nFiles = DragQueryFile(hDrop, (UINT)-1, NULL, 0);
				//dbg("%d files dropped on main window %p", nFiles, hwnd);
				if( nFiles == 1 )
				{
					if( DragQueryFile(hDrop, 0, buffer, MAX_PATH) )
					{
						if( PathIsDirectory(buffer) )
						{
							SetDlgItemText(hwnd, CID_FNAME, buffer);
							SendDlgItemMessage(hwnd, CID_FNAME, EM_SETSEL, -1, -1);
							SendDlgItemMessage(hwnd, CID_FNAME, EM_REPLACESEL, FALSE, (LPARAM)"\\");
							SendMessage(hwnd, WM_COMMAND, MAKEWPARAM(IDOK, BN_CLICKED), (LPARAM)GetDlgItem(hwnd, IDOK));
							SetDlgItemText(hwnd, CID_FNAME, "");
							if( getSharedData() )
							{
								focusDlgItem(hwnd, gOwShared.iFocus);
							}
						}
					}
				}
			}
			return 0;
		case WM_DESTROY:
			{
				WNDPROC wpOrig = pow->wpOrig;
				unsubclass(hwnd);
				return CallWindowProc(wpOrig, hwnd, msg, wp, lp);
			}
			break;
	}
	return CallWindowProc(pow->wpOrig, hwnd, msg, wp, lp);
}

LRESULT CALLBACK WINAPI wpSubShellCtl(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
	POWSubClassData pow = (POWSubClassData)GetProp(hwnd, OW_PROP_NAME);
	if( !pow )
		return DefWindowProc(hwnd, msg, wp, lp);

	switch(msg)
	{
		case WM_DROPFILES:
			{
				HANDLE hDrop = (HANDLE)wp;
				int nFiles = DragQueryFile(hDrop, (UINT)-1, NULL, 0);
				dbg("%d files dropped, fwding mesg to %p", nFiles, pow->lpData);
				return SendMessage((HWND)pow->lpData, msg, wp, lp);
			}
			break;
		case WM_DESTROY:
			{
				WNDPROC wpOrig = pow->wpOrig;
				unsubclass(hwnd);
				dbg("SHell view being destroyed");
				return CallWindowProc(wpOrig, hwnd, msg, wp, lp);
			}
			break;
	}
	return CallWindowProc(pow->wpOrig, hwnd, msg, wp, lp);
}


int initSharedMem(void)
{
	ghMutex = OpenMutex(SYNCHRONIZE, FALSE, OW_MUTEX_NAME);
	if( !ghMutex )
		return 0;
	return 1;
}


int getSharedData(void)
{
	int		rv = 0;
	if(ghMutex)
	{
		DWORD dwRes = WaitForSingleObject(ghMutex, 1000);
		if(dwRes != WAIT_OBJECT_0)
			return 0;
	}

	HANDLE hMap = OpenFileMapping(FILE_MAP_READ, TRUE, OW_SHARED_FILE_MAPPING);
	if( hMap )
	{
		POWSharedData pow = (POWSharedData)MapViewOfFile(hMap, FILE_MAP_READ, 0,0, 0);
		if( pow )
		{
			CopyMemory(&gOwShared, pow, sizeof(OWSharedData));
			rv = 1;
			UnmapViewOfFile(pow);
		}
		else
			dbg("Failed to get ptr to shared data: %d", GetLastError());
		CloseHandle(hMap);
	}

	if( ghMutex )
		ReleaseMutex(ghMutex);
	return rv;
}

BOOL CALLBACK fpEnumChildren(
    HWND hwnd,	// handle to child window
    LPARAM lParam 	// application-defined value
)
{
	static char buf[32];
	if( GetClassName(hwnd, buf, 31) )
	{
		switch(gOwShared.iFocus)
		{
			case F_DIRLIST:
				if( strcmp(buf, WC_LISTVIEWA) == 0 && GetDlgCtrlID(hwnd) == CID_DIRLIST)
					SetFocus(hwnd);
				break;
			case F_FNAME:
				if( strcmp(buf, WC_COMBOBOXEXA) == 0 && GetDlgCtrlID(hwnd) == CID_FNAME)
					SetFocus(hwnd);
				break;
			case F_FTYPE:
				if( strcmp(buf, WC_COMBOBOXA) == 0 && GetDlgCtrlID(hwnd) == CID_FTYPE)
					SetFocus(hwnd);
				break;
			case F_PLACES:
				if( IsWindowVisible(hwnd) && strcmp(buf, TOOLBARCLASSNAMEA) == 0 && GetDlgCtrlID(hwnd) == CID_PLACES)
					SetFocus(hwnd);
				break;
			case F_LOOKIN:
				if( strcmp(buf, WC_COMBOBOXA) == 0 && GetDlgCtrlID(hwnd) == CID_LOOKIN)
					SetFocus(hwnd);
				break;
		}
	}
/*	if( GetDlgCtrlID(hwnd) == 0x47c )
	{
		SetWindowText(hwnd, "C:\\code\\");
		SendMessage((HWND)lParam, WM_COMMAND, MAKEWPARAM(1, BN_CLICKED), (LPARAM)GetDlgItem((HWND)lParam, 1));
	}*/
	return 1;
}




static LRESULT CALLBACK SysMsgProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    HWND hwnd;

    if(nCode < 0)
        return CallNextHookEx(ghMsgHook, nCode, wParam, lParam);

	LPMSG	pMsg = (MSG *)lParam;
	if( pMsg->message < WM_MOUSEFIRST || pMsg->message > WM_MOUSELAST )
	{
		dbg("SysMsgProc: %d is code, pMsg: hwnd: %p, message: %d, wp: %x, lp: %x", nCode, pMsg->hwnd, pMsg->message, pMsg->wParam, pMsg->lParam);
	}
	if( GetAsyncKeyState(VK_SHIFT) & 0x8000 )
	{
		LRESULT lRet = CallNextHookEx(ghSysMsgHook, nCode, wParam, lParam);
		UnhookWindowsHookEx(ghSysMsgHook);
		ghSysMsgHook = NULL;
		return lRet;
	}
    switch(pMsg->message)
    {
		case WM_MENUSELECT:
			{
				dbg("WM_MENUSELECT: %p %d", pMsg->hwnd, LOWORD(pMsg->wParam));

				HMENU hm = (HMENU)pMsg->lParam;
				if(hm)
				{
					static char buf[80];
					MENUITEMINFO	mii ={0};
					int nItems = GetMenuItemCount(hm);
					dbg(" %d items in menu", nItems);
					for(int i=0; i < nItems; i++)
					{
						mii.cbSize = sizeof(mii);
						mii.fMask = MIIM_FTYPE | MIIM_ID | MIIM_STRING;
						mii.cch = 79;
						mii.dwTypeData = (LPTSTR)buf;
						GetMenuItemInfo(hm, i, TRUE, &mii);
						dbg(" Item %d:	%.8s, %d", i, buf, mii.wID);
					}
				}

				UnhookWindowsHookEx(ghSysMsgHook);
				ghSysMsgHook = NULL;
			}
			break;
/*		case WM_INITMENUPOPUP:
			{
				dbg("WM_INITMENUPOPUP");
				UnhookWindowsHookEx(ghSysMsgHook);
				ghSysMsgHook = NULL;
			}
			break;*/
		default:
			break;
	}

    // Call the next hook, if there is one
    return CallNextHookEx(ghSysMsgHook, nCode, wParam, lParam);
}



DWORD DLLEXPORT GetDllVersion(LPCTSTR lpszDllName)
{
    HINSTANCE hinstDll;
    DWORD dwVersion = 0;
	char	szDll[MAX_PATH+1];
    /* For security purposes, LoadLibrary should be provided with a
       fully-qualified path to the DLL. The lpszDllName variable should be
       tested to ensure that it is a fully qualified path before it is used. */
	if( !PathSearchAndQualify(lpszDllName, szDll, MAX_PATH) )
		return 0;
    hinstDll = LoadLibrary(szDll);

    if(hinstDll)
    {
        DLLGETVERSIONPROC pDllGetVersion;
        pDllGetVersion = (DLLGETVERSIONPROC)	GetProcAddress(hinstDll, "DllGetVersion");

        /* Because some DLLs might not implement this function, you
        must test for it explicitly. Depending on the particular
        DLL, the lack of a DllGetVersion function can be a useful
        indicator of the version. */

        if(pDllGetVersion)
        {
            DLLVERSIONINFO dvi;
            HRESULT hr;

            ZeroMemory(&dvi, sizeof(dvi));
            dvi.cbSize = sizeof(dvi);

            hr = (*pDllGetVersion)(&dvi);

            if(SUCCEEDED(hr))
            {
               dwVersion = PACKVERSION(dvi.dwMajorVersion, dvi.dwMinorVersion);
            }
        }

        FreeLibrary(hinstDll);
    }
    return dwVersion;
}

BOOL DLLEXPORT	isWinXP(void)
{
	return GetDllVersion("Shell32.dll") >= PACKVERSION(6,00);
}

WORD	viewToCmdID(int	iView)
{
	BOOL bXP = isWinXP();
	switch( iView )
	{
		case V_DETAILS:
			return CMD_2K_DETAILS;
		case V_LIST:
			return (bXP) ? CMD_XP_LIST : CMD_2K_LIST;
		case V_SMICONS:
			return (bXP) ? 0 : CMD_2K_SMICONS;
		case V_LGICONS:
			return (bXP) ? CMD_XP_LGICONS : CMD_2K_LGICONS;
		case V_THUMBS:
			return (bXP) ? CMD_XP_THUMBS : CMD_2K_THUMBS;
		case V_TILES:
			return (bXP) ? CMD_XP_TILES : 0;
	}
	return CMD_2K_LIST;
}


WORD	focusToCtlID(int	iFocus)
{
	//BOOL bXP = isWinXP();
	switch( iFocus )
	{
		case F_DIRLIST:
			return CID_DIRLIST;
		case F_FNAME:
			return CID_FNAME;
		case F_FTYPE:
			return CID_FTYPE;
		case F_PLACES:
			return CID_PLACES;
		case F_LOOKIN:
			return CID_LOOKIN;
	}
	return CID_DIRLIST;
}

int focusDlgItem(HWND hwnd, int iFocus)
{
	UINT uID = focusToCtlID(gOwShared.iFocus);
	if( uID == CID_DIRLIST )
	{
		return SetFocus( GetDlgItem( GetDlgItem(hwnd, CID_DIRLISTPARENT) , uID) ) != NULL;
	}
	else
		return SetFocus( GetDlgItem(hwnd, uID)) != NULL;
}

void CALLBACK timerProc(HWND hwnd, UINT uMsg, UINT uID, DWORD dwTime)
{
	KillTimer(hwnd, 251177);
	//gbDbg = FALSE;
	openWide(hwnd);
//	SendMessage(hwnd, CDM_SETCONTROLTEXT, edt1, (LPARAM)"You've been hooked in!!!");
}


static LRESULT CALLBACK CBTProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    HWND hwnd;

    if(nCode < 0)
        return CallNextHookEx(ghMsgHook, nCode, wParam, lParam);

    switch(nCode)
    {
	case HCBT_CREATEWND:
		{
			static char buf[256];
			HWND	hwNew = (HWND)wParam;
			CBT_CREATEWND * pcw = (CBT_CREATEWND *)lParam;
			CREATESTRUCT * pcs = (CREATESTRUCT*) pcw->lpcs;

			if( getSharedData() )
			{
				if( GetClassName(hwNew, buf, 255) && pcs->lpszName )
				{
					DWORD style = (DWORD)GetWindowLong(hwNew, GWL_STYLE);
					DWORD exStyle = (DWORD)GetWindowLong(hwNew, GWL_EXSTYLE);
//					dbg("style: %x, exStyle: %x", style, exStyle);
					if(	style == OW_MATCH_STYLE && exStyle == OW_MATCH_EXSTYLE
					&&	strcmp(buf, "#32770") == 0
					&& strcmp(pcs->lpszName, "Open")==0 )
					{
						wsprintf(buf, "Patched window %p", hwNew);
						SendMessage(gOwShared.hwLog, LB_ADDSTRING, 0, (LPARAM)buf);
						dbg(buf);
						pcs->style &= ~WS_VISIBLE;
						ShowWindow(hwNew, SW_HIDE);
//						dbg("Subclassing now:");
						subclass(hwNew, wpSubMain, 0);
//						SetTimer(hwNew, 251177, 20, timerProc);
					}
				}
			}

/*			wsprintf(buf, "CreateWindow: %p, %p(cbtdata)", hwNew, pcw);
			SendDlgItemMessage(ghwNotify, IDL_LOG, LB_ADDSTRING, 0, (LPARAM)buf);

			wsprintf(buf, "  hInstance: %p", pcs->hInstance);
			SendDlgItemMessage(ghwNotify, IDL_LOG, LB_ADDSTRING, 0, (LPARAM)buf);
			wsprintf(buf, "  hMenu: %p", pcs->hMenu);
			SendDlgItemMessage(ghwNotify, IDL_LOG, LB_ADDSTRING, 0, (LPARAM)buf);
			wsprintf(buf, "  hwndParent: %p", pcs->hwndParent);
			SendDlgItemMessage(ghwNotify, IDL_LOG, LB_ADDSTRING, 0, (LPARAM)buf);
			wsprintf(buf, "  cx: %d, cy: %d", pcs->cx, pcs->cy);
			SendDlgItemMessage(ghwNotify, IDL_LOG, LB_ADDSTRING, 0, (LPARAM)buf);
			wsprintf(buf, "  x: %d, y: %d", pcs->x, pcs->y);
			SendDlgItemMessage(ghwNotify, IDL_LOG, LB_ADDSTRING, 0, (LPARAM)buf);
			wsprintf(buf, "  style: %x", pcs->style);
			SendDlgItemMessage(ghwNotify, IDL_LOG, LB_ADDSTRING, 0, (LPARAM)buf);
			wsprintf(buf, "  name: %p [%.8s]", pcs->lpszName, pcs->lpszName ? pcs->lpszName : "null");
			SendDlgItemMessage(ghwNotify, IDL_LOG, LB_ADDSTRING, 0, (LPARAM)buf);
			wsprintf(buf, "  class: %p", pcs->lpszClass);
			SendDlgItemMessage(ghwNotify, IDL_LOG, LB_ADDSTRING, 0, (LPARAM)buf);
			wsprintf(buf, "  exStyle: %x", pcs->dwExStyle);
			SendDlgItemMessage(ghwNotify, IDL_LOG, LB_ADDSTRING, 0, (LPARAM)buf);


			if( GetClassName(hwNew, buf, 255) )
			{
				SendDlgItemMessage(ghwNotify, IDL_LOG, LB_ADDSTRING, 0, (LPARAM)buf);
			}
*/
		}
		return 0;
/*    case HCBT_ACTIVATE:
		{
			ghwNotify = getSharedData();
			CBTACTIVATESTRUCT * pac = (CBTACTIVATESTRUCT *)lParam;

			SendMessage(ghwNotify, WM_USER, 0, (LPARAM)pac->hWndActive);

			hwnd = (HWND)wParam;
			dbg("Got activate: WPARAM[old window]==(%0lx) LPARAM[CBTACTIVATESTRUCT*](%0lx)->{%s-activate, [new window]%p} ::: Sending to window %p",
				wParam, lParam, pac->fMouse ? "mouse" : "key", pac->hWndActive, ghwNotify);

			HWND hwPrev = GetActiveWindow();
		}
		return 0;*/
     }
    // Call the next hook, if there is one
    return CallNextHookEx(ghMsgHook, nCode, wParam, lParam);
}


void releaseSharedMem(void)
{
	DWORD dwRes = WaitForSingleObject(ghMutex, INFINITE);
	if( dwRes == WAIT_OBJECT_0 )
		CloseHandle(ghMutex);
}

int DLLEXPORT	setHook(HWND hwLB)
{
	//if( timeoutSeconds == 0 )
	//	return MessageBox(hwParent, szMsg, szTitle, uType);
	if(gbHooked)
	{
		SendMessage(gOwShared.hwLog, LB_ADDSTRING, 0, (LPARAM)"Already hooked");
		return 1;
	}
	else if( !initSharedMem() )
		return FALSE;

//	dbg("List box window handle is %p", hwLB);
	if(!getSharedData())
	{
		SendMessage(hwLB, LB_ADDSTRING, 0, (LPARAM)"Failed to get shared data!!!");
		return FALSE;
	}
	SendMessage(gOwShared.hwLog, LB_ADDSTRING, 0, (LPARAM)"Setting hook....");
	ghMsgHook = SetWindowsHookEx(
						WH_CBT,
				        CBTProc,
				        ghInst,
				        0 //GetCurrentThreadId()            // Only install for THIS thread!!!
				);
	if(ghMsgHook != NULL)
	{
		gbHooked = TRUE;
		SendMessage(gOwShared.hwLog, LB_ADDSTRING, 0, (LPARAM)"Hooked ok!");
		return 1;
	}
	SendMessage(gOwShared.hwLog, LB_ADDSTRING, 0, (LPARAM)"Failed hook");
	return 0;
}


int DLLEXPORT rmvHook(void)
{
	if( !gbHooked || !ghMsgHook )
		return 1;

	if( ghSysMsgHook )
	{
		UnhookWindowsHookEx(ghSysMsgHook);
		ghSysMsgHook = NULL;
	}

	if( ghMsgHook )
	{
		UnhookWindowsHookEx(ghMsgHook);
		gbHooked = FALSE;
		ghMsgHook = NULL;
	}
	releaseSharedMem();
	return 1;
}

