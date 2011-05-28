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


#include	<windows.h>
/**
 * @author  Luke Hudson
 * @licence GPL2
 */

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


// Command identifiers for the trayicon popup menu.
enum	TrayCommands	{	IDM_ABOUT = 100, IDM_SETTINGS, IDM_QUIT };

// Window handles for the various windows
HWND		ghwMain 	= NULL, ghwPropSheet	= NULL;

// Global instance handle
HINSTANCE	ghInstance	= NULL;

// Icon handles; these icons are used in a few places
HICON		ghIconLG		= NULL,	ghIconSm	= NULL;

// This is a pointer shared data structure, containing all the useful info
POWSharedData 	gPowData	= NULL;

// Handle to our shared memory area -- access to this is controlled by a mutex
HANDLE			ghSharedMem = NULL;

// Mutex handle -- used to limit access to the shared memory area.
HANDLE			ghMutex 	= NULL;

// Message id, from RegisterWindowMessage(...), which is used to notify closure of : TODO: find out!
UINT			giCloseMessage	= 0;



/*
 * Initialise the shared memory area 
 *
 * initShareMem( mainWindowHandle );
 *
 * @returns 1 on success, 0 on failure
 */
int initSharedMem(HWND hwnd)
{
	int rVal = 0;	// return value

	// Create the mutex
	ghMutex = CreateMutex(NULL, TRUE, OW_MUTEX_NAME);
	if( !ghMutex )
		return 0;
/*	else if( GetLastError() == ERROR_ALREADY_EXISTS)
	{
		CloseHandle(ghMutex);
		ghMutex = NULL;
		return 0;
	}*/

	POWSharedData	pRData = NULL;	// Pointer to registry data

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
	ghSharedMem = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, 
		sizeof(OWSharedData), OW_SHARED_FILE_MAPPING);
	if(ghSharedMem)
	{
		// map a view of the 'file'
		gPowData = (POWSharedData)MapViewOfFile(ghSharedMem, FILE_MAP_WRITE, 0,0,0);
		if( gPowData )
		{
			// clear the shared mem
			ZeroMemory(gPowData, sizeof(OWSharedData));
			// If we succeeded in loading saved data from the registry, 
			// then copy it to the shared memory area
			if( pRData )
				CopyMemory(gPowData, pRData, sizeof(OWSharedData));
			else
			{	// Nothing loaded from registry, so
				// initialise the memory to some useful default values.

				// Get screen dimensions
				int w,h;
				w = GetSystemMetrics(SM_CXSCREEN);
				h = GetSystemMetrics(SM_CYSCREEN);

				// Set the Open/Save dialog origin and width from the screen dimensions
				gPowData->ptOrg.x = w/2 - w/4;
				gPowData->ptOrg.y = (h - 2*h/3)/2;
				gPowData->szDim.cx = w/2;	// Screen width / 2
				gPowData->szDim.cy = 2*h/3;	// 2/3 of screen height
				gPowData->iView = V_DETAILS;	// Use details view
				gPowData->iFocus = F_DIRLIST;	// Focus directory listing
				gPowData->bShowIcon = 1;	// Show the tray icon
			}
			gPowData->hwListener = hwnd;	// Listener window for trayicon events, etc.
			gPowData->bDisable = 0;		// Enable the hook!
			gPowData->refCount = 0;		// reference counting TODO: add explanation
			giCloseMessage	= gPowData->iCloseMsg 
					= RegisterWindowMessage("Lingo.OpenWide.ProcessFinished.Message");
			rVal = 1;	// Return value to indicate success
		} // if( gPowData )
	} // if( ghSharedMem )

	if( pRData ) // free registry data
		free(pRData);

	releaseMutex(); // unlock Mutex, so shared memory area can be accessed.
	return rVal;
} // END of initSharedMem(...)



/*
 * Free the shared memory area, once it is no longer in use.
 */
void releaseSharedMem(void)
{
	if( ghMutex ) // Obtain a lock on the shared memory access mutex
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
		if( gPowData )
		{
			UnmapViewOfFile(gPowData);
			gPowData = NULL;
		}
		CloseHandle(ghSharedMem);
		ghSharedMem = NULL;
	}
	ReleaseMutex(ghMutex);
} // END of releaseSharedMem()

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

/* 
 * Determines if this application has a shortcut in the 'Start Menu->Startup' directory.
 */
BOOL	isStartupApp(HWND hwnd)
{
	static char szBuf[MAX_PATH+1];	// path buffer

	// Retrieve path to windows Startup folder
	HRESULT hRes = SHGetFolderPath(hwnd, CSIDL_STARTUP, NULL, SHGFP_TYPE_CURRENT, szBuf);
	if( hRes == S_OK )
	{
		PathAppend(szBuf, "OpenWide.lnk");	// Append the name of the link :: TODO - this should be cleverer, perhaps.
		return fileExists(szBuf);		// Check the shortcut exists :: TODO: Check if it points to the right place, too?
	}
	return FALSE;
}

