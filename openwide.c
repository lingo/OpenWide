#include	<windows.h>
#include	<objbase.h>
#include	<shobjidl.h>
#include	<shlguid.h>
#include	<shlobj.h>
#include	<shlwapi.h>
#include	<shellapi.h>
#include	<stdio.h>
#include	"openwidedll.h"
#include	"openwideres.h"
#include	"owutil.h"
#include	"owSharedUtil.h"
#include	"openwide_proto.h"


enum	TrayCommands	{	IDM_ABOUT = 100, IDM_SETTINGS, IDM_QUIT };

HWND		ghwMain 	= NULL, ghwPropSheet	= NULL;
HINSTANCE	ghInstance	= NULL;
//HWND		ghwListBox	= NULL;

HICON		ghIconLG		= NULL,	ghIconSm	= NULL;

POWSharedData 	gPowData	= NULL;
HANDLE			ghSharedMem = NULL;
HANDLE			ghMutex 	= NULL;
UINT			giCloseMessage	= 0;

int initSharedMem(HWND hwnd)
{
	int rVal = 0;
	ghMutex = CreateMutex(NULL, TRUE, OW_MUTEX_NAME);
	if( !ghMutex )
		return 0;
/*	else if( GetLastError() == ERROR_ALREADY_EXISTS)
	{
		CloseHandle(ghMutex);
		ghMutex = NULL;
		return 0;
	}*/
	POWSharedData	pRData = NULL;
	// try to read in saved settings from registry
	HKEY	hk = regCreateKey(HKEY_CURRENT_USER,  OW_REGKEY_NAME);
	if( hk )
	{
		BOOL	bOK;
		DWORD dwSize = regReadDWORD(hk, NULL, &bOK);
		if( bOK && dwSize == sizeof(OWSharedData) )
			pRData = (POWSharedData)regReadBinaryData(hk, "OWData");
		regCloseKey(hk);
	}

	// Create a file mapping object against the system virtual memory.
	// This will be used to share data between the application and instances of the dll.
	ghSharedMem = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(OWSharedData), OW_SHARED_FILE_MAPPING);
	if(ghSharedMem)
	{
		// map a view of the file
		gPowData = (POWSharedData)MapViewOfFile(ghSharedMem, FILE_MAP_WRITE, 0,0,0);
		if( gPowData )
		{
			// clear the shared mem
			ZeroMemory(gPowData, sizeof(OWSharedData));
			// if we succeeded in loading saved data, copy it back
			if( pRData )
				CopyMemory(gPowData, pRData, sizeof(OWSharedData));
			else
			{
				// initialise the memory to useful values.
				int w,h;
				w = GetSystemMetrics(SM_CXSCREEN);
				h = GetSystemMetrics(SM_CYSCREEN);
				gPowData->ptOrg.x = w/2 - w/4;
				gPowData->ptOrg.y = (h - 2*h/3)/2;
				gPowData->szDim.cx = w/2;
				gPowData->szDim.cy = 2*h/3;
				gPowData->iView = V_DETAILS;
				gPowData->iFocus = F_DIRLIST;
				gPowData->bShowIcon = 1;
			}
			gPowData->hwListener = hwnd;
			gPowData->bDisable = 0;
			gPowData->refCount = 0;
			giCloseMessage = gPowData->iCloseMsg = RegisterWindowMessage("Lingo.OpenWide.ProcessFinished.Message");
			rVal = 1;
		}
	}

	if( pRData )
		free(pRData);

	releaseMutex();
	return rVal;
}



