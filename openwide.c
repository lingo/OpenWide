#include	<windows.h>
#include	<shellapi.h>
#include	<stdio.h>
#include	"openwidedll.h"
#include	"openwide.h"

int setHook(HWND hwLB);
int rmvHook(void);

#define	OW_REGKEY_NAME	("Software\\HiveMind\\OpenWide")

#define		RCWIDTH(r)	((r).right - (r).left + 1)
#define		RCHEIGHT(r)	((r).bottom - (r).top + 1)

#define		WM_TRAYICON		(WM_APP)

HWND		ghwMain = NULL;
HINSTANCE	hInst = NULL;
HWND		ghwListBox = NULL;

static POWSharedData 	gPowData = NULL;
static HANDLE			ghSharedMem =  NULL;
static HANDLE			ghMutex = NULL;
static BOOL				gbChanged = FALSE;

int		dlgUnits2Pix(HWND hwnd, int units, BOOL bHorz)
{
	RECT r;
	SetRect(&r, 0,0,units,units);
	MapDialogRect(hwnd, &r);
	return (bHorz) ? r.right : r.bottom;
}

int		pix2DlgUnits(HWND hwnd, int pix, BOOL bHorz)
{
	RECT r;
	SetRect(&r, 0,0,10,10);
	MapDialogRect(hwnd, &r);
	return (bHorz) ? (pix / r.right) : (pix / r.bottom);
}

const char * geterrmsg(void)
{
	static char szMsg[256];
	FormatMessage(
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		GetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
		(LPTSTR) szMsg,
		255,
		NULL
	);
	int len = strlen(szMsg) - 1;
	if(len < 0) return NULL;
	while( szMsg[len] == '\r' || szMsg[len] == '\n'){
		szMsg[len--] = 0;
		if(len < 0) break;
	}
	return szMsg;
}

/* Copied from Pro2
 * Function source : C:\Code\lcc\Proto\util.c */
void Error(char *szError, ...)
{
	char		szBuff[256];
	va_list		vl;
	va_start(vl, szError);
    _vsnprintf(szBuff, 256, szError, vl);	// print error message to string
	OutputDebugString(szBuff);
	MessageBox(NULL, szBuff, "Error", MB_OK); // show message
	va_end(vl);
	exit(-1);
}

/* Copied from Pro2
 * Function source : C:\Code\lcc\Proto\util.c */
void Warn(char *szError, ...)
{
	char		szBuff[256];
	va_list		vl;
	va_start(vl, szError);
    _vsnprintf(szBuff, 256, szError, vl);	// print error message to string
	OutputDebugString(szBuff);
	MessageBox(NULL, szBuff, "Error", MB_OK); // show message
	va_end(vl);
}

void dbg(char *szError, ...)
{
	char		szBuff[256];
	va_list		vl;
	va_start(vl, szError);
    _vsnprintf(szBuff, 256, szError, vl);	// print error message to string
	OutputDebugString(szBuff);
	va_end(vl);
}

UINT APIENTRY OFNHookProcOldStyle(
    HWND hdlg,	// handle to the dialog box window
    UINT uiMsg,	// message identifier
    WPARAM wParam,	// message parameter
    LPARAM lParam 	// message parameter
)
{
	return FALSE;
}


/* Copied by pro2 : (c)2004 Luke Hudson */
/** Function source : C:\Data\Code\C\resh\resh.c */
char *Prompt_File_Name(int iFlags, HWND hwOwner, const char *pszFilter, const char *pszTitle)
{
	static char szFileName[MAX_PATH];			// store selected file
	char szDefFilter[] = "All Files\0*.*\0\0";	// default filter
	OPENFILENAME ofn;

	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);

	szFileName[0] = 0; // mark the file name buffer as empty;

	ofn.hwndOwner = hwOwner;
	// If a filter is supplied, use it.  Otherwise, use default.
	ofn.lpstrFilter = (pszFilter) ? pszFilter : szDefFilter;
	ofn.lpstrFile = szFileName;		// where to store filename
	ofn.nMaxFile = MAX_PATH;		// length of filename buffer
	ofn.lpstrTitle = pszTitle;		// title if supplied

	if(iFlags == 1)
	{
		ofn.Flags = OFN_EXPLORER	// use new features
					| OFN_CREATEPROMPT	// create files if user wishes
					| OFN_OVERWRITEPROMPT
					| OFN_PATHMUSTEXIST // what it says
					| OFN_HIDEREADONLY; // hide the read-only option
		if(pszFilter)
		{
			int i;
			for(i=0; pszFilter[i]!=0; i++)
				;
			i++;
			char *sp= strchr(&pszFilter[i], '.');
			if(sp)	ofn.lpstrDefExt = sp+1;
		}
		if(!GetSaveFileName(&ofn))
			return NULL;
		return szFileName;
	}
	else
	{
//		ofn.lpfnHook = OFNHookProcOldStyle;
//		ofn.Flags = OFN_ENABLEHOOK;
		ofn.Flags = OFN_EXPLORER	// use new features
					| OFN_FILEMUSTEXIST	// file names must be valid
					| OFN_PATHMUSTEXIST // and paths too.
					| OFN_HIDEREADONLY; // hide the read-only option
		if(!GetOpenFileName(&ofn))
			return NULL;
		return szFileName;
	}
	/*	MSG msg;
	while (PeekMessage(&msg, hwOwner, WM_MOUSEFIRST, WM_MOUSELAST, PM_REMOVE));*/
}



