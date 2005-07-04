#include <windows.h>
#include <commctrl.h>
#include	<shlwapi.h>
#include <stdlib.h>
#include	<shellapi.h>
#include	<assert.h>
#include	"owdll.h"
#include "link.h"
#include	"openwidedll.h"
#include	"winutil.h"
//#include <stdio.h>

extern HINSTANCE ghInstance;
static int giPlacesIcon = 0;

/* Copied on : Fri May 13 14:37:57 2005
 */
static int findInsert(POWSharedData pow, const char *szTxt);


int DLLEXPORT lockFaves(POWSharedData pow)
{
	if( pow->pFaves )
		return 1;
	pow->hFavMap = OpenFileMapping( FILE_MAP_ALL_ACCESS, FALSE, OW_FAVES_FILE_MAPPING);
	if(!pow->hFavMap)
	{
		dbg("%s, failed to open named mapping: %s", __func__, geterrmsg());
		return 0;
	}
	pow->pFaves = (PFavLink)MapViewOfFile(pow->hFavMap, FILE_MAP_ALL_ACCESS, 0,0,0);
	if(!pow->pFaves)
	{
		dbg("%s, failed to map view: %s", __func__, geterrmsg());
		CloseHandle(pow->hFavMap);
		pow->hFavMap = NULL;
		return 0;
	}
	return (pow->pFaves != NULL);
}

void DLLEXPORT unlockFaves(POWSharedData pow)
{
	if(pow->pFaves)
	{
		UnmapViewOfFile(pow->pFaves);
		pow->pFaves = NULL;
	}
	if( pow->hFavMap )
	{
		CloseHandle(pow->hFavMap);
		pow->hFavMap = NULL;
	}
}

int	DLLEXPORT expandFaves(POWSharedData pow, int nFaves)
{
	int iDesired = (pow->nFaves + nFaves) * sizeof(FavLink);
	if( pow->dwFaveSize < iDesired )
	{
		unlockFaves(pow);
		if( pow->hRootMap )
		{
			dbg("%s, closing root map handle", __func__);
			CloseHandle(pow->hRootMap);
			pow->hRootMap = NULL;
		}
		if( pow->hFavFile != INVALID_HANDLE_VALUE )
		{
			dbg("%s, increasing size by %lu bytes", __func__, iDesired - pow->dwFaveSize);
			pow->dwFaveSize = iDesired;
			pow->hRootMap = CreateFileMapping(pow->hFavFile, NULL, PAGE_READWRITE, 0, pow->dwFaveSize, OW_FAVES_FILE_MAPPING);
			if( pow->hRootMap == NULL )
				dbg("%s, error re-mapping: %s", __func__, geterrmsg());
			else
			{
				dbg("%s, created new mapping: %p", __func__, pow->hRootMap);			
				return lockFaves(pow);
			}
		}
		return 0;
	}
	return 1;
}

int		iterateFaves(POWSharedData pow, FPFaveIter f, void * pData)
{
	int n = 0;
	for(int i=0; i < pow->nFaves; i++)
	{
		PFavLink pFav = &pow->pFaves[i];
		if( !pFav )
			return -1;
		++n;
		if( ! f(pFav, pData) )
			break;
	}
	return n;
}

PFavLink	getNthFave(POWSharedData pow, int n)
{
	if( n < 0 || n >= pow->nFaves )
		return NULL;
	return &pow->pFaves[n];
}

void	dbgLink(const char *szMsg, PFavLink plk)
{
	if( plk )
	{
		dbg("%s: <FavLink> %p:", szMsg, plk);
		dbg("	->szFav:	%p, [%s]", plk->szFav, plk->szFav);
		dbg("	->idCmd:	%d", plk->idCmd);
	}
	else
		dbg("%s: <NULL FavLink>", szMsg);
}

static int	findInsert(POWSharedData pow, const char *szTxt)
{
	PFavLink pFav;
	for(int i=0; i < pow->nFaves; i++)
	{
		pFav = &pow->pFaves[i];
		if( pFav )
		{
			if( lstrcmpi(pFav->szFav, szTxt) >= 0 )
				return max(0, i-1);
		}
	}
	return pow->nFaves;
}

