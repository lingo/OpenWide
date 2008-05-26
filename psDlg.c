#include	<windows.h>
/**
/**
 * @author  Luke Hudson
 * @licence GPL2
 */

 * @author  Luke Hudson
 * @licence GPL2
 */

#include	<shlwapi.h>
#include	"openwidedll.h"
#include	"openwideres.h"
#include	"owutil.h"
#include	"owSharedUtil.h"

#include	"openwide_proto.h"


#define	SETDLGRESULT(hw, x)	SetWindowLong((hw), DWL_MSGRESULT, (LONG)(x))
static int gIdxLastSheet = 0;


static void setTabChanged(HWND hwnd, BOOL bChanged)
{
	if( bChanged )
		PropSheet_Changed(ghwPropSheet, hwnd);
	else
		PropSheet_UnChanged(ghwPropSheet, hwnd);
}

int	savePrefsToRegistry(void)
{
	HKEY	hk = regCreateKey(HKEY_CURRENT_USER,  OW_REGKEY_NAME);
	if( hk )
	{
		regWriteDWORD(hk, NULL, sizeof(OWSharedData));
		regWriteBinaryData(hk, "OWData", (byte *)gPowData, sizeof(OWSharedData));
		regCloseKey(hk);
		return 1;
	}
	return 0;
}

/* Copied on : Mon Jul 11 22:27:04 2005 */
/** Function source : C:\Data\Code\C\openwide\openwide.c */
int initPrefs(HWND hwnd)
{
	if(!initSharedMem(NULL))
		return 0;

	CheckDlgButton(hwnd, IDC_WSTARTUP, isStartupApp(hwnd));
	CheckDlgButton(hwnd, IDC_LOG, BST_CHECKED);

	if( !waitForMutex() )
		return 0;

	CheckDlgButton(hwnd, IDC_STARTMIN, gPowData->bStartMin);
	CheckDlgButton(hwnd, IDC_SHOWICON, gPowData->bShowIcon);

	releaseMutex();

/*	if( bMinimize )
	{
		SendMessage(ghwMain, WM_SYSCOMMAND, SC_MINIMIZE, 0);
		SetFocus(NULL);
		EndTrayOperation();
	}
	else
	{
		SetFocus(GetDlgItem(hwnd, IDB_SETPLACEMENT));
		ShowWindow(hwnd, SW_SHOWNORMAL);
	}*/
	return 1;
}

static void onPrefsTabCommand(HWND hwnd, WPARAM wp, LPARAM lp)
{
	switch(LOWORD(wp))
	{
		case IDC_WSTARTUP:
		case IDC_STARTMIN:
		case IDC_SHOWICON:
			setTabChanged(hwnd, HIWORD(wp)==BN_CLICKED);
			break;
	}
}

static BOOL CALLBACK wpPrefs(HWND hwnd, UINT uMsg, WPARAM wp, LPARAM lp)
{
	switch (uMsg)
	{
		case WM_INITDIALOG:
			initPrefs(hwnd);
			break;
		case WM_COMMAND:
			onPrefsTabCommand(hwnd, wp, lp);
			break;
		case WM_NOTIFY:
			{
				LPNMHDR pnh = (LPNMHDR)lp;
				if( pnh->hwndFrom == ghwPropSheet )
				{
					switch(pnh->code)
					{
						case PSN_SETACTIVE:
							SETDLGRESULT(hwnd, FALSE);
							gIdxLastSheet = 1;
							return FALSE;
						case PSN_APPLY:
							{
								if(!waitForMutex())
								{
									//Warn("Couldn't get access to shared memory!");
									SETDLGRESULT(hwnd, PSNRET_INVALID);
									return FALSE;
								}
								
								gPowData->bStartMin = (IsDlgButtonChecked(hwnd, IDC_STARTMIN) == BST_CHECKED);
								BOOL bShowIcon = gPowData->bShowIcon = (IsDlgButtonChecked(hwnd, IDC_SHOWICON) == BST_CHECKED);
								BOOL bWinStart = IsDlgButtonChecked(hwnd, IDC_WSTARTUP) == BST_CHECKED;
							
								savePrefsToRegistry();
								
								releaseMutex();

								setStartupApp(hwnd, bWinStart);
								if( bShowIcon )
									addTrayIcon(ghwMain);
								else
									remTrayIcon(ghwMain);
								SETDLGRESULT(hwnd, PSNRET_NOERROR);
								return TRUE;
							}
							break;
					}
				}
			}
			break;
/*
		case WM_DESTROY:
			break;
		case WM_CLOSE:
			break;
*/
	}
	return 0;
}