/*
 * Makes this program start with Windows, by creating a shortcut in the Windows Startup folder
 *
 *	setStartupApp( parentWindowOfErrors, isStartupApp )
 *
 *	@param isStartupApp -- set to 1 to create the shortcut, 0 to remove it.
 *	@returns 1 on success, 0 on failure
 */
int		setStartupApp(HWND hwnd, BOOL	bSet)
{
	char szBuf[MAX_PATH+1], szModule[MAX_PATH+1];	// some path buffers

	// Empty the string buffers
	ZeroMemory(szBuf, sizeof(szBuf)/sizeof(char));
	ZeroMemory(szModule, sizeof(szBuf)/sizeof(char));

	// Retrieve the path to this executable
	if( !GetModuleFileName(NULL, szModule, MAX_PATH) )
		return 0;

	// Get the windows startup folder
	HRESULT hRes = SHGetFolderPath(hwnd, CSIDL_STARTUP, NULL, SHGFP_TYPE_CURRENT, szBuf);
	if( hRes != S_OK || !PathAppend(szBuf, "OpenWide.lnk"))	// Append link path to startup folder path as side-effect
		return 0;
	if( bSet )	// Create the shortcut?
	{	
		if( fileExists(szBuf) )
			hRes = delFile(hwnd, szBuf); // Delete existing if found
		hRes = CreateLink(szModule, szBuf, "OpenWide - control Open&Save dialogs..."); // Create new shortcut
		return hRes == S_OK;
	}
	else		// Delete the shortcut
	{
		return delFile(hwnd, szBuf);
	}
} // END of setStartupApp(...)


/*
 * Create an event-listener window, which will be used for communication between the 
 * app and DLL instances ???  TODO:: Check this!!
 *
 * NOTE: Looks like this is badly named, doesn't really do SFA with the listener win!!!  @see createListenerWindow()
 * @param hwnd -- parent window?
 */
int initListener(HWND hwnd)
{
	if(!initSharedMem(hwnd))
		return 0;		// Make sure the shared memory area is set up.

	BOOL bMinimize = FALSE, bIcon = TRUE;

// wait for then lock shared data
	if( !waitForMutex() )
			return 0;

	bMinimize	= gPowData->bStartMin;	// get the preferences from the shared memory
	bIcon		= gPowData->bShowIcon;

/// released shared data
	releaseMutex();

	if(bIcon)
		addTrayIcon(hwnd);

	// Show property sheet only if the preferences specify it.
	if( bMinimize )
		ghwPropSheet = NULL;
	else
		ghwPropSheet = showDlg(hwnd);

	// Create the main event hook
	if( !setHook() )
	{
		Warn( "Failed to set hook: %s", geterrmsg() );
		return 0;
	}
	return 1;
} // END of initListener(...)


/*
 * Create, and show the popup menu for the trayicon
 *
 * @param hwnd	-- window to receive WM_COMMAND messages.
 */
void	doTrayMenu(HWND hwnd)
{
	if( ghwPropSheet )	// Show the property sheet window
		SetForegroundWindow(ghwPropSheet);

	// Create the popup menu structure
	HMENU	hm = CreatePopupMenu();
	AppendMenu(hm, MF_STRING, IDM_ABOUT, "&About OpenWide...");
	AppendMenu(hm, MF_GRAYED | MF_SEPARATOR, 0, NULL);
	AppendMenu(hm, MF_STRING | (ghwPropSheet ? MF_GRAYED : 0), IDM_SETTINGS, "&Settings...");
	AppendMenu(hm, MF_STRING | (ghwPropSheet ? MF_GRAYED : 0), IDM_QUIT, "&Quit");

	SetMenuDefaultItem(hm, IDM_SETTINGS, FALSE);

	POINT pt;
	GetCursorPos(&pt);	// Find mouse location

	// Popup the menu
	TrackPopupMenu(hm, TPM_RIGHTBUTTON | TPM_BOTTOMALIGN | TPM_RIGHTALIGN,
		pt.x+8, pt.y+8, 0, hwnd, NULL);

	// Delete the menu structure again.
	DestroyMenu(hm);
} // END of doTrayMenu(...)

/**
 * Display the main property sheet dialog
 *
 *	@param hwnd -- parent window
 */
void	showSettingsDlg(HWND hwnd)
{
	if( ghwPropSheet )	// un-hide it if it exists already.
	{
		ShowWindow(ghwPropSheet, SW_SHOWNORMAL);
		SetForegroundWindow(ghwPropSheet);
	}
	else 			// create and show the dialog if it doesn't already exist
		ghwPropSheet = showDlg(hwnd);
} // END of showSettingsDlg(...)


/*
 * Quit the application, running cleanup tasks first.
 *
 * This is a high-level function which makes up part of the 'public' API of the program, 
 * at least, as far as it has such an clear designed concept :(
 *
 * TODO:  Make sure the above statement is actually correct, and sort the functions into these sorts of categories somewhere.
 */