PFavLink DLLEXPORT	newFav(POWSharedData pow, const char *szTxt)
{
	static int iCmd = OW_FAVOURITE_CMDID;
	if( !szTxt )
	{
		dbg("%s, NULL text passed to newFav", __func__);
		return NULL;
	}

	int len = lstrlen(szTxt);
	if(!len)
	{
		dbg("%s, text with len 0 passed to newFav", __func__);
		return NULL;
	}

	if( !expandFaves(pow, 1) )
	{
		dbg("%s, failed to expandFaves", __func__);
		return NULL;
	}
	dbg("%s, fave array starting %p", __func__, pow->pFaves);

		
/*	PFavLink plink = allocFave(pow, szTxt);//GlobalAlloc(GPTR, sizeof(FavLink) + len + 1);
	if(!plink)
		return NULL;
	dbg("%s, got ptr to new fave: %p", __func__, plink); */

	int idx = findInsert(pow, szTxt);
	
	dbg("%s, insert point for \"%s\" is at %d, (arraysize is %d)", __func__, szTxt, idx, pow->nFaves);

	if( idx < 0 )
		idx = 0;
	else
	{
		for(int i=pow->nFaves-1; i>idx; i--)
		{
			dbg("%s, moving fave %d to %d", __func__, i, i-1);
			CopyMemory( &pow->pFaves[i], &pow->pFaves[i-1], sizeof(FavLink));
		}
	}
	PFavLink plink = &pow->pFaves[idx];
	dbg("%s, fave addr is %p, (from array starting %p)", __func__, plink, pow->pFaves);

	lstrcpy(plink->szFav, szTxt);
	plink->idCmd = iCmd + pow->nFaves + 1;
	plink->iLen = len;
	dbg("%s:	plink->idCmd == %d", __func__, plink->idCmd);
	pow->nFaves++;
	
	//dbgLink(__func__, plink);
	return plink;
}

int iterAddPlace(PFavLink pFav, HWND hwnd)
{
	dbg("%s, adding favourite %p {%s}", __func__, pFav, (pFav ? pFav->szFav : "<NULL>"));
	addPlace(hwnd, pFav);
	return 1;
}

int initPlaces(HWND hwnd)
{
	HWND hwTB = GetDlgItem(hwnd, CID_PLACES);
	if(!hwTB)
		return 0;

	int idxNew = -1;
	HICON hIcn = (HICON)LoadImage(ghInstance, MAKEINTRESOURCE(IDI_TBICON), IMAGE_ICON, 32, 32, 0);
	if( hIcn )
	{
		idxNew = addIcon2TB(hwTB, hIcn);
		DestroyIcon(hIcn);
	}
	giPlacesIcon = idxNew;

	POWSharedData pow = lockSharedData();
	if( pow )
	{
		iterateFaves(pow, (FPFaveIter)iterAddPlace, hwnd);
		unlockSharedData(pow);
	}
	return 1;
}


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
}

int	addFavourite(HWND hwnd, POWSharedData pow)
{
	//static char szBuf[2*MAX_PATH+1];
	assert( pow && IsWindow(hwnd));

	char *szBuf = (char *)GlobalAlloc(GPTR, 2 * MAX_PATH + 1);
	if( !szBuf )
		return 0;
	if( SendMessage(hwnd, CDM_GETFOLDERPATH, 2*MAX_PATH, (LPARAM)szBuf) > 0 )
	{		
		if( !faveExists(pow, szBuf) )
		{
			PFavLink plNew = newFav(pow, szBuf);
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


PFavLink	findFaveByPath(POWSharedData pow, const char *szFave)
{
	PFavLink pFav;
	for(int i=0; i < pow->nFaves; i++)
	{
		pFav = getNthFave(pow, i);		
		if( pFav )
		{ 
			if(lstrcmpi(pFav->szFav, szFave) == 0 )
				return pFav;
		}
	}
	return NULL;
}


PFavLink	findFaveByCmdID(POWSharedData pow, UINT uID)
{
	PFavLink pFav;
	for(int i=0; i < pow->nFaves; i++)
	{
		pFav = getNthFave(pow, i);		
		if( pFav )
		{
			if( pFav->idCmd == uID)
				return pFav;
		}
		else
		{
			dbg("%s:	got NULL from getNthFave(%d)", __func__, i);
			break;
		}
	}
	return NULL;
}


int	DLLEXPORT	faveExists(POWSharedData pow, const char *szFave)
{
	return findFaveByPath(pow, szFave) != NULL;
}
