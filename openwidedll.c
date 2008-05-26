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


// DLL Instance handle
HINSTANCE		ghInst = NULL;

// Handle to shared memory access locking mutex.
HANDLE			ghMutex = NULL;
POWSharedData	gpSharedMem = NULL;	// pointer to shared mem
// Handle to filemapping for shared mem
HANDLE			ghMap		= NULL;
// System hook handles
HHOOK			ghMsgHook	= NULL, ghSysMsgHook = NULL;

// This cache is refreshed -- TODO: When?
OWSharedData	gOwShared; // stores copy of shared mem for access to non-pointer datas without extended blocking



/* Add an icon to the toolbar within the O&S Dialogs 
 * @param hwTB  -- toolbar handle
 * @param hIcn  -- icon handle
 *
 * @returns -1 on error, or the index of the new icon within the imagelist.
 */
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
    return -1;
}

/*
 * Adds a new button to the toolbar : hwnd
 *
 * @param hwnd  -- parent window of toolbar, ie. the O&S Dialog.
 *
 * @returns 1 -- TODO: this should return 0 on error, but doesn't appear to.
 */
static int addTBButton(HWND hwnd)
{
    // Locate the toolbar handle.
	HWND hwTB = findChildWindow(hwnd, CID_TOOLBAR, TOOLBARCLASSNAME);

    // Create toolbar button structure
	TBBUTTON tb = { 0 };
	tb.iBitmap = VIEW_NETCONNECT;

    // Load the toolbar icon, and add it to the imagelist, retaining the index.
	int idxNew = -1;
	HICON hIcn = (HICON)LoadImage(ghInst, MAKEINTRESOURCE(IDI_TBICON), IMAGE_ICON, 16, 16, 0);
	if (hIcn)
	{
		idxNew = addIcon2TB(hwTB, hIcn);
		DestroyIcon(hIcn);
	}
	if (idxNew >= 0)
		tb.iBitmap = idxNew;    // set button image index
	tb.idCommand = OW_TBUTTON_CMDID;    // set command id -- @see openwidedll.h
    // Set the button style flags
	tb.fsStyle = BTNS_AUTOSIZE | BTNS_BUTTON | BTNS_SHOWTEXT | BTNS_DROPDOWN;	// BTNS_WHOLEDROPDOWN;
    // Set the button state flags
	tb.fsState = TBSTATE_ENABLED;
    // And give it a tooltip ? TODO: Check this
	tb.iString = (INT_PTR)"OpenWide by Lingo";
    // Add the button.
	SendMessage(hwTB, TB_ADDBUTTONS, 1, (LPARAM) & tb);

    // Ensure that the toolbar window is large enough to show the new button.
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
} // END of addTBButton(...)

/*
 * Show a drop-down menu from the toolbar button.
 *
 * @param hwnd  --  O&S dialog
 * @param hwTB  --  toolbar handle
 * @param uiItem -- id of toolbar item ? -- TODO: Check this.
 */
