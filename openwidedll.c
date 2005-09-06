/* --- The following code comes from c:\lcc\lib\wizard\dll.tpl. */
#include <windows.h>
#include <commctrl.h>
#include	<shlwapi.h>
#include	<shellapi.h>
#include	<shlobj.h>
#include	<stdarg.h>
#include	<stdio.h>
#include	<dlgs.h>
#include	"openwidedll.h"
#include	"openwidedllres.h"
#include	"owDllInc.h"
#include	"owSharedUtil.h"


HINSTANCE		ghInst = NULL;
HANDLE			ghMutex = NULL;
POWSharedData	gpSharedMem = NULL;	// pointer to shared mem
HANDLE			ghMap		= NULL;
HHOOK			ghMsgHook	= NULL, ghSysMsgHook = NULL;

OWSharedData	gOwShared; // stores copy of shared mem for access to non-pointer datas without extended blocking



int addIcon2TB(HWND hwTB, HICON hIcn)
{
	HIMAGELIST hImgList = NULL;
	hImgList = (HIMAGELIST)SendMessage(hwTB, TB_GETIMAGELIST, 0, 0);
	if (hImgList)
	{
//		int nImgs = ImageList_GetImageCount(hImgList);
		int idxNew = ImageList_AddIcon(hImgList, hIcn);
		if (idxNew == -1)
		{
			//dbg("%s, Error adding to imglist: %s", __func__, geterrmsg());
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
	HICON hIcn = (HICON)LoadImage(ghInst, MAKEINTRESOURCE(IDI_TBICON), IMAGE_ICON, 16, 16, 0);
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
	tb.iString = (INT_PTR)"OpenWide by Lingo";
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
	AppendMenu(hm, MF_STRING, OW_SHOWDESK_CMDID, "Show &Desktop [for Gabriel]...");
	SetMenuDefaultItem(hm, OW_SHOWDESK_CMDID, FALSE);
/*
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
*/
	AppendMenu(hm, MF_SEPARATOR | MF_DISABLED, 0, NULL);
	AppendMenu(hm, MF_STRING, OW_ABOUT_CMDID, "&About OpenWide...");
	TrackPopupMenuEx(hm, TPM_HORIZONTAL | TPM_RIGHTALIGN | TPM_LEFTBUTTON | TPM_VERTICAL, r.right, r.bottom, hwnd, &tpm);
	DestroyMenu(hm);
}
/*
int addPlace(HWND hwnd, PFavLink plk)
{
	HWND hwTB = GetDlgItem(hwnd, CID_PLACES);

	TBBUTTON tb = { 0 };
	tb.iBitmap = giPlacesIcon;
	tb.idCommand = plk->idCmd;
	tb.fsStyle = BTNS_BUTTON | BTNS_SHOWTEXT;	// BTNS_WHOLEDROPDOWN;
	tb.fsState = TBSTATE_ENABLED;
	tb.iString = (INT_PTR)PathFindFileName(plk->szFav);
	SendMessage(hwTB, TB_ADDBUTTONS, 1, (LPARAM) & tb);
	return 1;
}*/
/*
int	addFavourite(HWND hwnd)
{
	//static char szBuf[2*MAX_PATH+1];
	assert( IsWindow(hwnd));

	char *szBuf = (char *)GlobalAlloc(GPTR, 2 * MAX_PATH + 1);
	if( !szBuf )
		return 0;
	if( SendMessage(hwnd, CDM_GETFOLDERPATH, 2*MAX_PATH, (LPARAM)szBuf) > 0 )
	{		
		if( !faveExists(szBuf) )
		{
			PFavLink plNew = newFav(szBuf);
			if( plNew )
				addPlace(hwnd, plNew);
			else
				dbg("%s, failed to create new fave", __func__);
		}
		else
			dbg("%s, Fave: '%s' exists already", __func__, szBuf);
		return 1;
	}
	GlobalFree(szBuf);
	return 0;
}
*/
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
#ifdef	HOOK_SYSMSG
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

	addTBButton(hwnd);
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

void	showDesktop(HWND hwnd)
{
	char * szDesk = GlobalAlloc(GPTR, MAX_PATH+1);
	if( szDesk && SHGetSpecialFolderPath(hwnd, szDesk, CSIDL_DESKTOPDIRECTORY,FALSE) )
	{
		char *szOld = getDlgItemText(hwnd, CID_FNAME);
		SetDlgItemText(hwnd, CID_FNAME, szDesk);
		SendDlgItemMessage(hwnd, CID_FNAME, EM_SETSEL, -1, -1);
		SendDlgItemMessage(hwnd, CID_FNAME, EM_REPLACESEL, FALSE, (LPARAM)"\\");
		SendMessage(hwnd, WM_COMMAND, MAKEWPARAM(IDOK, BN_CLICKED), (LPARAM)GetDlgItem(hwnd, IDOK));
		if( szOld )
			SetDlgItemText(hwnd, CID_FNAME, szOld);
		else
			SetDlgItemText(hwnd, CID_FNAME, "");
		free(szOld);
		GlobalFree(szDesk);
	}
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
		case WM_COMMAND:
			switch (LOWORD(wp))
			{
				case OW_ABOUT_CMDID:
					MessageBox(hwnd, "OpenWide is written by Luke Hudson. (c)2005", "About OpenWide", MB_OK);
					return 0;
				case OW_TBUTTON_CMDID:
				case OW_SHOWDESK_CMDID:
					showDesktop(hwnd);
					break;
				case OW_EXPLORE_CMDID:
				{
					char *szParm = "/select,";
					wsprintf(buffer, szParm);
					int len = strlen(szParm);
					LPARAM lpBuf = (LPARAM)buffer + (LPARAM)len;
					dbg("CDM_GET..PATH, cbSize=%d, buffer = %p", MAX_PATH-len, lpBuf);
					len = SendMessage(hwnd, CDM_GETFOLDERPATH, MAX_PATH - len, lpBuf); //(LPARAM)(char *)((unsigned int)buffer + (unsigned int)len));
					if (len)
					{
						dbg("Getfolderpath returned len %d, path: \"%s\"",len, buffer);
						ShellExecute(hwnd, NULL, "explorer.exe", buffer, NULL, SW_SHOWNORMAL);
					}
				}
					return 0;
/*
				case OW_ADDFAV_CMDID:
					doAddFave(hwnd);
					return 0;
				default:
					if (LOWORD(wp) >= OW_FAVOURITE_CMDID)
					{
						getFavourite(hwnd, (int)LOWORD(wp));
					}
					break;*/
			}
			break;

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
		}
		break;

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
				//dbg("DLL: DESTROY");
				if( openSharedMem() )
				{
					//dbg("DLL: Opened shared memory");
					if( --gpSharedMem->refCount < 0 )
						gpSharedMem->refCount = 0;
					//dbg("DLL: dec'd refCount to %d, posting msg %x to app window %p", gpSharedMem->refCount, gpSharedMem->iCloseMsg, gpSharedMem->hwListener);
					PostMessage( gpSharedMem->hwListener, gpSharedMem->iCloseMsg, 0,0);
					closeSharedMem();
					//dbg("DLL: Closed shared memory");
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
				////dbg("%d files dropped, fwding mesg to %p", nFiles, pow->lpData);
				return SendMessage((HWND)pow->lpData, msg, wp, lp);
			}
			break;
		case WM_DESTROY:
			{
				WNDPROC wpOrig = pow->wpOrig;
				unsubclass(hwnd);
				//dbg("SHell view being destroyed");
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
						//dbg("DLL: Module: %s", szApp);
						free(szApp);
					}

					///dbg("DLL: Found O&S dlg %p, attempting access to shared mem", hwNew);

					if( bTakeOver && openSharedMem() )
					{
						//dbg("DLL: Opened shared memory");
						if( gpSharedMem->bDisable )
							bTakeOver = FALSE;
						else
							gpSharedMem->refCount++;
						//dbg("DLL: Inc'd refCount to %d", gpSharedMem->refCount);
						closeSharedMem();
						//dbg("DLL: Closed shared memory");
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


int DLLEXPORT setHook(void)
{
	if(ghMsgHook != NULL)
	{
		//SendMessage(gOwShared.hwLog, LB_ADDSTRING, 0, (LPARAM)"Already hooked");
		return 1;
	}
	if(!getSharedData())
	{
//		SendMessage(hwLB, LB_ADDSTRING, 0, (LPARAM)"Failed to get shared data!!!");
	//	dbg("Hook failed to get shared mems");
		return FALSE;
	}
	//SendMessage(gOwShared.hwLog, LB_ADDSTRING, 0, (LPARAM)"Setting hook....");
	ghMsgHook = SetWindowsHookEx(WH_CBT, CBTProc, ghInst, 0);
	if(ghMsgHook != NULL)
	{
		//dbg("Hooked okay");
		//SendMessage(gOwShared.hwLog, LB_ADDSTRING, 0, (LPARAM)"Hooked ok!");
		return 1;
	}
	//dbg("Hook failed");
	//SendMessage(gOwShared.hwLog, LB_ADDSTRING, 0, (LPARAM)"Failed hook");
	return 0;
}


int DLLEXPORT rmvHook(void)
{
	if( !ghMsgHook )
		return 1;

	if( ghSysMsgHook )
	{
		UnhookWindowsHookEx(ghSysMsgHook);
		ghSysMsgHook = NULL;
	}
	//dbg("DLL: Removing hook");
	if( ghMsgHook )
	{
		UnhookWindowsHookEx(ghMsgHook);
		ghMsgHook = NULL;
	}
	//dbg("DLL: removed hook okay");
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

BOOL DLLEXPORT WINAPI DLLPROC(HINSTANCE hDLLInst, DWORD fdwReason, LPVOID lpvReserved)
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

