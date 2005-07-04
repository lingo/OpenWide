#include	<windows.h>
#include	<shlwapi.h>
#include	<shellapi.h>
#include	<stdio.h>
#include	"openwidedll.h"
#include	"openwide.h"
#include	"winutil.h"

//#include	<shlguid.h>
#include	<shlobj.h>

#pragma lib "shell32.lib"
#pragma lib "shlwapi.lib"

////////////////////////////////////////////////////////////////////////////////
//  Defines

#define	OW_REGKEY_NAME	("Software\\HiveMind\\OpenWide")
#define	SETDLGRESULT(hw, x)	SetWindowLong((hw), DWL_MSGRESULT, (LONG)(x))

#define		RCWIDTH(r)	((r).right - (r).left + 1)
#define		RCHEIGHT(r)	((r).bottom - (r).top + 1)

#define		WM_TRAYICON		(WM_APP)

////////////////////////////////////////////////////////////////////////////////
//  Globals

HWND		ghwMain = NULL;
HINSTANCE	ghInstance = NULL;
HWND		ghwListBox = NULL;

static POWSharedData 	gPowData = NULL;
static HANDLE			ghSharedMem =  NULL;
static HANDLE			ghMutex = NULL;
static BOOL				gbChanged = FALSE;

////////////////////////////////////////////////////////////////////////////////
//  Prototypes

