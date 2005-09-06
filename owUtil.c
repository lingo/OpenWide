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
//		dbg("Address of OFN data is %p, structsize is %lu", &ofn, ofn.lStructSize);
		if(!GetOpenFileName(&ofn))
			return NULL;
		return szFileName;
	}
	/*	MSG msg;
	while (PeekMessage(&msg, hwOwner, WM_MOUSEFIRST, WM_MOUSELAST, PM_REMOVE));*/
}


/* Copied on : Tue Jul 12 18:03:16 2005 */
/** Function source : C:\Data\Code\C\resh\listview.c */
TCHAR *lbGetItemText(HWND hwLB, int iItem)
{
	TCHAR *szText = NULL;
	int len;
	len = SendMessage(hwLB, LB_GETTEXTLEN, iItem, 0) + 1;
	if(len <= 1)
		return NULL;
	szText = malloc(len+1 * sizeof(char));
	if(!szText) return NULL;
	len = SendMessage(hwLB, LB_GETTEXT, iItem, (LPARAM)szText);
	if( len <= 0 )
		free(szText);
	return szText;
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


BOOL	delFile(HWND hwnd, const char *szFile)
{
	SHFILEOPSTRUCT shlOp = {0};
	shlOp.hwnd = hwnd;
	shlOp.wFunc = FO_DELETE;
	shlOp.pFrom = szFile;
	shlOp.fFlags = FOF_ALLOWUNDO | FOF_NOCONFIRMATION | FOF_SILENT | FOF_NOERRORUI | FOF_NO_CONNECTED_ELEMENTS | FOF_NORECURSION;
	return (SHFileOperation(&shlOp) == S_OK);
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
		//dbg ("fileExists? getLastError gives %d", errCode);
	}
	return TRUE;
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

/* Copied on : Tue Jul 12 02:29:56 2005 */
/** Function source : C:\Data\Code\C\openwide\psDlg.c */
BOOL	waitForMutex(void)
{
	DWORD dwRes;
	if( !ghMutex )
	{
		ghMutex = OpenMutex(SYNCHRONIZE, FALSE, OW_MUTEX_NAME);
	}
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
				//dbg("Mutex wait failed");
				return FALSE;
		}
	}
	return TRUE;
}

void	releaseMutex(void)
{
	if( ghMutex )
	{	
		ReleaseMutex(ghMutex);
	}
}