static void dropMenu(HWND hwnd, HWND hwTB, UINT uiItem)
{
	RECT r;
    // Get the screen coords rectangle of the button
	SendMessage(hwTB, TB_GETRECT, uiItem, (LPARAM) & r);
	MapWindowPoints(hwTB, NULL, (LPPOINT) & r, 2);

    // Set the area for the menu to avoid. ? :: TODO: see Platform SDK on TrackPopupMenuEx
	TPMPARAMS tpm = { 0 };
	tpm.cbSize = sizeof(tpm);
	tpm.rcExclude = r;

    // Create the menu structure.
	HMENU hm = CreatePopupMenu();
	AppendMenu(hm, MF_STRING, OW_EXPLORE_CMDID, "&Locate current folder with Explorer...");
	AppendMenu(hm, MF_STRING, OW_SHOWDESK_CMDID, "Show &Desktop [for Gabriel]...");
	AppendMenu(hm, MF_SEPARATOR | MF_DISABLED, 0, NULL);
	AppendMenu(hm, MF_STRING, OW_ABOUT_CMDID, "&About OpenWide...");
	SetMenuDefaultItem(hm, OW_SHOWDESK_CMDID, FALSE);
/*
// * This section is todo with having Favourite directories, and is unfinished.
//
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
    // Display, track, then destroy the menu.
	TrackPopupMenuEx(hm, TPM_HORIZONTAL | TPM_RIGHTALIGN | TPM_LEFTBUTTON | TPM_VERTICAL, r.right, r.bottom, hwnd, &tpm);
	DestroyMenu(hm);
} // END of dropMenu(...)


/*
// add a new 'place' into the Places bar.
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
// Add a new favourite dir, first retrieving the path from the current view of the O&S dialog.
// *unfinished*
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

/*
 * Given the handle to the O&S dialog, this does the magic to it, such as
 * adding toolbar buttons, setting the focus and view mode, and adding items to
 * the system menu.
 *
 * @returns 1, TODO:: There should be error indication, perhaps!
 */
int		openWide(HWND hwnd)
{
	// set placement
	int w = gOwShared.szDim.cx;
	int h = gOwShared.szDim.cy;
	int x = gOwShared.ptOrg.x;
	int y = gOwShared.ptOrg.y;
	SetWindowPos(hwnd, NULL, x, y, w, h, SWP_NOZORDER);

	// set view mode
	HWND hwDirCtl = GetDlgItem(hwnd, CID_DIRLISTPARENT);
	WORD	vCmdID = viewToCmdID(gOwShared.iView);
    // set the view mode, by sending a fake command message.
	SendMessage(hwDirCtl, WM_COMMAND, MAKEWPARAM(vCmdID, 0), 0);

    // set the focus!
	focusDlgItem(hwnd, gOwShared.iFocus);

#ifdef	HOOK_SYSMSG
	// debug hook, to find menu cmd IDs
	dbg("Hooking SYSMSG...");
	ghSysMsgHook = SetWindowsHookEx(
						WH_SYSMSGFILTER,
				        SysMsgProc,
				        ghInst,
				        0 //GetCurrentThreadId()            // Only install for THIS thread!!!
				);
	dbg("Hooking returned %p", ghSysMsgHook);
#endif
    // Allow drag&drop onto window. Unfortunately this doesn't work for the
    // directory list, as that is already set to accept drops, as a command to
    // copy/move files.  I would have to subclass this too, and that's rather
    // more complicated.
	DragAcceptFiles(hwnd, TRUE);

    // Insert item into the system menu (right-click on titlebar)
	HMENU hm = GetSystemMenu(hwnd, FALSE);
	AppendMenu(hm, MF_SEPARATOR | MF_DISABLED, 0, NULL);
	AppendMenu(hm, MF_STRING, OW_EXPLORE_CMDID, "&Locate current folder with Explorer...");
	AppendMenu(hm, MF_STRING, OW_ABOUT_CMDID, "&About OpenWide...");

	addTBButton(hwnd);  // add the toolbar button.
/*
//  Modify the dir-list view flags
	HWND hwShellCtl = GetDlgItem(hwnd, CID_DIRLISTPARENT);
	hwShellCtl = GetDlgItem(hwShellCtl, 1);
	ListView_SetExtendedListViewStyleEx(hwShellCtl, OW_LISTVIEW_STYLE, OW_LISTVIEW_STYLE );
*/
	return 1;
} // END openWide(...)


/* Load the desktop folder into the dir list */
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
} // END showDesktop


/*
 * This is the callback for the subclassed O&S dialog window, and it handles
 * all the extra functionality, passing along messages to the old callback
 * function unless the behaviour is modified.
 *
 */