static int enumExcludes(HKEY hkRoot, const char *szKey, DWORD dwType, LPVOID pParm)
{
	if( dwType == REG_DWORD )
		SendMessage((HWND)pParm, LB_ADDSTRING, 0, (LPARAM)szKey);
	return 1;
}

static int	addExcludes2LB(HWND hwnd, UINT uID)
{
	HWND hwLB = GetDlgItem(hwnd, uID);
	if( !hwLB )
		return 0;
	HKEY hk = regOpenKey(HKEY_CURRENT_USER, OW_REGKEY_EXCLUDES_NAME);
	if( hk )
	{
		regEnumValues(hk, enumExcludes, (LPVOID)hwLB);
		regCloseKey(hk);
		return 1;
	}
	return 0;
}

static int initExcludes(HWND hwnd)
{
	return addExcludes2LB(hwnd, IDL_EXCLUDES);
}

static int addExclude(HWND hwnd)
{
	const char *szEx = Prompt_File_Name(OPEN, hwnd, "Executable Files\0*.exe;*.com", "Choose program to exclude");
	if( szEx )
	{
//		PathStripPath((char *)szEx);
		int idx = SendDlgItemMessage(hwnd, IDL_EXCLUDES, LB_FINDSTRINGEXACT, -1, (LPARAM)szEx);
		if( idx != LB_ERR )
			Warn("This program is already excluded");
		else
			SendDlgItemMessage(hwnd, IDL_EXCLUDES, LB_ADDSTRING, 0, (LPARAM)szEx);
	}
	return 1;
}

static int rmvExclude(HWND hwnd)
{
	int idx = SendDlgItemMessage(hwnd, IDL_EXCLUDES, LB_GETCURSEL, 0,0);
	if( idx != LB_ERR )
		SendDlgItemMessage(hwnd, IDL_EXCLUDES, LB_DELETESTRING, idx, 0);
	return 1;
}

static int	saveExcludes2Registry(HWND hwnd, UINT uID)
{
	HWND hwLB = GetDlgItem(hwnd, uID);
	if( !hwLB )
		return 0;
	static char szKey[32];
	regDeleteKey(HKEY_CURRENT_USER, OW_REGKEY_EXCLUDES_NAME);
	HKEY hk = regCreateKey(HKEY_CURRENT_USER, OW_REGKEY_EXCLUDES_NAME);
	if( hk )
	{
		int num = SendMessage(hwLB, LB_GETCOUNT,0,0);
		if( num != LB_ERR )
		{
			for(int i=0; i < num; i++)
			{
				char *szEx = lbGetItemText(hwLB, i);
				if( szEx )
				{
					regWriteDWORD(hk, szEx, 1);
					free(szEx);
				}
			}
		}
		regCloseKey(hk);
		return 1;
	}
	return 0;
}

static void onExTabCmd(HWND hwnd, WPARAM wp, LPARAM lp)
{
	switch(LOWORD(wp))
	{
		case IDB_ADDEX:
			addExclude(hwnd);
			break;
		case IDB_RMVEX:
			rmvExclude(hwnd);
			break;
	}
}