/* Copied by pro2 : (c)2004 Luke Hudson */
/** Function source : C:\Data\Code\C\Proto\registry.c */
void regCloseKey(HKEY hk)
{
	RegCloseKey(hk);
}

/** Function source : C:\Data\Code\C\Proto\registry.c */
HKEY regCreateKey(HKEY hkParent, const char *szSubKey){
	LONG res;
	HKEY hk;
	res = RegCreateKeyEx(hkParent, szSubKey, 0L, NULL, REG_OPTION_NON_VOLATILE,
							KEY_READ | KEY_WRITE, NULL, &hk, NULL);
	return ( res == ERROR_SUCCESS ) ? hk : NULL;
}

/** Function source : C:\Data\Code\C\Proto\registry.c */
BYTE *regReadBinaryData(HKEY hkRoot, const char *szValueName){
	DWORD dwType, dwSize = 0;
	LONG res;
	res = RegQueryValueEx(hkRoot, szValueName, NULL, &dwType, NULL, &dwSize);
	if( res == ERROR_SUCCESS	&&	dwType == REG_BINARY	&&	dwSize > 0)
	{
		BYTE * buf = malloc(dwSize);
		if(!buf)
			return NULL;
		res = RegQueryValueEx(hkRoot, szValueName, NULL, NULL, buf, &dwSize);
		if( res == ERROR_SUCCESS )
			return buf;
	}
	return NULL;
}

/** Function source : C:\Data\Code\C\Proto\registry.c */
int regWriteBinaryData(HKEY hkRoot, const char *szValue, BYTE *buf, int bufSize ){
	LONG res;
	res = RegSetValueEx(hkRoot, szValue, 0L, REG_BINARY, buf, bufSize);
	return (res == ERROR_SUCCESS);
}

/* Copied by pro2 : (c)2004 Luke Hudson */
/** Function source : C:\Data\Code\C\Proto\registry.c */
DWORD regReadDWORD(HKEY hkRoot, const char *szValueName, int *pSuccess)
{
	LONG res;
	DWORD dwType, dwSize = 0;
	DWORD dword = 0L;

	if( pSuccess )
		*pSuccess = 0;
	res = RegQueryValueEx(hkRoot, szValueName, NULL, &dwType, NULL, &dwSize);
	if( res == ERROR_SUCCESS	&& (dwType == REG_DWORD || REG_DWORD_LITTLE_ENDIAN)		&& dwSize == sizeof(DWORD) )
	{
		res = RegQueryValueEx(hkRoot, szValueName, NULL, NULL, (BYTE*)&dword, &dwSize);
		if( res == ERROR_SUCCESS && pSuccess)
			*pSuccess = 1;
	}
	return dword;
}



/* Copied by pro2 : (c)2004 Luke Hudson */
/** Function source : C:\Data\Code\C\Proto\registry.c */
int regWriteDWORD(HKEY hkRoot, const char *szValue, DWORD dwData)
{
	LONG res;
	res = RegSetValueEx(hkRoot, (LPCTSTR)szValue, 0L, REG_DWORD,
						(BYTE *)&dwData,
						sizeof(dwData));
	return (res == ERROR_SUCCESS);
}



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
			}
			gPowData->hwLog = hwnd;
			rVal = 1;
		}
	}

	if( pRData )
		free(pRData);

	ReleaseMutex(ghMutex);
	return rVal;
}



