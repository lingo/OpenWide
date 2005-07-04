/* --- The following code comes from c:\lcc\lib\wizard\dll.tpl. */
//#include  <stdarg.h>
//#include  <stdio.h>
#include <windows.h>
#include <commctrl.h>
#include	<shlwapi.h>
#include <stdlib.h>
#include	<shellapi.h>
#include	<dlgs.h>
#include	<excpt.h>
#include	"openwidedll.h"
#include	"openwide.h"
#include	"owdll.h"
#include	"winutil.h"
//#include  "link.h"
//#include  "dbgwm.h"

////////////////////////////////////////////////////////////////////////////////
//  Defines

#define	OW_LISTVIEW_STYLE	(LVS_EX_FULLROWSELECT)


////////////////////////////////////////////////////////////////////////////////
//  Types

typedef struct OWSubClassData {
	WNDPROC wpOrig;
	LPARAM lpData;
	BOOL bSet;
} OWSubClassData, *POWSubClassData;


typedef struct FindChildData {
	const char *szClass;
	UINT uID;
	HWND hwFound;
} FindChildData, *PFindChildData;

////////////////////////////////////////////////////////////////////////////////
//  Globals

//static BOOL   gbDbg = FALSE;

HINSTANCE ghInstance = NULL;
static HHOOK ghMsgHook = NULL, ghSysMsgHook = NULL;
static BOOL gbHooked = FALSE;
static HANDLE ghMutex = NULL;
static HANDLE ghMap = NULL;
static OWSharedData gOwShared;


////////////////////////////////////////////////////////////////////////////////
//  Prototypes
/* Copied on : Tue May 10 23:20:15 2005 */

static int addTBButton(HWND hwnd);
static LRESULT CALLBACK CBTProc(int nCode, WPARAM wParam, LPARAM lParam);
static int doAddFave(HWND hwnd);
static void dropMenu(HWND hwnd, HWND hwTB, UINT uiItem);
static int CALLBACK WINAPI enumFindChildWindow(HWND hwnd, PFindChildData pData);
static HWND findChildWindow(HWND hwParent, UINT uID, const char *szClass);
static int focusDlgItem(HWND hwnd, int iFocus);
static WORD focusToCtlID(int iFocus);
static void getFavourite(HWND hwnd, int idx);
static int getSharedData(void);
static int initSharedMem(void);
static BOOL isOpenSaveDlg(HWND hwNew, LPCREATESTRUCT pcs);
static int openWide(HWND hwnd);
static void releaseSharedMem(void);
static void setCDMPath(HWND hwnd, const char *szPath);
static int subclass(HWND hwnd, WNDPROC wpNew, LPARAM lpData);
static LRESULT CALLBACK SysMsgProc(int nCode, WPARAM wParam, LPARAM lParam);
static int unsubclass(HWND hwnd);
static WORD viewToCmdID(int iView);
static LRESULT CALLBACK WINAPI wpSubMain(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);

////////////////////////////////////////////////////////////////////////////////
//  Functions


static int subclass(HWND hwnd, WNDPROC wpNew, LPARAM lpData)
{
	if (GetProp(hwnd, OW_PROP_NAME) != NULL)
		return 0;
	POWSubClassData pow = (POWSubClassData)GlobalAlloc(GPTR, sizeof(OWSubClassData));	// was malloc
	if (!pow)
		return 0;
	ZeroMemory(pow, sizeof(OWSubClassData));
	if (!SetProp(hwnd, OW_PROP_NAME, pow))
	{
		GlobalFree(pow);
		//free(pow);
		return 0;
	}
	pow->lpData = lpData;
	pow->wpOrig = (WNDPROC)SetWindowLong(hwnd, GWL_WNDPROC, (LONG)wpNew);
	return 1;
}

static int unsubclass(HWND hwnd)
{
	POWSubClassData pow = (POWSubClassData)GetProp(hwnd, OW_PROP_NAME);
	if (pow)
	{
		SetWindowLong(hwnd, GWL_WNDPROC, (LONG)pow->wpOrig);
		RemoveProp(hwnd, OW_PROP_NAME);
		GlobalFree(pow);
		//free(pow);
		return 1;
	}
	return 0;
}