/* Copied by pro2 : (c)2004 Luke Hudson */
int  addTrayIcon(HWND hwnd);
int  cbAddString(HWND hwCB, const char *szStr, LPARAM lpData);
int createWin(void);
int doApply(HWND hwnd);
void fillFocusCB(HWND hwnd, UINT uID);
void fillViewCB(HWND hwnd, UINT uID);
int initApp(void);
int initSharedMem(HWND hwnd);
int initWin(HWND hwnd);
BOOL isChanged(void);
BOOL isStartupApp(HWND hwnd);
int lockSharedMem(void);
static void onCommand(HWND hwnd, WPARAM wp, LPARAM lp);
void releaseSharedMem(void);
void remTrayIcon(HWND hwnd);
void selectCBView(HWND hwnd, UINT uID, int iView);
int  setStartupApp(HWND hwnd, BOOL bSet);
void shutdownApp(void);
int unlockSharedMem(void);
int WINAPI WinMain(HINSTANCE hi, HINSTANCE hiPrv, LPSTR fakeCmdLine, int iShow);
BOOL WINAPI CALLBACK wpMain(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
static BOOL CALLBACK wpPageFolders(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
static BOOL CALLBACK wpPagePrefs(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
static BOOL CALLBACK wpPageWinSettings(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
BOOL WINAPI CALLBACK wpPlacement(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
BOOL WINAPI CALLBACK wpPlacement(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);

////////////////////////////////////////////////////////////////////////////////
//  Functions

void	setChanged(HWND hwnd, BOOL bChanged)
{
	gbChanged = bChanged;
	EnableWindow( GetDlgItem(hwnd, IDOK), bChanged);
	if( !bChanged )
		SetFocus( GetDlgItem(hwnd, IDB_TEST) );
}

BOOL	isChanged(void)
{
	return gbChanged;
}


static BOOL CALLBACK wpPageWinSettings(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
	switch (msg)
	{
		case WM_INITDIALOG:
			if( !initWin(hwnd) )
				SendMessage(hwnd, PSM_PRESSBUTTON, PSBTN_CANCEL, 0);
			break;
		case WM_COMMAND:
			switch(LOWORD(wp))
			{
				case IDCB_VIEW:
				case IDCB_FOCUS:
					if( HIWORD(wp) == CBN_SELCHANGE )
						setChanged(hwnd, TRUE);
					break;
				case IDE_LEFT:
				case IDE_TOP:
				case IDE_WIDTH:
				case IDE_HEIGHT:
					if( HIWORD(wp) == EN_CHANGE )
						setChanged(hwnd, TRUE);
					break;
				case IDB_QUIT:
					//DestroyWindow(hwnd);
					break;
				case IDB_SETPLACEMENT:
					{
						RECT r;
						r.left = GetDlgItemInt(hwnd, IDE_LEFT, NULL, TRUE);
						r.top = GetDlgItemInt(hwnd, IDE_TOP, NULL, TRUE);
						r.right = max( GetDlgItemInt(hwnd, IDE_WIDTH, NULL, FALSE), 100 );
						r.bottom = max( GetDlgItemInt(hwnd, IDE_HEIGHT, NULL, FALSE), 100 );

						if( DialogBoxParam(ghInstance, MAKEINTRESOURCE(IDD_PLACEMENT), hwnd, wpPlacement, (LPARAM)&r) )
						{
							SetDlgItemInt(hwnd, IDE_LEFT, r.left, TRUE);
							SetDlgItemInt(hwnd, IDE_TOP, r.top, TRUE);
							SetDlgItemInt(hwnd, IDE_WIDTH, r.right - r.left, TRUE);
							SetDlgItemInt(hwnd, IDE_HEIGHT, r.bottom - r.top, TRUE);
							setChanged(hwnd, TRUE);
						}
					}
					break;
				case IDB_TEST:
		//			rmvHook();
					//Prompt_File_Name(0, hwnd, NULL, NULL);
		//			setHook(hwnd);
					propSheet(hwnd, "Testing", NULL, 3, IDD_WINSETTINGS, wpPageWinSettings, IDD_FOLDERS, wpPageFolders, IDD_PREFS, wpPagePrefs);
					break;
			}
			break;
		case WM_NOTIFY:
			{
				LPNMHDR pnh = (LPNMHDR)lp;
				dbg("%s, WM_NOTIFY from %p, %d", __func__, pnh->hwndFrom, pnh->idFrom);
//				if( pnh->hwndFrom == hwPropSheet )
//				{
					switch(pnh->code)
					{
						case PSN_SETACTIVE:
							SETDLGRESULT(hwnd, FALSE);
							return FALSE;
						case PSN_APPLY:
							{
								SETDLGRESULT(hwnd, PSNRET_NOERROR);
								return TRUE;
							}
							break;
					}
//				}
			}
			break;
		case WM_DESTROY:
			break;
		case WM_CLOSE:
			break;
	}
	return 0;
}

static BOOL CALLBACK wpPageFolders(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
	switch (msg)
	{
		case WM_INITDIALOG:
			break;
		case WM_COMMAND:
/*			switch (LOWORD(wp))
			{
			}
			break;*/
			break;
		case WM_NOTIFY:
			{
				LPNMHDR pnh = (LPNMHDR)lp;
				dbg("%s, WM_NOTIFY from %p, %d", __func__, pnh->hwndFrom, pnh->idFrom);
//				if( pnh->hwndFrom == hwPropSheet )
//				{
					switch(pnh->code)
					{
						case PSN_SETACTIVE:
							SETDLGRESULT(hwnd, FALSE);
							return FALSE;
						case PSN_APPLY:
							{
								SETDLGRESULT(hwnd, PSNRET_NOERROR);
								return TRUE;
							}
							break;
					}
//				}
			}
			break;
		case WM_DESTROY:
			break;
		case WM_CLOSE:
			break;
	}
	return 0;
}

static BOOL CALLBACK wpPagePrefs(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
	switch (msg)
	{
		case WM_INITDIALOG:
			break;
		case WM_COMMAND:
			switch (LOWORD(wp))
			{
				case IDC_WSTARTUP:
				case IDC_STARTMIN:
					setChanged(hwnd, HIWORD(wp)==BN_CLICKED);
					break;
/*				case IDC_LOG:
					{
						RECT rw, rc;
						GetWindowRect(hwnd, &rw);
						if( IsWindowVisible(GetDlgItem(hwnd, IDG_LOG)) )
						{
							GetWindowRect( GetDlgItem(hwnd, IDC_LOG), &rc);
							SetWindowPos(hwnd, NULL, 0,0, RCWIDTH(rw), rc.bottom+dlgUnits2Pix(hwnd, 6, FALSE) - rw.top, SWP_NOMOVE | SWP_NOZORDER);
							ShowWindow( GetDlgItem(hwnd, IDG_LOG), SW_HIDE);
							ShowWindow( GetDlgItem(hwnd, IDL_LOG), SW_HIDE);
						}
						else
						{
							GetWindowRect( GetDlgItem(hwnd, IDG_LOG), &rc);
							SetWindowPos(hwnd, NULL, 0,0, RCWIDTH(rw), rc.bottom+dlgUnits2Pix(hwnd, 6, FALSE) - rw.top, SWP_NOMOVE | SWP_NOZORDER);
							ShowWindow( GetDlgItem(hwnd, IDG_LOG), SW_SHOW);
							ShowWindow( GetDlgItem(hwnd, IDL_LOG), SW_SHOW);
						}
					}
					break;
				case IDOK:
					if( doApply(hwnd) )
						setChanged(hwnd, FALSE);
					break;*/
			}
			break;
		case WM_NOTIFY:
			{
				LPNMHDR pnh = (LPNMHDR)lp;
				dbg("%s WM_NOTIFY from %p, %d", __func__, pnh->hwndFrom, pnh->idFrom);
//				if( pnh->hwndFrom == hwPropSheet )
//				{
					switch(pnh->code)
					{
						case PSN_SETACTIVE:
							SETDLGRESULT(hwnd, FALSE);
							return FALSE;
						case PSN_APPLY:
							{
								SETDLGRESULT(hwnd, PSNRET_NOERROR);
								return TRUE;
							}
							break;
					}
//				}
			}
			break;
		case WM_DESTROY:
			break;
		case WM_CLOSE:
			break;
	}
	return 0;
}

int initFavourites(POWSharedData powData)
{
	powData->pFaves = NULL;
	powData->nFaves = 0;
	powData->dwFaveSize = 0;
	powData->hFavMap = NULL;
	const char *szFile = getLocalPath("favourites.dat");

	powData->hFavFile = CreateFile( szFile, FILE_ALL_ACCESS, 0, NULL, TRUNCATE_EXISTING, FILE_ATTRIBUTE_HIDDEN, NULL);
	if( powData->hFavFile == INVALID_HANDLE_VALUE )
		powData->hFavFile = CreateFile( szFile, FILE_ALL_ACCESS, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_HIDDEN, NULL);

	dbg("%s, Opened favourite file \"%s\": got handle %p", __func__, szFile, powData->hFavFile);
	if( powData->hFavFile != INVALID_HANDLE_VALUE )
	{
		powData->dwFaveSize = max( sizeof(FavLink), powData->dwFaveSize );
		powData->hRootMap = CreateFileMapping(powData->hFavFile, NULL, 
			PAGE_READWRITE, 0, powData->dwFaveSize, OW_FAVES_FILE_MAPPING);

		if( powData->hRootMap && lockFaves(powData) )
		{
			HKEY	hk = regOpenKey(HKEY_CURRENT_USER,  OW_REGKEY_NAME);
			if( hk )
			{
				BOOL bOK;
				powData->nFaves = 0;

				int nFaves = regReadDWORD(hk, "NumFavourites", &bOK);
				dbg("%s, registry says there are %d faves", __func__, nFaves);
				if( !bOK )	
					nFaves = 0;

				char szName[128];
				int nAdded = 0;
				for(int i=0; i < nFaves; i++ )
				{
					wsprintf(szName, "Favourite%02d", i);
					char *szFav = regReadSZ(hk, szName);
					if(szFav)
					{
						dbg("%s, %s was '%s'", __func__, szName, szFav);
						if( !faveExists(powData, szFav) )
						{
							if( newFav(powData, szFav) )
								nAdded++;
							else
								dbg("%s, error creating new favourite '%s'", __func__, szFav);
						}
						free(szFav);
					}
					else
						dbg("%s, failed to read fave: '%s'", __func__, szName);
				}
				dbg("%s, added %d faves okay", __func__, nAdded);
				//powData->nFaves = nAdded;
				regCloseKey(hk);
			}
		}
	}
	return 1;
}

int		saveFaves2Registry(FavLink pFaves[], int nFaves)
{
	int nSaved = 0;
	HKEY	hk = regCreateKey(HKEY_CURRENT_USER,  OW_REGKEY_NAME);
	if( hk )
	{
		char szName[128];
		for(int i=0; i < nFaves; i++)
		{
			wsprintf(szName, "Favourite%02d", i);
			regWriteSZ(hk, szName, pFaves[i].szFav);
			nSaved++;
		}
		regWriteDWORD(hk, "NumFavourites", nSaved);
		regCloseKey(hk);
	}
	return nSaved;
}


void	shutdownFavourites(POWSharedData pow)
{
	unlockFaves(pow);
	if( pow->hRootMap )
	{
		if( lockFaves(pow) )
		{
			saveFaves2Registry(pow->pFaves, pow->nFaves);
			unlockFaves(pow);
		}
		CloseHandle(pow->hRootMap);
		pow->hRootMap = NULL;
	}
	if( pow->hFavFile != INVALID_HANDLE_VALUE )
	{
		CloseHandle(pow->hFavFile);
		pow->hFavFile = INVALID_HANDLE_VALUE;
	}
}

int initSharedMem(HWND hwnd)
{
	int rVal;
	rVal = 0;
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
			{
				CopyMemory(gPowData, pRData, sizeof(OWSharedData));
				//gPowData->iUserView = gPowData->iView;
			}
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
				gPowData->iView = V_DETAILS; //gPowData->iUserView ..
				gPowData->iFocus = F_DIRLIST;
			}
			initFavourites(gPowData);
			gPowData->hwLog = hwnd;
			rVal = 1;
		}
	}

	if( pRData )
		free(pRData);

	ReleaseMutex(ghMutex);
	return rVal;
}

int lockSharedMem(void)
{
	if( ghMutex )
	{
		DWORD dwRes = WaitForSingleObject(ghMutex, INFINITE);
		switch(dwRes)
		{
			case WAIT_OBJECT_0:
				return 1;
			case WAIT_ABANDONED:
			default:
				return 0;
		}
	}
	else
		return 0;
}

int	unlockSharedMem(void)
{
	if( !lockSharedMem() )
		return 0;
	if( ghMutex )
		ReleaseMutex(ghMutex);
	else
		return 0;
	return 1;
}


void releaseSharedMem(void)
{
	//dbg("%s, Waiting for mutex", __func__);
	if( ghMutex )
	{
		DWORD dwRes = WaitForSingleObject(ghMutex, INFINITE);
		if(dwRes != WAIT_OBJECT_0)
		{
			dbg("%s, Failed to get ownership of mutex in releaseSharedMem!!!", __func__);
			return;
		}
	}

	//dbg("%s, Releasing file mapping", __func__);
	if( ghSharedMem )
	{
		if( gPowData );
		{
			shutdownFavourites(gPowData);
			UnmapViewOfFile(gPowData);
			gPowData = NULL;
		}
		CloseHandle(ghSharedMem);
		ghSharedMem = NULL;
	}
	ReleaseMutex(ghMutex);
}

int	doApply(HWND hwnd)
{
	DWORD dwRes;
	if( !lockSharedMem() )
		return 0;
	//dbg("%s, openwide.exe: Locked shared mem okay", __func__);
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
		regWriteBinaryData(hk, "OWData", gPowData, sizeof(OWSharedData));
		regCloseKey(hk);
	}

	//dbg("%s, openwide.exe: Releasing shared mem...", __func__);
	unlockSharedMem();
	setStartupApp(hwnd, bWinStart);
	return 1;
}


int		cbAddString(HWND hwCB, const char *szStr, LPARAM lpData)
{
	int idx = SendMessage(hwCB, CB_ADDSTRING, 0, (LPARAM)szStr);
	if( idx != CB_ERR )
	{
		int res = SendMessage(hwCB, CB_SETITEMDATA, idx, lpData);
		if( res == CB_ERR )
		{
			SendMessage(hwCB, CB_DELETESTRING, idx, 0);
			idx = CB_ERR;
		}
	}
	return idx;
}

void	fillFocusCB(HWND hwnd, UINT uID)
{
	HWND hwCB = GetDlgItem(hwnd, uID);
	if(!hwCB)
		return;
	SendMessage(hwCB, CB_RESETCONTENT, 0,0);
	cbAddString(hwCB, "Directory listing", F_DIRLIST);
	cbAddString(hwCB, "File name", F_FNAME);
	cbAddString(hwCB, "File type", F_FTYPE);
	cbAddString(hwCB, "Places bar", F_PLACES);
	cbAddString(hwCB, "Look in", F_LOOKIN);
}


void	fillViewCB(HWND hwnd, UINT uID)
{
	HWND hwCB = GetDlgItem(hwnd, uID);
	if(!hwCB)
		return;
	SendMessage(hwCB, CB_RESETCONTENT, 0,0);
	cbAddString(hwCB, "Large Icons", V_LGICONS);
	if( isWinXP() )
	cbAddString(hwCB, "Tiles", V_TILES);
	else
	cbAddString(hwCB, "Small Icons", V_SMICONS);
	cbAddString(hwCB, "List", V_LIST);
	cbAddString(hwCB, "Details", V_DETAILS);
	cbAddString(hwCB, "Thumbnails", V_THUMBS);
}

void	selectCBView(HWND hwnd, UINT uID, int iView)
{
	HWND hwCB = GetDlgItem(hwnd, uID);
	if(!hwCB)
		return;
	int nItems = SendMessage(hwCB, CB_GETCOUNT, 0,0);
	for(int i=0; i < nItems; i++)
	{
		int ivItem = (int)SendMessage(hwCB, CB_GETITEMDATA, i, 0);
		if( ivItem == iView)
		{
			SendMessage(hwCB, CB_SETCURSEL, i, 0);
			break;
		}
	}
}


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

	if( !GetModuleFileName(NULL, szModule, MAX_PATH) )
		return 0;
	HRESULT hRes = SHGetFolderPath(hwnd, CSIDL_STARTUP, NULL, SHGFP_TYPE_CURRENT, szBuf);
	if( hRes != S_OK || !PathAppend(szBuf, "OpenWide.lnk"))
		return 0;
	if( bSet )
	{
		hRes = CreateLink(szModule, szBuf, "OpenWide - control Open&Save dialogs...");
		return hRes == S_OK;
	}
	else
	{
		int len = strlen(szBuf);
		for(int i=MAX_PATH; i > len; i--)
			szBuf[i] = 0;

		SHFILEOPSTRUCT shlOp = {0};
		shlOp.hwnd = hwnd;
		shlOp.wFunc = FO_DELETE;
		shlOp.pFrom = szBuf;
		shlOp.fFlags = FOF_ALLOWUNDO | FOF_NOCONFIRMATION | FOF_SILENT;
		return SHFileOperation(&shlOp) == S_OK;
	}
}