void releaseSharedMem(void)
{
//	dbg("DLL: Waiting for mutex");
	if( ghMutex )
	{
		DWORD dwRes = WaitForSingleObject(ghMutex, INFINITE);
		if(dwRes != WAIT_OBJECT_0)
		{
//			dbg("DLL: Failed to get ownership of mutex in releaseSharedMem!!!");
			return;
		}
	}

//	dbg("DLL: Releasing file mapping");
	if( ghSharedMem )
	{
		if( gPowData );
		{
			UnmapViewOfFile(gPowData);
			gPowData = NULL;
		}
		CloseHandle(ghSharedMem);
		ghSharedMem = NULL;
	}
	ReleaseMutex(ghMutex);
}
/*
int	doApply(HWND hwnd)
{
	DWORD dwRes;
	if( ghMutex )
	{
		dwRes = WaitForSingleObject(ghMutex, INFINITE);
		switch(dwRes)
		{
			case WAIT_OBJECT_0:
				// okay, continue
				break;
			case WAIT_ABANDONED:
			default:
				return 0;
		}
	}
	BOOL bOK;
	int i = GetDlgItemInt(hwnd, IDE_LEFT, &bOK, TRUE);
	if( bOK )
		gPowData->ptOrg.x = i;
	i = GetDlgItemInt(hwnd, IDE_TOP, &bOK, TRUE);
	if( bOK )
		gPowData->ptOrg.y = i;
	i = GetDlgItemInt(hwnd, IDE_WIDTH, &bOK, TRUE);
	if( bOK )
		gPowData->szDim.cx = i;
	i = GetDlgItemInt(hwnd, IDE_HEIGHT, &bOK, TRUE);
	if( bOK )
		gPowData->szDim.cy = i;
	int idx = SendDlgItemMessage(hwnd, IDCB_FOCUS, CB_GETCURSEL, 0, 0);
	if( idx != CB_ERR )
	{
		int f = SendDlgItemMessage(hwnd, IDCB_FOCUS, CB_GETITEMDATA, idx, 0);
		if( f >= 0 && f < F_MAX )
			gPowData->iFocus = f;
	}
	idx = SendDlgItemMessage(hwnd, IDCB_VIEW, CB_GETCURSEL, 0, 0);
	if( idx != CB_ERR )
	{
		int v = SendDlgItemMessage(hwnd, IDCB_VIEW, CB_GETITEMDATA, idx, 0);
		if( v >= 0 && v < V_MAX )
			gPowData->iView = v;
	}
	gPowData->bStartMin = IsDlgButtonChecked(hwnd, IDC_STARTMIN) == BST_CHECKED;
	BOOL bWinStart = IsDlgButtonChecked(hwnd, IDC_WSTARTUP) == BST_CHECKED;

	HKEY	hk = regCreateKey(HKEY_CURRENT_USER,  OW_REGKEY_NAME);
	if( hk )
	{
		regWriteDWORD(hk, NULL, sizeof(OWSharedData));
		regWriteBinaryData(hk, "OWData", (byte *)gPowData, sizeof(OWSharedData));
		regCloseKey(hk);
	}

	if( ghMutex )
		ReleaseMutex(ghMutex);

	setStartupApp(hwnd, bWinStart);
	return 1;
}
*/


BOOL	isStartupApp(HWND hwnd)
{
	static char szBuf[MAX_PATH+1];
	HRESULT hRes = SHGetFolderPath(hwnd, CSIDL_STARTUP, NULL, SHGFP_TYPE_CURRENT, szBuf);
	if( hRes == S_OK )
	{
		PathAppend(szBuf, "OpenWide.lnk");
		return fileExists(szBuf);
	}
	return FALSE;
}

int		setStartupApp(HWND hwnd, BOOL	bSet)
{
	char szBuf[MAX_PATH+1], szModule[MAX_PATH+1];

	ZeroMemory(szBuf, sizeof(szBuf)/sizeof(char));
	ZeroMemory(szModule, sizeof(szBuf)/sizeof(char));

	if( !GetModuleFileName(NULL, szModule, MAX_PATH) )
		return 0;
	HRESULT hRes = SHGetFolderPath(hwnd, CSIDL_STARTUP, NULL, SHGFP_TYPE_CURRENT, szBuf);
	if( hRes != S_OK || !PathAppend(szBuf, "OpenWide.lnk"))
		return 0;
	if( bSet )
	{	
		if( fileExists(szBuf) )
			hRes = delFile(hwnd, szBuf);
		hRes = CreateLink(szModule, szBuf, "OpenWide - control Open&Save dialogs...");
		return hRes == S_OK;
	}
	else
	{
		return delFile(hwnd, szBuf);
	}
}

int initListener(HWND hwnd)
{
	if(!initSharedMem(hwnd))
		return 0;

	BOOL bMinimize = FALSE, bIcon = TRUE;

// wait for then lock shared data
	if( !waitForMutex() )
			return 0;

	bMinimize	= gPowData->bStartMin;
	bIcon		= gPowData->bShowIcon;

/// released shared data
	releaseMutex();

	if(bIcon)
		addTrayIcon(hwnd);

	if( bMinimize )
		ghwPropSheet = NULL;
	else
		ghwPropSheet = showDlg(hwnd);

	if( !setHook() )
	{
		Warn( "Failed to set hook: %s", geterrmsg() );
		return 0;
	}
	return 1;
}