LRESULT CALLBACK WINAPI wpSubMain(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
/*	if( gbDbg )
	{
		dbgWM(hwnd, msg, wp, lp);
	}*/

    // retrieve a ptr to the extra information associated with the window.
	POWSubClassData pow = (POWSubClassData)GetProp(hwnd, OW_PROP_NAME);
	if( !pow )
		return DefWindowProc(hwnd, msg, wp, lp);    // something's broken, so just allow default window function.

	static char buffer[MAX_PATH+1];

	switch(msg)
	{
		case WM_INITDIALOG:
			{
                // dbg("DLL: INITDIALOG");
				LRESULT lRes = CallWindowProc(pow->wpOrig, hwnd, msg, wp, lp);  // call the old callback
				ShowWindow(hwnd, SW_HIDE);  // hide the window, until it's been magick-ed by openWide(...)
				return lRes;
			}
			break;

		case WM_SHOWWINDOW:
			if( wp && !pow->bSet )  // catch the first SHOWWINDOW only, 
			{
				pow->bSet = TRUE;
				openWide(hwnd);
			}
			break;

		case WM_COMMAND:            // handle custom toolbar button commands, etc.
			switch (LOWORD(wp))
			{
				case OW_ABOUT_CMDID:    // about
					MessageBox(hwnd, "OpenWide is written by Luke Hudson. (c)2005", "About OpenWide", MB_OK);
					return 0;
                // show desktop item, or click on button rather than
                // dropdown (ie, defaults to show desktop menuitem)
				case OW_TBUTTON_CMDID:
				case OW_SHOWDESK_CMDID:
					showDesktop(hwnd);
					break;
				case OW_EXPLORE_CMDID:  // Explore current dir in new Explorer window
				{
                    // Build a command line
					char *szParm = "/select,";
					wsprintf(buffer, szParm);
					int len = strlen(szParm);
					LPARAM lpBuf = (LPARAM)buffer + (LPARAM)len;
					//dbg("CDM_GET..PATH, cbSize=%d, buffer = %p", MAX_PATH-len, lpBuf);
					len = SendMessage(hwnd, CDM_GETFOLDERPATH, MAX_PATH - len, lpBuf); //(LPARAM)(char *)((unsigned int)buffer + (unsigned int)len));
					if (len)
					{
                        // execute the command line
						ShellExecute(hwnd, NULL, "explorer.exe", buffer, NULL, SW_SHOWNORMAL);
					}
				} // case
					return 0;
/*
// Handling of favourites -- UNFININSHED
				case OW_ADDFAV_CMDID:
					doAddFave(hwnd);
					return 0;
				default:
					if (LOWORD(wp) >= OW_FAVOURITE_CMDID)
					{
						getFavourite(hwnd, (int)LOWORD(wp));
					}
					break;*/
			} // switch : command ids
			break;

        // Handle notifications from the toolbar
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

        // handle notifications from the dir listview control
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
					if( hwLV )
					{
						if( GetWindowLong(hwLV, GWL_STYLE) & LVS_REPORT ) // if details view is in effect.
						{
                            // update style flags
							ListView_SetExtendedListViewStyleEx(hwLV, OW_LISTVIEW_STYLE, OW_LISTVIEW_STYLE );
                            // Send Control+NumpadPlus to expand all the columns to fit contents
							INPUT in[4] = {0};
							in[0].type = INPUT_KEYBOARD;
							in[0].ki.wVk = VK_CONTROL;
							in[1].type = INPUT_KEYBOARD;
							in[1].ki.wVk = VK_ADD;
							in[2].type = INPUT_KEYBOARD;
							in[2].ki.wVk = VK_CONTROL;
							in[2].ki.dwFlags = KEYEVENTF_KEYUP;
							in[3].type = INPUT_KEYBOARD;
							in[3].ki.wVk = VK_ADD;
							in[3].ki.dwFlags = KEYEVENTF_KEYUP;
							HWND hwOld = SetFocus(hwLV);
							SendInput(4, in, sizeof(INPUT));
							SetFocus(hwOld);
						}
					}
                    //SetTimer(hwnd, 251177, 1, NULL);    // set a timer, for what? TODO:: Check this
				}
			}
			break;