int initWin(HWND hwnd)
{
/*	RECT r;
	GetClientRect(hwnd, &r);
	InflateRect(&r, -16,-16);
	int w,h;
	w = r.right - r.left;
	h = r.bottom - r.top;
	ghwListBox = CreateWindow("LISTBOX", NULL, WS_VISIBLE | WS_BORDER | WS_CHILD | LBS_USETABSTOPS | LBS_WANTKEYBOARDINPUT | LBS_NOTIFY,
		r.left, r.top, w,h, hwnd, (HMENU)1000, hInst, NULL);*/

	ghwListBox = GetDlgItem(hwnd, IDL_LOG);
	if(!initSharedMem(ghwListBox))
		return 0;

	CheckDlgButton(hwnd, IDC_WSTARTUP, isStartupApp(hwnd));
	CheckDlgButton(hwnd, IDC_LOG, BST_CHECKED);
	fillViewCB(hwnd, IDCB_VIEW);
	fillFocusCB(hwnd, IDCB_FOCUS);

	BOOL bMinimize = FALSE;

// wait for then lock shared data
	if(ghMutex)
	{
		DWORD dwRes = WaitForSingleObject(ghMutex, 1000);
		if(dwRes != WAIT_OBJECT_0)
			return 0;
	}

	bMinimize = gPowData->bStartMin;

	SetDlgItemInt(hwnd, IDE_LEFT, gPowData->ptOrg.x, TRUE);
	SetDlgItemInt(hwnd, IDE_TOP, gPowData->ptOrg.y, TRUE);
	SetDlgItemInt(hwnd, IDE_WIDTH, gPowData->szDim.cx, FALSE);
	SetDlgItemInt(hwnd, IDE_HEIGHT, gPowData->szDim.cy, FALSE);

	selectCBView(hwnd, IDCB_VIEW, gPowData->iView);
	SendDlgItemMessage(hwnd, IDCB_FOCUS, CB_SETCURSEL, gPowData->iFocus, 0);

	CheckDlgButton(hwnd, IDC_STARTMIN, gPowData->bStartMin);

	if( ghMutex )
		ReleaseMutex(ghMutex);
/// released shared data

	EnableWindow( GetDlgItem(hwnd, IDOK), FALSE);
	ShowWindow(GetDlgItem(hwnd, IDC_LOG), FALSE);
	ShowWindow(GetDlgItem(hwnd, IDG_LOG), FALSE);
	ShowWindow(GetDlgItem(hwnd, IDL_LOG), FALSE);

	RECT rc,rw;
	GetWindowRect( GetDlgItem(hwnd, IDC_LOG), &rc);
	GetWindowRect( hwnd, &rw);
	SetWindowPos(hwnd, NULL, 0,0, RCWIDTH(rw), rc.top+dlgUnits2Pix(hwnd, 6, FALSE) - rw.top, SWP_NOMOVE | SWP_NOZORDER);

	if( bMinimize )
	{
		SendMessage(hwnd, WM_SYSCOMMAND, SC_MINIMIZE, 0);
		SetFocus(NULL);
		EndTrayOperation();
	}
	else
	{
		SetFocus(GetDlgItem(hwnd, IDB_SETPLACEMENT));
		ShowWindow(hwnd, SW_SHOWNORMAL);
	}
	return 1;
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


static void onCommand(HWND hwnd, WPARAM wp, LPARAM lp)
{
	switch(LOWORD(wp))
	{
		case IDCB_VIEW:
		case IDCB_FOCUS:
			if( HIWORD(wp) == CBN_SELCHANGE )
				setChanged(hwnd, TRUE);
			break;
		case IDE_LEFT:
		case IDE_TOP:
		case IDE_WIDTH:
		case IDE_HEIGHT:
			if( HIWORD(wp) == EN_CHANGE )
				setChanged(hwnd, TRUE);
			break;
		case IDB_QUIT:
			DestroyWindow(hwnd);
			break;
		case IDB_SETPLACEMENT:
			{
				RECT r;
				r.left = GetDlgItemInt(hwnd, IDE_LEFT, NULL, TRUE);
				r.top = GetDlgItemInt(hwnd, IDE_TOP, NULL, TRUE);
				r.right = max( GetDlgItemInt(hwnd, IDE_WIDTH, NULL, FALSE), 100 );
				r.bottom = max( GetDlgItemInt(hwnd, IDE_HEIGHT, NULL, FALSE), 100 );

				if( DialogBoxParam(ghInstance, MAKEINTRESOURCE(IDD_PLACEMENT), hwnd, wpPlacement, (LPARAM)&r) )
				{
					SetDlgItemInt(hwnd, IDE_LEFT, r.left, TRUE);
					SetDlgItemInt(hwnd, IDE_TOP, r.top, TRUE);
					SetDlgItemInt(hwnd, IDE_WIDTH, r.right - r.left, TRUE);
					SetDlgItemInt(hwnd, IDE_HEIGHT, r.bottom - r.top, TRUE);
					setChanged(hwnd, TRUE);
				}
			}
			break;
		case IDC_WSTARTUP:
		case IDC_STARTMIN:
			setChanged(hwnd, HIWORD(wp)==BN_CLICKED);
			break;
		case IDC_LOG:
			{
				RECT rw, rc;
				GetWindowRect(hwnd, &rw);
				if( IsWindowVisible(GetDlgItem(hwnd, IDG_LOG)) )
				{
					GetWindowRect( GetDlgItem(hwnd, IDC_LOG), &rc);
					SetWindowPos(hwnd, NULL, 0,0, RCWIDTH(rw), rc.bottom+dlgUnits2Pix(hwnd, 6, FALSE) - rw.top, SWP_NOMOVE | SWP_NOZORDER);
					ShowWindow( GetDlgItem(hwnd, IDG_LOG), SW_HIDE);
					ShowWindow( GetDlgItem(hwnd, IDL_LOG), SW_HIDE);
				}
				else
				{
					GetWindowRect( GetDlgItem(hwnd, IDG_LOG), &rc);
					SetWindowPos(hwnd, NULL, 0,0, RCWIDTH(rw), rc.bottom+dlgUnits2Pix(hwnd, 6, FALSE) - rw.top, SWP_NOMOVE | SWP_NOZORDER);
					ShowWindow( GetDlgItem(hwnd, IDG_LOG), SW_SHOW);
					ShowWindow( GetDlgItem(hwnd, IDL_LOG), SW_SHOW);
				}
			}
			break;
		case IDOK:
			if( doApply(hwnd) )
				setChanged(hwnd, FALSE);
			break;
		case IDB_TEST:
//			rmvHook();
			Prompt_File_Name(0, hwnd, NULL, NULL);
//			setHook(hwnd);
//			propSheet(hwnd, "Testing", NULL, 3, IDD_WINSETTINGS, wpPageWinSettings, IDD_FOLDERS, wpPageFolders, IDD_PREFS, wpPagePrefs);
			break;
	}
}


int		addTrayIcon(HWND hwnd)
{
	HICON hIcon = (HICON)LoadImage(ghInstance, MAKEINTRESOURCE(IDI_TRAY), IMAGE_ICON, 16,16, LR_SHARED | LR_VGACOLOR);
	return Add_TrayIcon( hIcon, "OpenWide\r\nLeft-Click to show...", hwnd, WM_TRAYICON, 0);
}

void	remTrayIcon(HWND hwnd)
{
	Rem_TrayIcon( hwnd, WM_TRAYICON, 0 );
}


void doTrayMenu(HWND hwnd)
{
	HMENU hm = CreatePopupMenu();
	AppendMenu(hm, MF_STRING, 0, "Show options");
	SetMenuDefaultItem(hm, 0, TRUE);
	AppendMenu(hm, MF_GRAYED | MF_SEPARATOR, 0, NULL);
	AppendMenu(hm, MF_STRING, 0, "About OpenWide...");
	AppendMenu(hm, MF_STRING, 0, "Help...");
	AppendMenu(hm, MF_GRAYED | MF_SEPARATOR, 0, NULL);
	AppendMenu(hm, MF_STRING, 0, "Quit");

	POINT pt;
	GetCursorPos(&pt);
	TrackPopupMenu(hm, TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, 0, hwnd, NULL);

	DestroyMenu(hm);
}


BOOL WINAPI CALLBACK wpMain(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
	switch(msg)
	{
		case WM_INITDIALOG:
			if( !initWin(hwnd) )
				DestroyWindow(hwnd);
			else if( !setHook( hwnd ) )
				Warn( "Failed to set hook: %s", geterrmsg() );
			return FALSE;
		case WM_TRAYICON:
			switch(lp)
			{
				case WM_LBUTTONUP:
					remTrayIcon(hwnd);
					ShowWindow(hwnd, SW_SHOWNORMAL);
					EnableWindow(hwnd, TRUE);
					SetFocus(GetDlgItem(hwnd, IDB_TEST));
					break;
				case WM_RBUTTONUP:
					doTrayMenu(hwnd);
					break;
			}
			break;
		case WM_SYSCOMMAND:
			if( wp == SC_MINIMIZE )
			{
				EnableWindow(hwnd, FALSE);
				ShowWindow(hwnd, SW_HIDE);
				addTrayIcon(hwnd);
				EndTrayOperation();
			}
			else
				return DefWindowProc(hwnd, msg, wp, lp);
			break;
		case WM_COMMAND:
			onCommand(hwnd, wp, lp);
			break;
		case WM_CONTEXTMENU:
			SendDlgItemMessage(hwnd, 1000, LB_RESETCONTENT, 0,0);
			break;
		case WM_DESTROY:
			rmvHook();
			releaseSharedMem();
			PostQuitMessage(0);
			break;
		case WM_CLOSE:
			DestroyWindow(hwnd);
			break;
	}
	return 0;
}


int createWin(void)
{

	WNDCLASSEX wc = {0};
	wc.cbSize = sizeof(wc);

	if(!GetClassInfoEx(NULL, WC_DIALOG, &wc))
		return 0;
	wc.hIconSm = (HICON)LoadImage(ghInstance, MAKEINTRESOURCE(IDI_APP), IMAGE_ICON, 16,16, LR_SHARED | LR_VGACOLOR);
	wc.hIcon = (HICON)LoadImage(ghInstance, MAKEINTRESOURCE(IDI_APP), IMAGE_ICON, 32,32, LR_SHARED | LR_VGACOLOR);
	RegisterClassEx(&wc);
		//return 0;

	ghwMain = CreateDialog(ghInstance, MAKEINTRESOURCE(IDD_MAIN), NULL, wpMain);
	if( !ghwMain )
		Warn("Unable to create main dialog: %s", geterrmsg());
	return (ghwMain!=NULL);
}

int initApp(void)
{
	HRESULT hRes = CoInitialize(NULL);
	if( hRes != S_OK && hRes != S_FALSE )
		return 0;
	if( !createWin() )
		return 0;
	return 1;
}


void shutdownApp(void)
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
		HWND hw = FindWindowEx(NULL, NULL, WC_DIALOG, "Open Wide!");
		if(hw)
		{
			SendMessage(hw, WM_TRAYICON, 0, WM_LBUTTONUP);
			FlashWindow(hw, TRUE);
			SetForegroundWindow(hw);
		}
		return 0;
	}
	ghInstance = hi;
	if( !initApp() )
	{
		shutdownApp();
		return 0;
	}

	MSG msg;
	int ret;

	while( (ret = GetMessage(&msg, NULL, 0,0) ) > 0 )
	{
		if( !IsDialogMessage(ghwMain, &msg) )
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	shutdownApp();
	return 0;
}