static BOOL CALLBACK wpExcludes(HWND hwnd, UINT uMsg, WPARAM wp, LPARAM lp)
{
	switch (uMsg)
	{
		case WM_INITDIALOG:
			initExcludes(hwnd);
			break;
		case WM_COMMAND:
			onExTabCmd(hwnd, wp, lp);
			break;
		case WM_NOTIFY:
			{
				LPNMHDR pnh = (LPNMHDR)lp;
				if( pnh->hwndFrom == ghwPropSheet )
				{
					switch(pnh->code)
					{
						case PSN_SETACTIVE:
							SETDLGRESULT(hwnd, FALSE);
							gIdxLastSheet = 1;
							return FALSE;
						case PSN_APPLY:
							{
								saveExcludes2Registry(hwnd, IDL_EXCLUDES);
								SETDLGRESULT(hwnd, PSNRET_NOERROR);
								return TRUE;
							}
							break;
					}
				}
			}
			break;
	}
	return 0;
}



static void onOSTabCommand(HWND hwnd, WPARAM wp, LPARAM lp)
{
	switch(LOWORD(wp))
	{
		case IDCB_VIEW:
		case IDCB_FOCUS:
			if( HIWORD(wp) == CBN_SELCHANGE )
				setTabChanged(hwnd, TRUE);
			break;
		case IDE_LEFT:
		case IDE_TOP:
		case IDE_WIDTH:
		case IDE_HEIGHT:
			if( HIWORD(wp) == EN_CHANGE )
				setTabChanged(hwnd, TRUE);
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
					setTabChanged(hwnd, TRUE);
				}
			}
			break;
		case IDB_TEST:
			Prompt_File_Name(0, hwnd, NULL, NULL);
			break;
	}
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

static int initOSTab(HWND hwnd)
{
	CheckDlgButton(hwnd, IDC_WSTARTUP, isStartupApp(hwnd));
	CheckDlgButton(hwnd, IDC_LOG, BST_CHECKED);
	fillViewCB(hwnd, IDCB_VIEW);
	fillFocusCB(hwnd, IDCB_FOCUS);

	if(!waitForMutex())
		return 0;
//	bMinimize = gPowData->bStartMin;

	SetDlgItemInt(hwnd, IDE_LEFT, gPowData->ptOrg.x, TRUE);
	SetDlgItemInt(hwnd, IDE_TOP, gPowData->ptOrg.y, TRUE);
	SetDlgItemInt(hwnd, IDE_WIDTH, gPowData->szDim.cx, FALSE);
	SetDlgItemInt(hwnd, IDE_HEIGHT, gPowData->szDim.cy, FALSE);

	selectCBView(hwnd, IDCB_VIEW, gPowData->iView);
	SendDlgItemMessage(hwnd, IDCB_FOCUS, CB_SETCURSEL, gPowData->iFocus, 0);

/// released shared data
	releaseMutex();
	return 1;
}

