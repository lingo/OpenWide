#define	COBJMACROS
#include	<windows.h>
#include	<shlwapi.h>
#include	<objbase.h>
#include	<shobjidl.h>
#include	<shlguid.h>
#include	<shlobj.h>
#include	<shellapi.h>
#include	<stdio.h>
#include	"openwidedll.h"
#include	"openwide.h"


int		setStartupApp(HWND hwnd, BOOL	bSet);

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
	FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM |
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
		dbg("Address of OFN data is %p, structsize is %lu", &ofn, ofn.lStructSize);
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
	gPowData->bStartMin = IsDlgButtonChecked(hwnd, IDC_STARTMIN) == BST_CHECKED;
	BOOL bWinStart = IsDlgButtonChecked(hwnd, IDC_WSTARTUP) == BST_CHECKED;

	HKEY	hk = regCreateKey(HKEY_CURRENT_USER,  OW_REGKEY_NAME);
	if( hk )
	{
		regWriteDWORD(hk, NULL, sizeof(OWSharedData));
		regWriteBinaryData(hk, "OWData", gPowData, sizeof(OWSharedData));
		regCloseKey(hk);
	}

	if( ghMutex )
		ReleaseMutex(ghMutex);

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

// CreateLink - uses the Shell's IShellLink and IPersistFile interfaces
//              to create and store a shortcut to the specified object.
//
// Returns the result of calling the member functions of the interfaces.
//
// Parameters:
// lpszPathObj  - address of a buffer containing the path of the object.
// lpszPathLink - address of a buffer containing the path where the
//                Shell link is to be stored.
// lpszDesc     - address of a buffer containing the description of the
//                Shell link.

HRESULT CreateLink(LPCSTR lpszPathObj, LPCSTR lpszPathLink, LPCSTR lpszDesc)
{
    HRESULT hres;
    IShellLink* psl;

    // Get a pointer to the IShellLink interface.
    hres = CoCreateInstance(&CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER,
                            &IID_IShellLink, (LPVOID*)&psl);
    if (SUCCEEDED(hres))
    {
        IPersistFile* ppf;

        // Set the path to the shortcut target and add the description.
        psl->SetPath(lpszPathObj);
        psl->SetDescription(lpszDesc);

        // Query IShellLink for the IPersistFile interface for saving the
        // shortcut in persistent storage.
        hres = psl->QueryInterface(&IID_IPersistFile, (LPVOID*)&ppf);

        if (SUCCEEDED(hres))
        {
            WCHAR wsz[MAX_PATH];

            // Ensure that the string is Unicode.
            MultiByteToWideChar(CP_ACP, 0, lpszPathLink, -1, wsz, MAX_PATH);

            // TODO: Check return value from MultiByteWideChar to ensure success.
            // Save the link by calling IPersistFile::Save.
            hres = ppf->Save(wsz, TRUE);
            ppf->Release();
        }
        psl->Release();
    }
    return hres;
}



/* Copied by pro2 : (c)2004 Luke Hudson */
/** Function source : C:\Data\Code\C\Proto\util.c */
BOOL fileExists (const char *path)
{
	int errCode;

	if (GetFileAttributes (path) == 0xFFFFFFFF)
	{
		errCode = GetLastError ();
		if (errCode == ERROR_FILE_NOT_FOUND
				|| errCode == ERROR_PATH_NOT_FOUND)
			return FALSE;
		dbg ("fileExists? getLastError gives %d", errCode);
	}
	return TRUE;
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

				if( DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_PLACEMENT), hwnd, wpPlacement, (LPARAM)&r) )
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
			break;
	}
}


int		addTrayIcon(HWND hwnd)
{
	HICON hIcon = (HICON)LoadImage(hInst, MAKEINTRESOURCE(IDI_TRAY), IMAGE_ICON, 16,16, LR_SHARED | LR_VGACOLOR);
	return Add_TrayIcon( hIcon, "OpenWide\r\nLeft-Click to show...", hwnd, WM_TRAYICON, 0);
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
			return FALSE;
		case WM_TRAYICON:
			switch(lp)
			{
				case WM_LBUTTONUP:
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
	HRESULT hRes = CoInitialize(NULL);
	if( hRes != S_OK && hRes != S_FALSE )
		return 0;
	if( !createWin() )
		return 0;
	return 1;
}


void shutdown(void)
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
	hInst = hi;
	if( !init() )
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