void	doTrayMenu(HWND hwnd)
{
	if( ghwPropSheet )
		SetForegroundWindow(ghwPropSheet);

	HMENU	hm = CreatePopupMenu();
	AppendMenu(hm, MF_STRING, IDM_ABOUT, "&About OpenWide...");
	AppendMenu(hm, MF_GRAYED | MF_SEPARATOR, 0, NULL);
	AppendMenu(hm, MF_STRING | (ghwPropSheet ? MF_GRAYED : 0), IDM_SETTINGS, "&Settings...");
	AppendMenu(hm, MF_STRING | (ghwPropSheet ? MF_GRAYED : 0), IDM_QUIT, "&Quit");

	SetMenuDefaultItem(hm, IDM_SETTINGS, FALSE);

	POINT pt;
	GetCursorPos(&pt);
	TrackPopupMenu(hm, TPM_RIGHTBUTTON | TPM_BOTTOMALIGN | TPM_RIGHTALIGN,
		pt.x, pt.y, 0, hwnd, NULL);
	DestroyMenu(hm);
}

void	showSettingsDlg(HWND hwnd)
{
	if( ghwPropSheet )
	{
		ShowWindow(ghwPropSheet, SW_SHOWNORMAL);
		SetForegroundWindow(ghwPropSheet);
	}
	else
		ghwPropSheet = showDlg(hwnd);
}

void	doQuit(void)
{
	if( ghwPropSheet )
	{
		SetForegroundWindow(ghwPropSheet);
		Warn("Please close the settings dialog box before trying to quit");
		return;
	}
	//rmvHook();
	if( waitForMutex() )
	{
//		dbg("Setting bDisable");
		gPowData->bDisable = TRUE;
//		dbg("refCount is %d", gPowData->refCount);
		if(gPowData->refCount == 0)
		{
//			dbg("Posting quit message");
			PostQuitMessage(0);
		}
		releaseMutex();
	}
	remTrayIcon(ghwMain);
}

LRESULT WINAPI CALLBACK wpListener(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
	switch(msg)
	{
		case WM_CREATE:
			if(!initListener(hwnd))
			{
				Warn("OpenWide failed to initialize properly.  Please report this bug:\r\nErr msg: %s", geterrmsg());
				return -1;
			}
			break;
		case WM_COMMAND:
			switch(LOWORD(wp))
			{
				case IDM_ABOUT:
					MessageBox(hwnd, "OpenWide is written by Luke Hudson. (c)2005\r\nSee http://lingo.atspace.com", "About OpenWide", MB_OK);
					break;
				case IDM_SETTINGS:
					showSettingsDlg(hwnd);
					break;
				case IDM_QUIT:
					doQuit();
					break;
			}
			break;
		case WM_TRAYICON:
			switch(lp)
			{
				case WM_RBUTTONDOWN:
					doTrayMenu(hwnd);
					break;
				case WM_LBUTTONDBLCLK:
					showSettingsDlg(hwnd);
					break;
				case WM_LBUTTONUP:
/*					ShowWindow(hwnd, SW_SHOWNORMAL);
					EnableWindow(hwnd, TRUE);
					remTrayIcon(hwnd);
					SetFocus(hwnd);*/
					break;
			}
			break;
		case WM_DESTROY:
//			dbg("OWApp: listener destroying");
			rmvHook();
			releaseSharedMem();
			break;
		default:
			if( msg == giCloseMessage )
			{
//				dbg("OWApp: Received dll close msg");
				if( waitForMutex() )
				{
//					dbg("OWApp: refCount is %d", gPowData->refCount);
					if( gPowData->refCount == 0 && gPowData->bDisable)
						PostQuitMessage(0);
					releaseMutex();
				}
			}
			return DefWindowProc(hwnd, msg, wp, lp);
	}
	return 0;
}