void releaseSharedMem(void)
{
	dbg("Waiting for mutex");
	if( ghMutex )
	{
		DWORD dwRes = WaitForSingleObject(ghMutex, INFINITE);
		if(dwRes != WAIT_OBJECT_0)
		{
			dbg("Failed to get ownership of mutex in releaseSharedMem!!!");
			return;
		}
	}

	dbg("Releasing file mapping");
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

	HKEY	hk = regCreateKey(HKEY_CURRENT_USER,  OW_REGKEY_NAME);
	if( hk )
	{
		regWriteDWORD(hk, NULL, sizeof(OWSharedData));
		regWriteBinaryData(hk, "OWData", gPowData, sizeof(OWSharedData));
		regCloseKey(hk);
	}

	if( ghMutex )
		ReleaseMutex(ghMutex);
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

	CheckDlgButton(hwnd, IDC_LOG, BST_CHECKED);
	fillViewCB(hwnd, IDCB_VIEW);
	fillFocusCB(hwnd, IDCB_FOCUS);

	if(ghMutex)
	{
		DWORD dwRes = WaitForSingleObject(ghMutex, 1000);
		if(dwRes != WAIT_OBJECT_0)
			return 0;
	}

	SetDlgItemInt(hwnd, IDE_LEFT, gPowData->ptOrg.x, FALSE);
	SetDlgItemInt(hwnd, IDE_TOP, gPowData->ptOrg.y, FALSE);
	SetDlgItemInt(hwnd, IDE_WIDTH, gPowData->szDim.cx, FALSE);
	SetDlgItemInt(hwnd, IDE_HEIGHT, gPowData->szDim.cy, FALSE);
	SendDlgItemMessage(hwnd, IDCB_VIEW, CB_SETCURSEL, gPowData->iView, 0);
	SendDlgItemMessage(hwnd, IDCB_FOCUS, CB_SETCURSEL, gPowData->iFocus, 0);

	if( ghMutex )
		ReleaseMutex(ghMutex);
	EnableWindow( GetDlgItem(hwnd, IDOK), FALSE);

	RECT rc,rw;
	GetWindowRect( GetDlgItem(hwnd, IDC_LOG), &rc);
	GetWindowRect( hwnd, &rw);
	SetWindowPos(hwnd, NULL, 0,0, RCWIDTH(rw), rc.top+dlgUnits2Pix(hwnd, 6, FALSE) - rw.top, SWP_NOMOVE | SWP_NOZORDER);
//	ShowWindow( GetDlgItem(hwnd, IDG_LOG), SW_HIDE);
//	ShowWindow( GetDlgItem(hwnd, IDL_LOG), SW_HIDE);

	return 1;
}

BOOL WINAPI CALLBACK wpPlacement(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
	static LPRECT pr = NULL;
	switch(msg)
	{
		case WM_INITDIALOG:
			pr = (LPRECT)lp;
			if(!pr)
				EndDialog(hwnd, 0);
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
				pmm->ptMinTrackSize.x = 565;
				pmm->ptMinTrackSize.y = 349;
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

int	getDlgItemRect(HWND hwnd, UINT uID, LPRECT pr)
{
	if(!IsWindow(hwnd) || !pr)
		return 0;
	HWND hwCtl = GetDlgItem(hwnd, uID);
	if(!hwCtl)
		return 0;
	if(!GetWindowRect(hwCtl, pr))
		return 0;
	MapWindowPoints(NULL, hwnd, (LPPOINT)&pr->left, 2);
	return 1;
}

static void onCommand(HWND hwnd, WPARAM wp, LPARAM lp)
{
	switch(LOWORD(wp))
	{
		case IDCB_VIEW:
		case IDCB_FOCUS:
			if( HIWORD(wp) == CBN_SELCHANGE )
			{
				gbChanged = TRUE;
				EnableWindow( GetDlgItem(hwnd, IDOK), TRUE);
			}
			break;
		case IDE_LEFT:
		case IDE_TOP:
		case IDE_WIDTH:
		case IDE_HEIGHT:
			if( HIWORD(wp) == EN_CHANGE )
			{
				gbChanged = TRUE;
				EnableWindow( GetDlgItem(hwnd, IDOK), TRUE);
			}
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

				if( DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_PLACEMENT), hwnd, wpPlacement, (LPARAM)&r) )
				{
					SetDlgItemInt(hwnd, IDE_LEFT, r.left, TRUE);
					SetDlgItemInt(hwnd, IDE_TOP, r.top, TRUE);
					SetDlgItemInt(hwnd, IDE_WIDTH, r.right - r.left, TRUE);
					SetDlgItemInt(hwnd, IDE_HEIGHT, r.bottom - r.top, TRUE);
					gbChanged = TRUE;
					EnableWindow( GetDlgItem(hwnd, IDOK), TRUE);
				}
			}
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
			{
				gbChanged = FALSE;
				EnableWindow( GetDlgItem(hwnd, IDOK), FALSE);
			}
			break;
		case IDB_TEST:
//			rmvHook();
			Prompt_File_Name(0, hwnd, NULL, NULL);
//			setHook(hwnd);
			break;
	}
}

