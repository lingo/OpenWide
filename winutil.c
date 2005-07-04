#define	COBJMACROS

#include	<windows.h>
#include	<objbase.h>
#include	<shobjidl.h>
#include	<shlguid.h>
#include	<shlobj.h>
#include	<shellapi.h>
#include	<shlwapi.h>
#include	<stdio.h>
#include	"winutil.h"

#pragma lib "uuid.lib"
#pragma lib "ole32.lib"

extern HINSTANCE	ghInstance;

static char gszPath[MAX_PATH+1];


const char * getLocalPath(const char *szAppend)
{
	if( GetModuleFileName(NULL, gszPath, MAX_PATH) != 0 )
	{
		char * szFn = strrchr(gszPath, '\\');
		if( szFn )
			*szFn = 0;
		PathAppend(gszPath, szAppend);
		return gszPath;
	}
	return szAppend;
}

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
		//dbg("%s, Address of OFN data is %p, structsize is %lu", __func__, &ofn, ofn.lStructSize);
		if(!GetOpenFileName(&ofn))
			return NULL;
		return szFileName;
	}
	/*	MSG msg;
	while (PeekMessage(&msg, hwOwner, WM_MOUSEFIRST, WM_MOUSELAST, PM_REMOVE));*/
}


/* Copied on : Fri May 13 18:45:35 2005
 */
/** Function source : C:\Data\Code\C\Proto\registry.c */
HKEY regOpenKey(HKEY hkParent, const char *szSubKey){
	LONG res;
	HKEY hk;

	if(!szSubKey) return NULL;
	res = RegOpenKeyEx(hkParent, szSubKey, 0L, KEY_READ | KEY_WRITE, &hk);
	return ( res == ERROR_SUCCESS ) ? hk : NULL;
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
int regWriteBinaryData(HKEY hkRoot, const char *szValue, void *buf, int bufSize ){
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

/* Copied on : Fri May 13 17:30:02 2005
 */
/** Function source : C:\Data\Code\C\Proto\registry.c */
char *regReadSZ(HKEY hk, const char *szValueName){
	LONG res;
	DWORD dwType, dwSize = 0L;

	res = RegQueryValueEx(hk, szValueName, NULL, &dwType, NULL, &dwSize);
	if( res == ERROR_SUCCESS )
	{
		if(dwSize > 1 &&	(dwType == REG_SZ || dwType == REG_EXPAND_SZ) )
		{
			char * szData = malloc(dwSize);
			res = RegQueryValueEx(hk, szValueName, NULL, &dwType, (BYTE*)szData, &dwSize);
			if(res == ERROR_SUCCESS)
				return szData;
		}
	}
	return NULL;
}



/* Copied on : Fri May 13 17:28:47 2005  */
/** Function source : C:\Data\Code\C\Proto\registry.c */
int regWriteSZ(HKEY hkRoot, const char *szValueName, const char *szData){
	LONG res;
	if(!szData)
		szData = "";
		//return 1;
	res = RegSetValueEx(hkRoot, (LPCTSTR)szValueName, 0L, REG_SZ,
						(BYTE *)szData,
						strlen(szData) + 1);
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


/* Copied by pro2 : (c)2004 Luke Hudson */
/** Function source : C:\Data\Code\C\Proto\prefsdlg.c */
static void fillPSheetPage(PROPSHEETPAGE * psp, int idDlg, DLGPROC pfnDlgProc)
{
	memset(psp, 0, sizeof(PROPSHEETPAGE));
	psp->dwSize = sizeof(PROPSHEETPAGE);
	psp->dwFlags = PSP_DEFAULT;	// | PSP_USEICONID;
	psp->hInstance = ghInstance;
	psp->pszTemplate = MAKEINTRESOURCE(idDlg);
	psp->pszIcon = (LPSTR)NULL;// MAKEINTRESOURCE(IDI_DOC);
	psp->pfnDlgProc = pfnDlgProc;
	psp->lParam = 0;
}


/** Creates a property sheet dialog from the arguments:
 *	propSheet(hwParent, "Prefs", pfInit, 3, IDD_PAGE1, wpPage1, ...);
 */
BOOL	propSheet(HWND hwParent, const char *szTitle, PFNPROPSHEETCALLBACK pfInitFunc, int nPages, ...)
{
	va_list		vl;
	va_start(vl, nPages);

	PROPSHEETHEADER psh;
	PROPSHEETPAGE *pages = NULL;

	if(nPages < 1)
		return FALSE;

	pages = malloc(nPages * sizeof(PROPSHEETPAGE));
	if(!pages)
		return FALSE;

	for(int i=0; i < nPages; i++)
	{
		int dlgID = va_arg(vl, int);
		DLGPROC wpDlg = va_arg(vl, DLGPROC);
		dbg("Page %d: Dlg id %d, proc %p", i, dlgID, wpDlg);
		fillPSheetPage(&pages[i], dlgID, wpDlg);
	}

	va_end(vl);

	memset(&psh, 0, sizeof(PROPSHEETHEADER));
	psh.dwSize = sizeof(PROPSHEETHEADER);
	psh.dwFlags = PSH_PROPSHEETPAGE | (( pfInitFunc != NULL) ? PSH_USECALLBACK : 0 );	// | PSH_USEHICON
																		// PSH_NOAPPLYNOW
																		// //
																		// |
																		// PSH_USECALLBACK;
	psh.hwndParent = hwParent;
	psh.hInstance = ghInstance;
	psh.pszIcon = (LPSTR)NULL;// MAKEINTRESOURCE(IDI_APP);	// LoadImage(hInst, ,
													// IMAGE_ICON, 16,16,
													// LR_SHARED);
	psh.pszCaption = (LPSTR) szTitle;
	psh.nPages = nPages;
	psh.ppsp = (LPCPROPSHEETPAGE) pages;
	psh.pfnCallback = pfInitFunc;
	psh.nStartPage = 0;

	int iRet = PropertySheet(&psh);
	free(pages);
	return iRet != -1;
}


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
        psl->lpVtbl->SetPath(psl, lpszPathObj);
        psl->lpVtbl->SetDescription(psl, lpszDesc);

        // Query IShellLink for the IPersistFile interface for saving the
        // shortcut in persistent storage.
        hres = psl->lpVtbl->QueryInterface(psl, &IID_IPersistFile, (LPVOID*)&ppf);

        if (SUCCEEDED(hres))
        {
            WCHAR wsz[MAX_PATH];

            // Ensure that the string is Unicode.
            MultiByteToWideChar(CP_ACP, 0, lpszPathLink, -1, wsz, MAX_PATH);

            // TODO: Check return value from MultiByteWideChar to ensure success.
            // Save the link by calling IPersistFile::Save.
            hres = ppf->lpVtbl->Save(ppf, wsz, TRUE);
            ppf->lpVtbl->Release(ppf);
        }
        psl->lpVtbl->Release(psl);
    }
    return hres;
}