BOOL WINAPI CALLBACK wpPlacement(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
	static LPRECT pr = NULL;
	static BOOL bXP;
	switch(msg)
	{
		case WM_INITDIALOG:
			pr = (LPRECT)lp;
			if(!pr)
				EndDialog(hwnd, 0);
			bXP = isWinXP();
			MoveWindow(hwnd, pr->left, pr->top, pr->right, pr->bottom, FALSE);
			break;
		case WM_COMMAND:
			switch(LOWORD(wp))
			{
				case IDOK:
					GetWindowRect(hwnd, pr);
					EndDialog(hwnd, 1);
					break;
				case IDCANCEL:
					EndDialog(hwnd, 0);
					break;
			}
			break;
		case WM_GETMINMAXINFO:
			{
				MINMAXINFO * pmm = (MINMAXINFO *)lp;
				if(!pmm)
					break;
				if( bXP )
				{
					pmm->ptMinTrackSize.x = OW_XP_MINWIDTH;
					pmm->ptMinTrackSize.y = OW_XP_MINHEIGHT;
				}
				else
				{
					pmm->ptMinTrackSize.x = OW_2K_MINWIDTH;
					pmm->ptMinTrackSize.y = OW_2K_MINHEIGHT;
				}
			}
			break;
		case WM_CLOSE:
			EndDialog(hwnd, 0);
			break;
		default:
			return 0;
	}
	return 1;
}

int		addTrayIcon(HWND hwnd)
{
	return Add_TrayIcon( ghIconSm, "OpenWide\r\nRight-Click for menu...", hwnd, WM_TRAYICON, 0);
}

void	remTrayIcon(HWND hwnd)
{
	Rem_TrayIcon( hwnd, WM_TRAYICON, 0 );
}

HWND	createListenerWindow(void)
{
	static BOOL bRegd = FALSE;
	if(!bRegd)
	{
		WNDCLASSEX	wc;
		ZeroMemory(&wc, sizeof(wc));
		wc.cbSize = sizeof(wc);
		wc.hInstance = ghInstance;
		wc.lpfnWndProc = wpListener;
		wc.lpszClassName = "Lingo.OpenWide.Listener";
		wc.hIcon = ghIconLG;
		wc.hIconSm = ghIconSm;
		bRegd = (RegisterClassEx(&wc) != 0);
	}
	return CreateWindowEx( WS_EX_NOACTIVATE, "Lingo.OpenWide.Listener", "Lingo.OpenWide",
		WS_POPUP, 0,0, 1,1, NULL, NULL, ghInstance, NULL);
}

int createWin(void)
{	
	ghwMain = createListenerWindow();
	return 1;
}

int ow_init(void)
{
	HRESULT hRes = CoInitialize(NULL);
	if( hRes != S_OK && hRes != S_FALSE )
		return 0;
	ghIconSm = (HICON)LoadImage(ghInstance, MAKEINTRESOURCE(IDI_TRAY), IMAGE_ICON, 16,16, LR_SHARED | LR_VGACOLOR);
	ghIconLG = (HICON)LoadIcon(ghInstance, MAKEINTRESOURCE(IDI_TRAY));
	if( !createWin() )
		return 0;
	return 1;
}


void ow_shutdown(void)
{
	if(ghwMain)
	{
		DestroyWindow(ghwMain);
		ghwMain = NULL;
	}
	CoUninitialize();
}


int WINAPI WinMain(HINSTANCE hi, HINSTANCE hiPrv, LPSTR fakeCmdLine, int iShow)
{
	HANDLE	hMutex = NULL;
	hMutex = CreateMutex(NULL, FALSE, "OpenWide_App_Mutex");
	if( hMutex && GetLastError()==ERROR_ALREADY_EXISTS )
	{
		CloseHandle(hMutex);
		HWND hw = FindWindowEx(NULL, NULL, "Lingo.OpenWide.Listener", "Lingo.OpenWide");
		if(hw)
			SendMessage(hw, WM_TRAYICON, 0, WM_LBUTTONDBLCLK);
		return 0;
	}
	ghInstance = hi;
	if( !ow_init() )
	{
		ow_shutdown();
		return 0;
	}

	MSG msg;
	int ret;

	while( (ret = GetMessage(&msg, NULL, 0,0) ) != 0 )
	{
		if( ret == -1 )
			break;
		if( !PropSheet_IsDialogMessage(ghwPropSheet, &msg) )	//IsDialogMessage(ghwMain, &msg) )
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		if( !PropSheet_GetCurrentPageHwnd(ghwPropSheet) )
		{
			DestroyWindow(ghwPropSheet);
			ghwPropSheet=NULL;
		}
	}
//	dbg("OWApp: Shutting down");
	ow_shutdown();
	return 0;
}