static int CALLBACK WINAPI enumFindChildWindow(HWND hwnd, PFindChildData pData)
{
	UINT uID = GetDlgCtrlID(hwnd);
	static char buf[256];
	if (uID == pData->uID)
	{
		//dbg("Found window %p with id %d", hwnd, uID);
		if (GetClassName(hwnd, buf, 256) && strcmp(buf, pData->szClass) == 0)
		{
			pData->hwFound = hwnd;
			return 0;
		}
		//else
		//dbg("Window's class is %s - not what's wanted", buf);
	}
	return 1;
}

static HWND findChildWindow(HWND hwParent, UINT uID, const char *szClass)
{
	FindChildData fData;

	fData.szClass = szClass;
	fData.uID = uID;
	fData.hwFound = NULL;

	//dbg("findChildWindow: seeking \"%s\", %d", szClass, uID);

	EnumChildWindows(hwParent, (WNDENUMPROC)enumFindChildWindow, (LPARAM) & fData);

	return fData.hwFound;
}

static void dropMenu(HWND hwnd, HWND hwTB, UINT uiItem)
{
	RECT r;
	SendMessage(hwTB, TB_GETRECT, uiItem, (LPARAM) & r);
	MapWindowPoints(hwTB, NULL, (LPPOINT) & r, 2);

	TPMPARAMS tpm = { 0 };
	tpm.cbSize = sizeof(tpm);
	tpm.rcExclude = r;

	HMENU hm = CreatePopupMenu();
	AppendMenu(hm, MF_STRING, OW_EXPLORE_CMDID, "&Locate current folder with Explorer...");
	AppendMenu(hm, MF_STRING, OW_ADDFAV_CMDID, "Add &Favourite");
	SetMenuDefaultItem(hm, OW_ADDFAV_CMDID, FALSE);

	POWSharedData pow = lockSharedData();
	if (pow)
	{
		dbg("%s, Locked shared data ok", __func__);
		PFavLink pFav = pow->pFaves;
		if (pFav)
		{
			MENUITEMINFO mii = { 0 };
			mii.cbSize = sizeof(mii);
			AppendMenu(hm, MF_SEPARATOR | MF_DISABLED, 0, NULL);
			for (int i = 0; i < pow->nFaves; i++)
			{
				pFav = getNthFave(pow, i);
				if (!pFav)
					break;
				static char szBuf[MAX_PATH + 8];

				dbgLink("inserting...", pFav);
				mii.fMask = MIIM_STRING | MIIM_DATA | MIIM_ID;
				mii.fType = MFT_STRING;
				wsprintf(szBuf, "%s\tCtrl+%d", pFav->szFav, (pFav->idCmd - OW_FAVOURITE_CMDID) + 1);
				mii.dwTypeData = szBuf;
				mii.dwItemData = pFav->idCmd;
				mii.wID = pFav->idCmd;

				int iRes = InsertMenuItem(hm, -1, TRUE, &mii);
				if (iRes == 0)
					dbg("%s, Failed inserting item: %s", __func__, geterrmsg());
				//idx = AppendMenu(hm, MF_STRING, iCmd++, pFav->szFav);
			}
		}
		unlockSharedData(pow);
	}

	AppendMenu(hm, MF_SEPARATOR | MF_DISABLED, 0, NULL);
	AppendMenu(hm, MF_STRING, OW_ABOUT_CMDID, "&About OpenWide...");
	TrackPopupMenuEx(hm, TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_VERTICAL, r.left, r.bottom, hwnd, &tpm);
	DestroyMenu(hm);
}

int addIcon2TB(HWND hwTB, HICON hIcn)
{
	HIMAGELIST hImgList = NULL;
	hImgList = (HIMAGELIST)SendMessage(hwTB, TB_GETIMAGELIST, 0, 0);
	if (hImgList)
	{
		int nImgs = ImageList_GetImageCount(hImgList);

		int idxNew = ImageList_AddIcon(hImgList, hIcn);
		if (idxNew == -1)
		{
			dbg("%s, Error adding to imglist: %s", __func__, geterrmsg());
			return -1;
		}
		else
		{
			//dbg("%s, Image added at index %d", __func__, idxNew);
			SendMessage(hwTB, TB_SETIMAGELIST, 0, (LPARAM)hImgList);
			return idxNew;
		}
	}
	else
		return -1;
}