/** Function source : C:\Data\Code\C\ExtassE\util.c */
static int AddRem_TrayIcon(HICON hIcon, char *szTip, HWND hwnd, UINT uMsg, DWORD dwState, DWORD dwMode){

	NOTIFYICONDATA nid;

	memset(&nid, 0, sizeof(NOTIFYICONDATA));
	nid.cbSize = sizeof(NOTIFYICONDATA);
	nid.hWnd = hwnd;
	nid.uFlags =  NIF_MESSAGE | (hIcon ? NIF_ICON : 0) | (szTip ? NIF_TIP : 0);
	nid.hIcon = hIcon;
	nid.uCallbackMessage = uMsg;

	if( szTip )
	{
		strncpy(nid.szTip, szTip, 63);
		nid.szTip[63] = 0;
	}

	//nid.dwState = dwState;
	return (Shell_NotifyIcon(dwMode, &nid) != 0);
}

/* Copied by pro2 : (c)2004 Luke Hudson */
/** Function source : C:\Data\Code\C\ExtassE\util.c */
int Add_TrayIcon(HICON hIcon, char *szTip, HWND hwnd, UINT uMsg, DWORD dwState){
	return AddRem_TrayIcon(hIcon, szTip, hwnd, uMsg, dwState, NIM_ADD);
}

/** Function source : C:\Data\Code\C\ExtassE\util.c */
void EndTrayOperation(void)
{
	NOTIFYICONDATA nid = {0};
	nid.cbSize = sizeof(NOTIFYICONDATA);
	Shell_NotifyIcon(NIM_SETFOCUS, &nid);
}

/** Function source : C:\Data\Code\C\ExtassE\util.c */
int Rem_TrayIcon(HWND hwnd, UINT uMsg, DWORD dwState){
	return AddRem_TrayIcon(NULL, NULL, hwnd, uMsg, dwState, NIM_DELETE);
}



int		addTrayIcon(HWND hwnd)
{
	HICON hIcon = (HICON)LoadImage(hInst, MAKEINTRESOURCE(IDI_TRAY), IMAGE_ICON, 16,16, LR_SHARED | LR_VGACOLOR);
	return Add_TrayIcon( hIcon, "OpenWide\r\nDouble-click to show...", hwnd, WM_TRAYICON, 0);
}

void	remTrayIcon(HWND hwnd)
{
	Rem_TrayIcon( hwnd, WM_TRAYICON, 0 );
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
			break;
		case WM_TRAYICON:
			switch(lp)
			{
				case WM_LBUTTONDBLCLK:
					ShowWindow(hwnd, SW_SHOWNORMAL);
					EnableWindow(hwnd, TRUE);
					remTrayIcon(hwnd);
					SetFocus(hwnd);
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
	wc.hIconSm = (HICON)LoadImage(hInst, MAKEINTRESOURCE(IDI_TRAY), IMAGE_ICON, 16,16, LR_SHARED | LR_VGACOLOR);
	if( !RegisterClassEx(&wc) )
		return 0;

	ghwMain = CreateDialog(hInst, MAKEINTRESOURCE(IDD_MAIN), NULL, wpMain);
	if( !ghwMain )
		Warn("Unable to create main dialog: %s", geterrmsg());
	return (ghwMain!=NULL);
}

int init(void)
{
	if( !createWin() ) return 0;
	return 1;
}


void shutdown(void)
{
	if(ghwMain)
	{
		DestroyWindow(ghwMain);
		ghwMain = NULL;
	}
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
			ShowWindow(hw, SW_SHOWNORMAL);
			SetForegroundWindow(hw);
		}
		return 0;
	}
	hInst = hi;
	if(!init())
	{
		shutdown();
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

	shutdown();
	return 0;
}