void	doQuit(void) {
	if( ghwPropSheet )	// don't quit with the settings dialog open
	{
		SetForegroundWindow(ghwPropSheet);
		Warn("Please close the settings dialog box before trying to quit");
		return;
	}
	//rmvHook();
	if( waitForMutex() )
	{
		gPowData->bDisable = TRUE;	// Tell hook not to subclass any more windows

		if(gPowData->refCount == 0)	// if there are no more subclassed windows
		{
			PostQuitMessage(0);	// quit program
		}
		releaseMutex();
	}
	remTrayIcon(ghwMain);
}


/*
 * Listener window callback
 */
LRESULT WINAPI CALLBACK wpListener(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
	static UINT uTaskbarMsg=0;
	switch(msg)
	{
		case WM_CREATE:
			if(!initListener(hwnd))
			{
				Warn("OpenWide failed to initialize properly.  Please report this bug:\r\nErr msg: %s", geterrmsg());
				return -1;
			}
			uTaskbarMsg = RegisterWindowMessage("TaskbarCreated");		// TODO:: WHAT IS THIS?
			break;

// Handle trayicon menu commands
		case WM_COMMAND:
			switch(LOWORD(wp))
			{
				case IDM_ABOUT:
					MessageBox(hwnd, "OpenWide is written by Luke Hudson. (c)2005-2011\r\nSee http://speak.geek.nz", "About OpenWide", MB_OK);
					break;
				case IDM_SETTINGS:
					showSettingsDlg(hwnd);
					break;
				case IDM_QUIT:
					doQuit();
					break;
			}
			break;

// Handle trayicon interaction events
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
			else if( msg == uTaskbarMsg )	// taskbar re-created, re-add icon if option is enabled.
			{
				if( !waitForMutex() )
					return 0;
				BOOL bIcon		= gPowData->bShowIcon;
				releaseMutex();

				if(bIcon)	addTrayIcon(hwnd);
			}
			return DefWindowProc(hwnd, msg, wp, lp);
	}
	return 0;
}


/*
 * Callback for the placement window. This is the proxy window which can be 
 * resized and moved to set the desired Open/Save dialog dimensions. 
 */
BOOL WINAPI CALLBACK wpPlacement(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
	static LPRECT pr = NULL;
	static BOOL bXP, bWin7;	// Are we using WinXP ? -- Changes the minimum dialog dimensions.
	switch(msg)
	{
		case WM_INITDIALOG:
			pr = (LPRECT)lp;
			if(!pr)
				EndDialog(hwnd, 0);
			bXP = isWinXP();
			bWin7 = isWin7();
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

// Used to limit the minimum dimensions while resizing
		case WM_GETMINMAXINFO:
			{
				MINMAXINFO * pmm = (MINMAXINFO *)lp;
				if(!pmm)
					break;
				if( bXP )
				{
					pmm->ptMinTrackSize.x = OW_XP_MINWIDTH;
					pmm->ptMinTrackSize.y = OW_XP_MINHEIGHT;
				} else if (bWin7) {
					pmm->ptMinTrackSize.x = OW_7_MINWIDTH;
					pmm->ptMinTrackSize.y = OW_7_MINHEIGHT;
				} else {
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


/* Create our trayicon */
int		addTrayIcon(HWND hwnd)
{
	return Add_TrayIcon( ghIconSm, "OpenWide\r\nRight-Click for menu...", hwnd, WM_TRAYICON, 0);
}

/* Remove the trayicon! */
void	remTrayIcon(HWND hwnd)
{
	Rem_TrayIcon( hwnd, WM_TRAYICON, 0 );
}


/* Creates the listener window */
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

// TODO: Remove this useless function!
int createWin(void)
{	
	ghwMain = createListenerWindow();
	return 1;
}

// TODO: why the new naming convention ?
int ow_init(void)
{
	HRESULT hRes = CoInitialize(NULL);	// Initialise COM -- for some reason I forget, we need to do this.
	if( hRes != S_OK && hRes != S_FALSE )
		return 0;
	// Load the icons from the .exe resource section
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
	// This Mutex is used to ensure that only one instance of this app can be running at a time. 
	HANDLE	hMutex = NULL;
	hMutex = CreateMutex(NULL, FALSE, "OpenWide_App_Mutex");
	if( hMutex && GetLastError()==ERROR_ALREADY_EXISTS )
	{
		// This app is already loaded, so find its window and send it a message.
		CloseHandle(hMutex);
		HWND hw = FindWindowEx(NULL, NULL, "Lingo.OpenWide.Listener", "Lingo.OpenWide");
		if(hw)	// Fake a double click on the trayicon -- to show the window.
			SendMessage(hw, WM_TRAYICON, 0, WM_LBUTTONDBLCLK);
		return 0;	// Exit this instance of the app.
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
			ghwPropSheet=(void *)NULL;
		}
	}
//	dbg("OWApp: Shutting down");
	ow_shutdown();
	return 0;
}