static int addTBButton(HWND hwnd)
{
	HWND hwTB = findChildWindow(hwnd, CID_TOOLBAR, TOOLBARCLASSNAME);

	TBBUTTON tb = { 0 };
	tb.iBitmap = VIEW_NETCONNECT;

	int idxNew = -1;
	HICON hIcn = (HICON)LoadImage(ghInstance, MAKEINTRESOURCE(IDI_TBICON), IMAGE_ICON, 16, 16, 0);
	if (hIcn)
	{
		idxNew = addIcon2TB(hwTB, hIcn);
		DestroyIcon(hIcn);
	}
	if (idxNew >= 0)
		tb.iBitmap = idxNew;
	tb.idCommand = OW_TBUTTON_CMDID;
	tb.fsStyle = BTNS_AUTOSIZE | BTNS_BUTTON | BTNS_SHOWTEXT | BTNS_DROPDOWN;	// BTNS_WHOLEDROPDOWN;
	tb.fsState = TBSTATE_ENABLED;
	tb.iString = (INT_PTR)"OpenWide: Click to add a favourite!";
	SendMessage(hwTB, TB_ADDBUTTONS, 1, (LPARAM) & tb);

	RECT r;
	int idxLast = SendMessage(hwTB, TB_BUTTONCOUNT, 0, 0) - 1;
	if (SendMessage(hwTB, TB_GETITEMRECT, idxLast, (LPARAM) & r))
	{
		RECT rw;
		GetWindowRect(hwTB, &rw);
		MapWindowPoints(hwTB, NULL, (LPPOINT) & r, 2);
		SetWindowPos(hwTB, NULL, 0, 0, (r.right + 8) - rw.left, rw.bottom - rw.top + 1, SWP_NOMOVE | SWP_NOZORDER);
	}
	return 1;
}