/// Deprecated -- I think? TODO: Check
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
		case WM_SYSCOMMAND:     // Handle system menu commands
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
/*		case WM_NCPAINT:    // handle painting of non-content area, ie. window titlebar and frame.
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
		case WM_DROPFILES:  // Handle files which are dragged&dropped onto window.
			{
				HANDLE hDrop = (HANDLE)wp;
				int nFiles = DragQueryFile(hDrop, (UINT)-1, NULL, 0);
				if( nFiles == 1 )
				{
					if( DragQueryFile(hDrop, 0, buffer, MAX_PATH) )
					{
						if( PathIsDirectory(buffer) )
						{
                            // Set the view to dropped directory path.
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
					if( --gpSharedMem->refCount < 0 )   // Update the count for number of users of subclass code
						gpSharedMem->refCount = 0;
                    //dbg("DLL: dec'd refCount to %d, posting msg %x to app window %p", gpSharedMem->refCount,
                    //  gpSharedMem->iCloseMsg, gpSharedMem->hwListener);

                    // Notify application that another O&S dialog has closed
					PostMessage( gpSharedMem->hwListener, gpSharedMem->iCloseMsg, 0,0);
                    // Release any hold on the shared memory.
					closeSharedMem();
				}
                // Remove subclassing
				WNDPROC wpOrig = pow->wpOrig;
				unsubclass(hwnd);
                // Call original WM_DESTROY
				return CallWindowProc(wpOrig, hwnd, msg, wp, lp);
			}
			break;
	}
	return CallWindowProc(pow->wpOrig, hwnd, msg, wp, lp);
} // END wpSubMain


// Subclass the listview control -- TODO: I think this is un-needed
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
} // END wpSubShellCtl


/* Check whether this application has been excluded, ie, whether we should not
 * mess with its O&S dialogs
 */
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


/* 
 * Investigate the WM_CREATEPARAMS message
 */
static void	dbgCreateParams(LPVOID lpCreateParams)
{
	UNALIGNED short * pcbData = (UNALIGNED short *)lpCreateParams;
	if( pcbData )
	{
		short cbData = * pcbData;
		UNALIGNED byte *pbData = (UNALIGNED byte *)pcbData;
		pbData += sizeof(short);
		dbg("**CreateParams:");
		dbg("  %d bytes of data, starting at x%p", cbData, pbData);
		if( pbData )
		{
			dbg("  First 8 bytes follow:");
			int len = min( cbData, 8 );
			char 	s[32] = {0};
			int i;
			for(i=0; i < len; i++)
			{
				s[i] = pbData[i];
			}
			s[i] = 0;
			dbg("  \"%s\" (hex follows)", s);
			char st[8];
			s[0] = 0;
			for(i=0; i < len; i++)
			{
				sprintf(st, "%02x ", pbData[i]);
				strcat(s, st);
			}
			dbg("  %s", s);
		}
	}
	else
		dbg("CreateParams is NULL (%p)", pcbData);
}


/*
 * This is the main hook callback, which watches window creation, and
 * subclasses those which appear (with luck, correctly) to be O&S dialogs */
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
						//dbgCreateParams(pcs->lpCreateParams);
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
} // END CBTProc

/* 
 * Access to shared memory, and copy data to cache
 */
int	openSharedMem(void)
{
	if( ghMap != NULL || gpSharedMem != NULL )
		return 1;

	if(!waitForMutex())
		return 0;
	
	ghMap = OpenFileMapping(FILE_MAP_WRITE, FALSE, OW_SHARED_FILE_MAPPING);
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
} // END openSharedMem

/*
 * Release any access to shared memory we may hold.
 */
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
} // END closeSharedMem


/*
 * Simply refreshes our cache of the shared data.
 * Use this as a preference to the open/close-SharedMem functions.
 *
 * @returns 1 if successful, 0 on error.
 */
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


/*
 * exported function to setup the CBT hook, which watches window creation.
 */
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


/*
 * Exported function to remove the CBT hook.
 */
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
// This callback handles SYSMSG hooks, which I have used to investigate the O&S dialog behaviour.
//
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



/*
 * Main DLL entry point
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

