#include <windows.h>
#include <commctrl.h>
#include	<shlwapi.h>
#include	<shellapi.h>
#include	<stdarg.h>
#include	<stdio.h>
#include	<dlgs.h>
#include	"openwidedll.h"
#include	"openwideres.h"
#include	"owDllInc.h"


/* Copied on : Tue Jul 12 04:04:38 2005 */
/** Function source : C:\Data\Code\C\openwide\openwidedll.c */
int focusDlgItem(HWND hwnd, int iFocus)
{
	UINT uID = focusToCtlID(gOwShared.iFocus);
	if( uID == CID_DIRLIST )
	{
		return SetFocus( GetDlgItem( GetDlgItem(hwnd, CID_DIRLISTPARENT) , uID) ) != NULL;
	}
	else
		return SetFocus( GetDlgItem(hwnd, uID)) != NULL;
}

/** Function source : C:\Data\Code\C\openwide\openwidedll.c */
WORD	focusToCtlID(int	iFocus)
{
	//BOOL bXP = isWinXP();
	switch( iFocus )
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

/** Function source : C:\Data\Code\C\openwide\openwidedll.c */
HWND 	getChildWinFromPt(HWND hwnd)
{
	POINT pt;
	GetCursorPos(&pt);
	ScreenToClient(GetParent(hwnd), &pt);

	EnableWindow(hwnd, FALSE);
	HWND hw = ChildWindowFromPointEx(GetParent(hwnd), pt, CWP_SKIPDISABLED | CWP_SKIPINVISIBLE);
	EnableWindow(hwnd, TRUE);
	HWND hwSV = GetDlgItem(GetParent(hwnd), CID_DIRLISTPARENT);
	if( hw == hwSV )
	{
		ClientToScreen(GetParent(hwnd), &pt);
		ScreenToClient(hw, &pt);
		hw = ChildWindowFromPointEx(hw, pt, CWP_SKIPDISABLED | CWP_SKIPINVISIBLE);
	}
	HWND hwEd = GetDlgItem(GetParent(hwnd), CID_FNAME);
	if( hw == hwEd )
	{
		ClientToScreen(GetParent(hwnd), &pt);
		ScreenToClient(hw, &pt);
		hw = ChildWindowFromPointEx(hw, pt, CWP_SKIPDISABLED | CWP_SKIPINVISIBLE);
	}
	return hw;
}


/** Function source : C:\Data\Code\C\openwide\openwidedll.c */
int	subclass(HWND hwnd, WNDPROC wpNew, LPARAM lpData)
{
	if( GetProp(hwnd, OW_PROP_NAME) != NULL )
		return 0;
	POWSubClassData pow = (POWSubClassData)malloc(sizeof(OWSubClassData));
	if(!pow)
		return 0;
	ZeroMemory(pow, sizeof(OWSubClassData));
	if( !SetProp(hwnd, OW_PROP_NAME, pow) )
	{
		free(pow);
		return 0;
	}
	pow->lpData = lpData;
	pow->wpOrig = (WNDPROC)SetWindowLong(hwnd, GWL_WNDPROC, (LONG)wpNew);
	return 1;
}

/** Function source : C:\Data\Code\C\openwide\openwidedll.c */
int	unsubclass(HWND hwnd)
{
	POWSubClassData pow = (POWSubClassData)GetProp(hwnd, OW_PROP_NAME);
	if(pow)
	{
		SetWindowLong(hwnd, GWL_WNDPROC, (LONG)pow->wpOrig);
		RemoveProp(hwnd, OW_PROP_NAME);
		free(pow);
		return 1;
	}
	return 0;
}

/** Function source : C:\Data\Code\C\openwide\openwidedll.c */
WORD	viewToCmdID(int	iView)
{
	BOOL bXP = isWinXP();
	switch( iView )
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

/* Copied on : Tue Jul 12 02:29:56 2005 */
/** Function source : C:\Data\Code\C\openwide\psDlg.c */
BOOL	waitForMutex(void)
{
	DWORD dwRes;
	if( !ghMutex )
		ghMutex = OpenMutex(SYNCHRONIZE, FALSE, OW_MUTEX_NAME);
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
		ghMutex = NULL;
	}
}


BOOL CALLBACK fpEnumChildren(HWND hwnd, LPARAM lParam)
{
	static char buf[32];
	if( GetClassName(hwnd, buf, 31) )
	{
		switch(gOwShared.iFocus)
		{
			case F_DIRLIST:
				if( strcmp(buf, WC_LISTVIEWA) == 0 && GetDlgCtrlID(hwnd) == CID_DIRLIST)
					SetFocus(hwnd);
				break;
			case F_FNAME:
				if( strcmp(buf, WC_COMBOBOXEXA) == 0 && GetDlgCtrlID(hwnd) == CID_FNAME)
					SetFocus(hwnd);
				break;
			case F_FTYPE:
				if( strcmp(buf, WC_COMBOBOXA) == 0 && GetDlgCtrlID(hwnd) == CID_FTYPE)
					SetFocus(hwnd);
				break;
			case F_PLACES:
				if( IsWindowVisible(hwnd) && strcmp(buf, TOOLBARCLASSNAMEA) == 0 && GetDlgCtrlID(hwnd) == CID_PLACES)
					SetFocus(hwnd);
				break;
			case F_LOOKIN:
				if( strcmp(buf, WC_COMBOBOXA) == 0 && GetDlgCtrlID(hwnd) == CID_LOOKIN)
					SetFocus(hwnd);
				break;
		}
	}
/*	if( GetDlgCtrlID(hwnd) == 0x47c )
	{
		SetWindowText(hwnd, "C:\\code\\");
		SendMessage((HWND)lParam, WM_COMMAND, MAKEWPARAM(1, BN_CLICKED), (LPARAM)GetDlgItem((HWND)lParam, 1));
	}*/
	return 1;
}