static BOOL CALLBACK wpOSDlgs(HWND hwnd, UINT uMsg, WPARAM wp, LPARAM lp)
{
	switch (uMsg)
	{
		case WM_INITDIALOG:
			initOSTab(hwnd);
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
/*		case WM_SYSCOMMAND:
			if( wp == SC_MINIMIZE )
			{
				EnableWindow(hwnd, FALSE);
				ShowWindow(hwnd, SW_HIDE);
				addTrayIcon(hwnd);
				EndTrayOperation();
			}
			else
				return DefWindowProc(hwnd, uMsg, wp, lp);
			break;*/
		case WM_COMMAND:
			onOSTabCommand(hwnd, wp, lp);
			break;
		case WM_NOTIFY:
			{
				LPNMHDR pnh = (LPNMHDR)lp;
				if( pnh->hwndFrom == ghwPropSheet )
				{
					switch(pnh->code)
					{
						case PSN_SETACTIVE:
							SETDLGRESULT(hwnd, FALSE);
							gIdxLastSheet = 1;
							return FALSE;
						case PSN_APPLY:
							{
								if(!waitForMutex())
								{
									//Warn("Couldn't get access to shared memory!");
									SETDLGRESULT(hwnd, PSNRET_INVALID);
									return FALSE;
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

								savePrefsToRegistry();
								
								releaseMutex();
								SETDLGRESULT(hwnd, PSNRET_NOERROR);
								return TRUE;
							}
							break;
					}
				}
			}
			break;
	}
	return 0;
}


static BOOL CALLBACK wpApps(HWND hwnd, UINT uMsg, WPARAM wp, LPARAM lp)
{
	switch (uMsg)
	{
		case WM_INITDIALOG:
			break;
		case WM_NOTIFY:
			{
				LPNMHDR pnh = (LPNMHDR)lp;
				if( pnh->hwndFrom == ghwPropSheet )
				{
					switch(pnh->code)
					{
						case PSN_SETACTIVE:
							SETDLGRESULT(hwnd, FALSE);
							gIdxLastSheet = 1;
							return FALSE;
						case PSN_APPLY:
							{
								SETDLGRESULT(hwnd, PSNRET_NOERROR);
								return TRUE;
							}
							break;
					}
				}
			}
			break;
	}
	return 0;
}



/* Copied on : Tue Jul 05 19:21:25 2005 */
/** Function source : C:\Data\Code\C\Proto\prefsdlg.c */
static void fillSheet(PROPSHEETPAGE * psp, int idDlg, DLGPROC pfnDlgProc)
{
	memset(psp, 0, sizeof(PROPSHEETPAGE));
	psp->dwSize = sizeof(PROPSHEETPAGE);
	psp->dwFlags = PSP_DEFAULT;	// | PSP_USEICONID;
	psp->hInstance = ghInstance;
	psp->pszTemplate = MAKEINTRESOURCE(idDlg);
	//psp->pszIcon = (LPSTR) MAKEINTRESOURCE(IDI_DOC);
	psp->pfnDlgProc = pfnDlgProc;
	psp->lParam = 0;
}


/* Copied on : Tue Jul 05 19:21:08 2005 */
/** Function source : C:\Data\Code\C\Proto\prefsdlg.c */
int CALLBACK WINAPI initPropSheets(HWND hwnd, UINT msg, LPARAM lp)
{
	//dbg("initPropSheets: %p, %d", hwnd, msg);
	if (msg != PSCB_INITIALIZED)
		return 0;
	ghwPropSheet = hwnd;
	PropSheet_SetCurSel(ghwPropSheet, gIdxLastSheet, gIdxLastSheet);
	return 1;
}


HWND showDlg(HWND hwParent)
{
	PROPSHEETHEADER psh;
	PROPSHEETPAGE pages[4];

	int k=0;
	fillSheet(&pages[k++], IDD_OSDIALOGS, wpOSDlgs);
	fillSheet(&pages[k++], IDD_EXCLUDES, wpExcludes);
//	fillSheet(&pages[k++], IDD_APPS, wpApps);
	fillSheet(&pages[k++], IDD_PREFS, wpPrefs);

	memset(&psh, 0, sizeof(PROPSHEETHEADER));
	psh.dwSize = sizeof(PROPSHEETHEADER);
	psh.dwFlags = PSH_PROPSHEETPAGE | PSH_USEHICON | PSH_USECALLBACK | PSH_MODELESS;	// |
																		// PSH_NOAPPLYNOW
																		// //
																		// |
																		// PSH_USECALLBACK;
	psh.hwndParent = hwParent;
	psh.hInstance = ghInstance;
	psh.hIcon = 	ghIconSm;
	psh.pszCaption = (LPSTR) "OpenWide Settings";
	psh.nPages = k;	//sizeof(pages) / sizeof(PROPSHEETPAGE);
	psh.ppsp = (LPCPROPSHEETPAGE) pages;
	psh.pfnCallback = initPropSheets;
	psh.nStartPage = 0;

	return (HWND)PropertySheet(&psh);
}