static int openWide(HWND hwnd)
{
	// set placement
	int w = gOwShared.szDim.cx;
	int h = gOwShared.szDim.cy;
	int x = gOwShared.ptOrg.x;
	int y = gOwShared.ptOrg.y;
	SetWindowPos(hwnd, NULL, x, y, w, h, SWP_NOZORDER);

	// set view mode
	HWND hwDirCtl = GetDlgItem(hwnd, CID_DIRLISTPARENT);
	WORD vCmdID = viewToCmdID(gOwShared.iView);
	//dbg("Sending message to set view (%d), cmd id %d", gOwShared.iView, vCmdID);
	SendMessage(hwDirCtl, WM_COMMAND, MAKEWPARAM(vCmdID, 0), 0);

	focusDlgItem(hwnd, gOwShared.iFocus);

	// debug hook, to find menu cmd IDs
#ifdef	DEBUG_SYSMSG
	dbg("Hooking SYSMSG...");
	ghSysMsgHook = SetWindowsHookEx(WH_SYSMSGFILTER, SysMsgProc, ghInstance, 0	//GetCurrentThreadId()            // Only install for THIS thread!!!
		);
	dbg("Hooking returned %p", ghSysMsgHook);
#endif

	DragAcceptFiles(hwnd, TRUE);
	//SetWindowLong(hwnd, GWL_STYLE, GetWindowLong(hwnd, GWL_STYLE) | WS_CAPTION | WS_SYSMENU);

/*
	HMENU hm = GetSystemMenu(hwnd, FALSE);
	AppendMenu(hm, MF_SEPARATOR | MF_DISABLED, 0, NULL);
	AppendMenu(hm, MF_STRING, OW_EXPLORE_CMDID, "&Locate current folder with Explorer...");
	AppendMenu(hm, MF_STRING, OW_ABOUT_CMDID, "&About OpenWide...");
*/

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

static int doAddFave(HWND hwnd)
{
	int rv;
	POWSharedData pow = lockSharedData();
	if (pow)
	{
		//printLinks(pow->pFaves);
		dbg("%s: Locked shared data ok", __func__);
		rv = addFavourite(hwnd, pow);
		unlockSharedData(pow);
	}
	return rv;
}

/*
static void	setCDMPath(HWND hwnd, const char *szPath)
{
	SetDlgItemText(hwnd, CID_FNAME, szPath);
	SendDlgItemMessage(hwnd, CID_FNAME, EM_SETSEL, -1, -1);
	SendDlgItemMessage(hwnd, CID_FNAME, EM_REPLACESEL, FALSE, (LPARAM)"\\");
	SendMessage(hwnd, WM_COMMAND, MAKEWPARAM(IDOK, BN_CLICKED), (LPARAM)GetDlgItem(hwnd, IDOK));
	SetDlgItemText(hwnd, CID_FNAME, "");
}
*/

static void getFavourite(HWND hwnd, int idx)
{
	POWSharedData pow = lockSharedData();
	if (pow)
	{
		PFavLink plk = findFaveByCmdID(pow, idx);
		dbg("%s:	findFaveByCmdID(%d) returned %p", __func__, idx, plk);
		if (plk && plk->szFav)
		{
			SetDlgItemText(hwnd, CID_FNAME, plk->szFav);
			SendDlgItemMessage(hwnd, CID_FNAME, EM_SETSEL, -1, -1);
			SendDlgItemMessage(hwnd, CID_FNAME, EM_REPLACESEL, FALSE, (LPARAM)"\\");
		}
		unlockSharedData(pow);
		SendMessage(hwnd, WM_COMMAND, MAKEWPARAM(IDOK, BN_CLICKED), (LPARAM)GetDlgItem(hwnd, IDOK));
		SetDlgItemText(hwnd, CID_FNAME, "");
	}
}

static LRESULT CALLBACK WINAPI wpSubMain(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
/*	if( gbDbg )
	{
		dbgWM(hwnd, msg, wp, lp);
	}*/
	POWSubClassData pow = (POWSubClassData)GetProp(hwnd, OW_PROP_NAME);
	if (!pow)
		return DefWindowProc(hwnd, msg, wp, lp);

	static char buffer[MAX_PATH + 1];

	switch (msg)
	{
		case WM_INITDIALOG:
		{
			LRESULT lRes = CallWindowProc(pow->wpOrig, hwnd, msg, wp, lp);
			if (!initSharedMem())
			{
				dbg("WM_INITDIALOG: initSharedMem failed");
				unsubclass(hwnd);
				return lRes;
			}
			//dbg("subclass wproc got WM_INITDIALOG, %p, %p", wp, lp);
			addTBButton(hwnd);
			initPlaces(hwnd);
			ShowWindow(hwnd, SW_HIDE);
			return lRes;
		}
			break;
		case WM_SHOWWINDOW:
			if (wp && !pow->bSet)
			{
				pow->bSet = TRUE;
				//subclass(GetDlgItem(hwnd, CID_DIRLISTPARENT), wpSubShellCtl, (LPARAM)0);
				dbg("%s : First wm_showwindow, now modifying dlg...", __func__);
				openWide(hwnd);
			}
			break;
		case WM_COMMAND:
			switch (LOWORD(wp))
			{
				case OW_ABOUT_CMDID:
					MessageBox(hwnd, "OpenWide is written by Luke Hudson. (c)2005", "About OpenWide", MB_OK);
					return 0;
				case OW_EXPLORE_CMDID:
				{
					char *szParm = "/select,";
					wsprintf(buffer, szParm);
					int len = strlen(szParm);
					len = SendMessage(hwnd, CDM_GETFOLDERPATH, MAX_PATH - len, (LPARAM) (buffer + len));
					if (len)
					{
						ShellExecute(hwnd, NULL, "explorer.exe", buffer, NULL, SW_SHOWNORMAL);
					}
				}
					return 0;
				case OW_TBUTTON_CMDID:
				case OW_ADDFAV_CMDID:
					doAddFave(hwnd);
					return 0;
				default:
					if (LOWORD(wp) >= OW_FAVOURITE_CMDID)
					{
						getFavourite(hwnd, (int)LOWORD(wp));
					}
					break;
			}
			break;
		case WM_PARENTNOTIFY:
			if (LOWORD(wp) == WM_CREATE)
			{
				static char buf[33];
				GetClassName((HWND)lp, buf, 32);

				if (strcmp(buf, "SHELLDLL_DefView") == 0)
				{
					//dbg("Shell defview ctl created");
					//subclass((HWND)lp, wpSubShellCtl, (LPARAM)0);
					HWND hwLV = GetDlgItem((HWND)lp, 1);
					//DragAcceptFiles(hwLV, TRUE);
					if (hwLV)
					{
						if (GetWindowLong(hwLV, GWL_STYLE) & LVS_REPORT)
						{
							//dbg("hwLV is in report mode -- setting extended style");
							ListView_SetExtendedListViewStyleEx(hwLV, OW_LISTVIEW_STYLE, OW_LISTVIEW_STYLE);
						}
					}
					SetTimer(hwnd, 251177, 1, NULL);
				}
			}
			break;
		case WM_KEYUP:
			if (wp >= VK_0 && wp <= VK_9)
			{
				if ((short)GetAsyncKeyState(VK_CONTROL) < 0)
				{
					int idx = wp - VK_0;
					getFavourite(hwnd, idx);
				}
			}
			break;
		case WM_TIMER:
			if (wp == 251177)
			{
				KillTimer(hwnd, 251177);
				HWND hwDirCtl = GetDlgItem(hwnd, CID_DIRLISTPARENT);
				if (getSharedData())
				{
					if (gOwShared.iView == V_THUMBS || gOwShared.iView == V_TILES)
					{
						dbg("openwide.dll : posting cmd message to reset view?");
						WORD vCmdID = viewToCmdID(gOwShared.iView);
						PostMessage(hwDirCtl, WM_COMMAND, MAKEWPARAM(vCmdID, 0), 0);
					}
				}
			}
			break;
/*-		case WM_SYSCOMMAND:
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
			break;*/
		case WM_NOTIFY:
		{
			NMHDR *phdr = (NMHDR *)lp;
			HWND hwTB = findChildWindow(hwnd, CID_TOOLBAR, TOOLBARCLASSNAME);
			if (phdr->hwndFrom == hwTB)
			{
//                  dbg("Got notify %d from toolbar", phdr->code);
				if (phdr->code == TBN_DROPDOWN)
				{
					NMTOOLBAR *ptb = (NMTOOLBAR *)lp;
					if (ptb->iItem == OW_TBUTTON_CMDID)
					{
						dropMenu(hwnd, hwTB, ptb->iItem);
						return TBDDRET_DEFAULT;
					}
				}
			}
/*				HWND hwSV = GetDlgItem(hwnd, CID_DIRLISTPARENT);
				hwSV = GetDlgItem(hwSV, CID_DIRLIST);
				if( phdr->hwndFrom == hwSV )
				{
					dbg("Got notify %d from listview", phdr->code);
				}*/
		}
			break;
		case WM_DROPFILES:
		{
			HANDLE hDrop = (HANDLE)wp;
			int nFiles = DragQueryFile(hDrop, (UINT) - 1, NULL, 0);
			//dbg("%d files dropped on main window %p", nFiles, hwnd);
			if (nFiles == 1)
			{
				if (DragQueryFile(hDrop, 0, buffer, MAX_PATH))
				{
					if (PathIsDirectory(buffer))
					{
						SetDlgItemText(hwnd, CID_FNAME, buffer);
						SendDlgItemMessage(hwnd, CID_FNAME, EM_SETSEL, -1, -1);
						SendDlgItemMessage(hwnd, CID_FNAME, EM_REPLACESEL, FALSE, (LPARAM)"\\");
						SendMessage(hwnd, WM_COMMAND, MAKEWPARAM(IDOK, BN_CLICKED), (LPARAM)GetDlgItem(hwnd, IDOK));
						SetDlgItemText(hwnd, CID_FNAME, "");
						if (getSharedData())
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

static int initSharedMem(void)
{
	ghMutex = OpenMutex(SYNCHRONIZE, FALSE, OW_MUTEX_NAME);
	if (!ghMutex)
		return 0;
	return 1;
}


POWSharedData lockSharedData(void)
{
	int rv = 0;
	dbg("%s:  waiting for mutex", __func__);
	if (ghMutex)
	{
		DWORD dwRes = WaitForSingleObject(ghMutex, 200);
		if (dwRes != WAIT_OBJECT_0)
		{
			dbg("%s:	error, waitmutex returned %u", __func__, dwRes);
			return NULL;
		}
	}
	dbg("%s:  got mutex", __func__);
	if (ghMap == NULL)
	{
		ghMap = OpenFileMapping(FILE_MAP_WRITE, FALSE, OW_SHARED_FILE_MAPPING);
		dbg("%s:  openFileMapping returned %p", __func__, ghMap);
	}
	if (ghMap)
	{
		dbg("%s:  opened file mapping", __func__);
		POWSharedData pow = (POWSharedData)MapViewOfFile(ghMap, FILE_MAP_WRITE, 0, 0, 0);
		if (pow)
		{
			dbg("%s:  mapped view of file (%p)", __func__, pow);
			if (lockFaves(pow))
				return pow;
		}
		dbg("%s:	failed to map view of file, closing handle", __func__);
		CloseHandle(ghMap);
		ghMap = NULL;
	}
	else
		dbg("%s, failed to open filemapping : %s", __func__, geterrmsg());
	return NULL;
}

int unlockSharedData(POWSharedData pow)
{
	if (ghMutex)
	{
		dbg("%s:  waiting for mutex", __func__);
		DWORD dwRes = WaitForSingleObject(ghMutex, 1000);
		if (dwRes != WAIT_OBJECT_0)
		{
			dbg("%s:	error, waitmutex returned %u", __func__, dwRes);
			return 0;
		}
		dbg("%s:  got mutex", __func__);
	}
	dbg("%s:  unlocking...", __func__);
	if (pow)
	{
		unlockFaves(pow);
		UnmapViewOfFile(pow);
	}
	CloseHandle(ghMap);
	ghMap = NULL;

	if (ghMutex)
	{
		dbg("%s:  releasing mutex", __func__);
		ReleaseMutex(ghMutex);
		ghMutex = NULL;
	}
	return 1;
}

static int getSharedData(void)
{
	int rv = 0;
	if (ghMutex)
	{
		DWORD dwRes = WaitForSingleObject(ghMutex, 200);
		if (dwRes != WAIT_OBJECT_0)
			return 0;
	}
	HANDLE hMap = OpenFileMapping(FILE_MAP_READ, TRUE, OW_SHARED_FILE_MAPPING);
	if (hMap)
	{
		POWSharedData pow = (POWSharedData)MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, 0);
		if (pow)
		{
			__try
			{
				CopyMemory(&gOwShared, pow, sizeof(OWSharedData));
				//dbg("%s, openwide.dll : Got shared mem okay", __func__);
				rv = 1;
			}
			__except(EXCEPTION_EXECUTE_HANDLER)
			{
				dbg("%s, Exception in getSharedMemory: %d", __func__, exception_code());
			}
			UnmapViewOfFile(pow);
		}
		else
			dbg("%s, Failed to get ptr to shared data: %d", __func__, GetLastError());

		CloseHandle(hMap);
	}

	if (ghMutex)
		ReleaseMutex(ghMutex);
	return rv;
}


BOOL DLLEXPORT isWinXP(void)
{
	return GetDllVersion("Shell32.dll") >= PACKVERSION(6, 00);
}

static WORD viewToCmdID(int iView)
{
	BOOL bXP = isWinXP();
	switch (iView)
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

static WORD focusToCtlID(int iFocus)
{
	//BOOL bXP = isWinXP();
	switch (iFocus)
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

static int focusDlgItem(HWND hwnd, int iFocus)
{
	UINT uID = focusToCtlID(iFocus);
	if (uID == CID_DIRLIST)
	{
		return SetFocus(GetDlgItem(GetDlgItem(hwnd, CID_DIRLISTPARENT), uID)) != NULL;
	}
	else
		return SetFocus(GetDlgItem(hwnd, uID)) != NULL;
}
/*
static void CALLBACK timerProc(HWND hwnd, UINT uMsg, UINT uID, DWORD dwTime)
{
	KillTimer(hwnd, 251177);
	//gbDbg = FALSE;
	openWide(hwnd);
//	SendMessage(hwnd, CDM_SETCONTROLTEXT, edt1, (LPARAM)"You've been hooked in!!!");
}*/


static BOOL isOpenSaveDlg(HWND hwNew, LPCREATESTRUCT pcs)
{
	static char buf[256];
	if (GetClassName(hwNew, buf, 255) && pcs->lpszName)
	{
		DWORD style = (DWORD)GetWindowLong(hwNew, GWL_STYLE);
		DWORD exStyle = (DWORD)GetWindowLong(hwNew, GWL_EXSTYLE);
		if (style == OW_MATCH_STYLE && exStyle == OW_MATCH_EXSTYLE && strcmp(buf, "#32770") == 0 && strcmp(pcs->lpszName, "Open") == 0)
			return TRUE;
	}
	return FALSE;
/*
	return findChildWindow(hwNew, CID_FNAME, WC_COMBOBOXEX) 
	&& findChildWindow(hwNew, CID_FTYPE, WC_COMBOBOX)
	&& findChildWindow(hwNew, CID_LOOKIN, WC_COMBOBOX);*/
}

static LRESULT CALLBACK CBTProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode < 0)
		return CallNextHookEx(ghMsgHook, nCode, wParam, lParam);

	switch (nCode)
	{
		case HCBT_CREATEWND:
		{
			HWND hwNew = (HWND)wParam;
			CBT_CREATEWND *pcw = (CBT_CREATEWND *)lParam;
			LPCREATESTRUCT pcs = (LPCREATESTRUCT)pcw->lpcs;

			if (isOpenSaveDlg(hwNew, pcs))
			{
				if (getSharedData())
				{
					dbg("%s, Patched window %p", __func__, hwNew);
					pcs->style &= ~WS_VISIBLE;
					//ShowWindow(hwNew, SW_HIDE);
					//dbg("%s, Subclassing now:", __func__);
					subclass(hwNew, wpSubMain, 0);
				}
			}
		}
			return 0;
	}
	// Call the next hook, if there is one
	return CallNextHookEx(ghMsgHook, nCode, wParam, lParam);
}


static void releaseSharedMem(void)
{
	DWORD dwRes = WaitForSingleObject(ghMutex, INFINITE);
	if (dwRes == WAIT_OBJECT_0)
		CloseHandle(ghMutex);
}

int DLLEXPORT setHook(HWND hwLB)
{
	//if( timeoutSeconds == 0 )
	//  return MessageBox(hwParent, szMsg, szTitle, uType);
	if (gbHooked)
	{
		SendMessage(gOwShared.hwLog, LB_ADDSTRING, 0, (LPARAM)"Already hooked");
		return 1;
	}
	else if (!initSharedMem())
		return FALSE;

//  dbg("List box window handle is %p", hwLB);
	if (!getSharedData())
	{
		SendMessage(hwLB, LB_ADDSTRING, 0, (LPARAM)"Failed to get shared data!!!");
		return FALSE;
	}
	SendMessage(gOwShared.hwLog, LB_ADDSTRING, 0, (LPARAM)"Setting hook....");
	ghMsgHook = SetWindowsHookEx(WH_CBT, CBTProc, ghInstance, 0	//GetCurrentThreadId()            // Only install for THIS thread!!!
		);
	if (ghMsgHook != NULL)
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
	if (!gbHooked || !ghMsgHook)
		return 1;

	if (ghSysMsgHook)
	{
		UnhookWindowsHookEx(ghSysMsgHook);
		ghSysMsgHook = NULL;
	}

	if (ghMsgHook)
	{
		UnhookWindowsHookEx(ghMsgHook);
		gbHooked = FALSE;
		ghMsgHook = NULL;
	}
	releaseSharedMem();
	return 1;
}


#ifdef DEBUG_SYSMSG
static LRESULT CALLBACK SysMsgProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode < 0)
		return CallNextHookEx(ghMsgHook, nCode, wParam, lParam);

	LPMSG pMsg = (MSG *)lParam;
	if (pMsg->message < WM_MOUSEFIRST || pMsg->message > WM_MOUSELAST)
	{
		dbg("SysMsgProc: %d is code, pMsg: hwnd: %p, message: %d, wp: %x, lp: %x", nCode, pMsg->hwnd, pMsg->message, pMsg->wParam, pMsg->lParam);
	}
	if (GetAsyncKeyState(VK_SHIFT) & 0x8000)
	{
		LRESULT lRet = CallNextHookEx(ghSysMsgHook, nCode, wParam, lParam);
		UnhookWindowsHookEx(ghSysMsgHook);
		ghSysMsgHook = NULL;
		return lRet;
	}
	switch (pMsg->message)
	{
		case WM_MENUSELECT:
		{
			dbg("%s, WM_MENUSELECT: %p %d", __func__, pMsg->hwnd, LOWORD(pMsg->wParam));

			HMENU hm = (HMENU)pMsg->lParam;
			if (hm)
			{
				static char buf[80];
				MENUITEMINFO mii = { 0 };
				int nItems = GetMenuItemCount(hm);
				dbg(" %d items in menu", nItems);
				for (int i = 0; i < nItems; i++)
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
#endif


DWORD DLLEXPORT GetDllVersion(LPCTSTR lpszDllName)
{
	HINSTANCE hinstDll;
	DWORD dwVersion = 0;
	char szDll[MAX_PATH + 1];
	/* For security purposes, LoadLibrary should be provided with a
	   fully-qualified path to the DLL. The lpszDllName variable should be
	   tested to ensure that it is a fully qualified path before it is used. */
	if (!PathSearchAndQualify(lpszDllName, szDll, MAX_PATH))
		return 0;
	hinstDll = LoadLibrary(szDll);

	if (hinstDll)
	{
		DLLGETVERSIONPROC pDllGetVersion;
		pDllGetVersion = (DLLGETVERSIONPROC)GetProcAddress(hinstDll, "DllGetVersion");

		/* Because some DLLs might not implement this function, you
		   must test for it explicitly. Depending on the particular
		   DLL, the lack of a DllGetVersion function can be a useful
		   indicator of the version. */

		if (pDllGetVersion)
		{
			DLLVERSIONINFO dvi;
			HRESULT hr;

			ZeroMemory(&dvi, sizeof(dvi));
			dvi.cbSize = sizeof(dvi);

			hr = (*pDllGetVersion) (&dvi);

			if (SUCCEEDED(hr))
			{
				dwVersion = PACKVERSION(dvi.dwMajorVersion, dvi.dwMinorVersion);
			}
		}

		FreeLibrary(hinstDll);
	}
	return dwVersion;
}


/*------------------------------------------------------------------------
 Procedure:     DllMain ID:1
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
BOOL DLLEXPORT CALLBACK WINAPI DllMain(HINSTANCE hDLLInst, DWORD fdwReason, LPVOID lpvReserved)
{
	switch (fdwReason)
	{
		case DLL_PROCESS_ATTACH:
			// The DLL is being loaded for the first time by a given process.
			// Perform per-process initialization here.  If the initialization
			// is successful, return TRUE; if unsuccessful, return FALSE.
			ghInstance = hDLLInst;
		//initSharedMem();
			break;
		case DLL_PROCESS_DETACH:
			// The DLL is being unloaded by a given process.  Do any
			// per-process clean up here, such as undoing what was done in
			// DLL_PROCESS_ATTACH.  The return value is ignored.
			if (ghMutex)
			{
				CloseHandle(ghMutex);
				ghMutex = NULL;
			}
			ghInstance = NULL;
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


/*
static int		cmdIDToView(WORD cmdID)
{
	if( !isWinXP() )
	{
		switch(cmdID)
		{
			case CMD_2K_DETAILS:
				return V_DETAILS;
			case CMD_2K_THUMBS:
				return V_THUMBS;
			case CMD_2K_LIST:
				return V_LIST;
			case CMD_2K_LGICONS:
				return V_LGICONS;
			case CMD_2K_SMICONS:
				return V_SMICONS;
			default:
				return V_MAX;
		}
	}
	else
	{
		switch(cmdID)
		{
			case CMD_XP_DETAILS:
				return V_DETAILS;
			case CMD_XP_THUMBS:
				return V_THUMBS;
			case CMD_XP_LIST:
				return V_LIST;
			case CMD_XP_LGICONS:
				return V_LGICONS;
			case CMD_XP_TILES:
				return V_TILES;
			default:
				return V_MAX;
		}
	}
}
*/

/*
static LRESULT CALLBACK WINAPI wpSubShellCtl(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
	//dbg("Shellctl got wmcommand: id %d, code %d", LOWORD(wp), HIWORD(wp));
	//dbgWM(hwnd, msg, wp, lp);

	static BOOL	bIsXP = FALSE;

	POWSubClassData pow = (POWSubClassData)GetProp(hwnd, OW_PROP_NAME);
	if( !pow )
		return DefWindowProc(hwnd, msg, wp, lp);

	switch(msg)
	{
		case WM_CREATE:
			bIsXP = isWinXP();
			break;
		case WM_DROPFILES:
			{
				HANDLE hDrop = (HANDLE)wp;
				int nFiles = DragQueryFile(hDrop, (UINT)-1, NULL, 0);
				dbg("%d files dropped, fwding mesg to %p", nFiles, pow->lpData);
				return SendMessage((HWND)pow->lpData, msg, wp, lp);
			}
			break;
		case WM_COMMAND:
			//dbg("Shellctl got wmcommand: id %d, code %d", LOWORD(wp), HIWORD(wp));
			{
				int view = cmdIDToView(LOWORD(wp));
				if( view >= 0 && view < V_MAX )
				{
					dbg("User reset view to %d", view);
					if( getSharedData() )
					{
						gOwShared.iView = view;
						//setSharedData();
					}
				}
			}
			break;
		case WM_NOTIFY:
			{
//				LPNMHDR pHdr = (LPNMHDR)lp;
//				dbg("Shellctl got WMNOTIFY: id %d, code %d", pHdr->idFrom, pHdr->code);
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
*/
