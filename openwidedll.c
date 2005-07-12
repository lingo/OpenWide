/* --- The following code comes from c:\lcc\lib\wizard\dll.tpl. */
#include <windows.h>
#include <commctrl.h>
#include	<shlwapi.h>
#include	<shellapi.h>
#include	<stdarg.h>
#include	<stdio.h>
#include	<dlgs.h>
#include	"openwidedll.h"
#include	"openwideres.h"
#include	"owDllInc.h"
#include	"owSharedUtil.h"


HINSTANCE		ghInst = NULL;
HANDLE			ghMutex = NULL;
POWSharedData	gpSharedMem = NULL;	// pointer to shared mem
HANDLE			ghMap		= NULL;
HHOOK			ghMsgHook	= NULL, ghSysMsgHook = NULL;

OWSharedData	gOwShared; // stores copy of shared mem for access to non-pointer datas without extended blocking

BOOL __declspec(dllexport) WINAPI DLLPROC(HINSTANCE hDLLInst, DWORD fdwReason, LPVOID lpvReserved)
{
    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
			ghInst = hDLLInst;
            break;
        case DLL_PROCESS_DETACH:
			ghInst = NULL;
	        break;
		default:
            break;
    }
    return TRUE;
}


int		openWide(HWND hwnd)
{
	dbg("DLL: openWide(%p)", hwnd);
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
				dbg("DLL: INITDIALOG");
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
/*		case WM_TIMER:
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
s*/
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
				dbg("DLL: DESTROY");
				if( openSharedMem() )
				{
					dbg("DLL: Opened shared memory");
					if( --gpSharedMem->refCount < 0 )
						gpSharedMem->refCount = 0;
					dbg("DLL: dec'd refCount to %d, posting msg %x to app window %p", gpSharedMem->refCount, gpSharedMem->iCloseMsg, gpSharedMem->hwListener);
					PostMessage( gpSharedMem->hwListener, gpSharedMem->iCloseMsg, 0,0);
					closeSharedMem();
					dbg("DLL: Closed shared memory");
				}
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

static BOOL isExcluded(const char *szApp)
{
	BOOL bEx = FALSE;
	HKEY hk = regOpenKey(HKEY_CURRENT_USER, OW_REGKEY_EXCLUDES_NAME);
	if( hk )
	{
		regGetDWORD(hk, szApp, &bEx);
		regCloseKey(hk);
	}
	return bEx;
}


static LRESULT CALLBACK CBTProc(int nCode, WPARAM wParam, LPARAM lParam)
{

    if(nCode < 0)
	{
		if( ghMsgHook )
	        return CallNextHookEx(ghMsgHook, nCode, wParam, lParam);
	}

    switch(nCode)
    {
	case HCBT_CREATEWND:
		{
			static char buf[256];
			HWND	hwNew = (HWND)wParam;
			CBT_CREATEWND * pcw = (CBT_CREATEWND *)lParam;
			CREATESTRUCT * pcs = (CREATESTRUCT*) pcw->lpcs;

			if( GetClassName(hwNew, buf, 255) && pcs->lpszName )
			{
				DWORD style = (DWORD)GetWindowLong(hwNew, GWL_STYLE);
				DWORD exStyle = (DWORD)GetWindowLong(hwNew, GWL_EXSTYLE);

				if(	style == OW_MATCH_STYLE && exStyle == OW_MATCH_EXSTYLE
					&&	strcmp(buf, "#32770") == 0
					&&	strcmp(pcs->lpszName, "Open") == 0 )
				{
					BOOL	bTakeOver = TRUE;

				//// FIND name of Calling app ////
					char *szApp = malloc( MAX_PATH+1 );
					if( szApp && GetModuleFileName(NULL, szApp, MAX_PATH) )
					{
						if( isExcluded(szApp) )
							bTakeOver = FALSE;
						dbg("DLL: Module: %s", szApp);
						free(szApp);
					}

					dbg("DLL: Found O&S dlg %p, attempting access to shared mem", hwNew);

					if( bTakeOver && openSharedMem() )
					{
						dbg("DLL: Opened shared memory");
						if( gpSharedMem->bDisable )
							bTakeOver = FALSE;
						else
							gpSharedMem->refCount++;
						dbg("DLL: Inc'd refCount to %d", gpSharedMem->refCount);
						closeSharedMem();
						dbg("DLL: Closed shared memory");
					}
					if( bTakeOver )
					{
						pcs->style &= ~WS_VISIBLE;
						ShowWindow(hwNew, SW_HIDE);
						subclass(hwNew, wpSubMain, 0);
					}
				}
			}
		}
		return 0;
     }
    // Call the next hook, if there is one
    return CallNextHookEx(ghMsgHook, nCode, wParam, lParam);
}

int	openSharedMem(void)
{
	if( ghMap != NULL || gpSharedMem != NULL )
		return 1;

	if(!waitForMutex())
		return 0;
	
	ghMap = OpenFileMapping(FILE_MAP_WRITE, TRUE, OW_SHARED_FILE_MAPPING);
	if( ghMap )
	{
		gpSharedMem = (POWSharedData)MapViewOfFile(ghMap, FILE_MAP_WRITE, 0,0, 0);
		if( gpSharedMem )
		{
			CopyMemory(&gOwShared, gpSharedMem, sizeof(OWSharedData));
			return 1;
		}
		else
			closeSharedMem();
	}
	return 0;
}

void closeSharedMem(void)
{
	if( gpSharedMem )
	{
		UnmapViewOfFile(gpSharedMem);
		gpSharedMem = NULL;
	}
	if( ghMap )
	{
		CloseHandle(ghMap);
		ghMap = NULL;
	}
	releaseMutex();
}

int getSharedData(void)
{
	int		rv = 0;
	if( openSharedMem() )
	{
		rv = 1;
		closeSharedMem();
	}
	return rv;
}


int __declspec(dllexport)	setHook(void)
{
	if(ghMsgHook != NULL)
	{
		//SendMessage(gOwShared.hwLog, LB_ADDSTRING, 0, (LPARAM)"Already hooked");
		return 1;
	}
	if(!getSharedData())
	{
//		SendMessage(hwLB, LB_ADDSTRING, 0, (LPARAM)"Failed to get shared data!!!");
		dbg("Hook failed to get shared mems");
		return FALSE;
	}
	//SendMessage(gOwShared.hwLog, LB_ADDSTRING, 0, (LPARAM)"Setting hook....");
	ghMsgHook = SetWindowsHookEx(WH_CBT, CBTProc, ghInst, 0);
	if(ghMsgHook != NULL)
	{
		dbg("Hooked okay");
		//SendMessage(gOwShared.hwLog, LB_ADDSTRING, 0, (LPARAM)"Hooked ok!");
		return 1;
	}
	dbg("Hook failed");
	//SendMessage(gOwShared.hwLog, LB_ADDSTRING, 0, (LPARAM)"Failed hook");
	return 0;
}


int __declspec(dllexport) rmvHook(void)
{
	if( !ghMsgHook )
		return 1;

	if( ghSysMsgHook )
	{
		UnhookWindowsHookEx(ghSysMsgHook);
		ghSysMsgHook = NULL;
	}
	dbg("DLL: Removing hook");
	if( ghMsgHook )
	{
		UnhookWindowsHookEx(ghMsgHook);
		ghMsgHook = NULL;
	}
	dbg("DLL: removed hook okay");
	return 1;
}



/*
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
		default:
			break;
	}
    // Call the next hook, if there is one
    return CallNextHookEx(ghSysMsgHook, nCode, wParam, lParam);
}
*/