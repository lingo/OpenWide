#include	<windows.h>
/**
 * @author  Luke Hudson
 * @licence GPL2
 */

#include	<objbase.h>
#include	<shobjidl.h>
#include	<shlguid.h>
#include	<shlobj.h>
#include	<shlwapi.h>
#include	<shellapi.h>
#include	<stdio.h>
#include	"openwidedll.h"
//#include	"openwideres.h"
#include	"owSharedUtil.h"

/* Copied on : Tue Jul 12 18:59:49 2005 */
/** Function source : C:\Data\Code\C\openwide\owDLLUtil.c */
void dbg(char *szError, ...)
{
#ifdef DEBUG
	char		szBuff[256];
	va_list		vl;
	va_start(vl, szError);
    vsnprintf(szBuff, 256, szError, vl);	// print error message to string
	OutputDebugString(szBuff);
	va_end(vl);
#endif
}

/* Copied on : Wed Jul 13 23:03:36 2005 */
/** Function source : C:\Data\Code\C\Proto\gui.c */
char * getDlgItemText(HWND hwnd, UINT uID)
{
	HWND hwCtl = GetDlgItem(hwnd, uID);
	if(! IsWindow(hwCtl) )
		return NULL;
	int len = GetWindowTextLength(hwCtl);
	if( len <= 0 )
		return NULL;
	char * buf = malloc(len+1);
	if(!buf)
		return NULL;
	len = GetWindowText(hwCtl, buf, len+1);
	if( len > 0 )
		return buf;
	free(buf);
	return NULL;
}


/* Copied on : Tue Jul 12 18:59:04 2005 */
/** Function source : C:\Data\Code\C\openwide\owDLLUtil.c */
DWORD GetDllVersion(LPCTSTR lpszDllName)
{
    HINSTANCE hinstDll;
    DWORD dwVersion = 0;
	char	szDll[MAX_PATH+1];
    /* For security purposes, LoadLibrary should be provided with a
       fully-qualified path to the DLL. The lpszDllName variable should be
       tested to ensure that it is a fully qualified path before it is used. */
	if( !PathSearchAndQualify(lpszDllName, szDll, MAX_PATH) )
		return 0;
    hinstDll = LoadLibrary(szDll);

    if(hinstDll)
    {
        DLLGETVERSIONPROC pDllGetVersion;
        pDllGetVersion = (DLLGETVERSIONPROC)	GetProcAddress(hinstDll, "DllGetVersion");

        /* Because some DLLs might not implement this function, you
        must test for it explicitly. Depending on the particular
        DLL, the lack of a DllGetVersion function can be a useful
        indicator of the version. */

        if(pDllGetVersion)
        {
            DLLVERSIONINFO dvi;
            HRESULT hr;

            ZeroMemory(&dvi, sizeof(dvi));
            dvi.cbSize = sizeof(dvi);

            hr = (*pDllGetVersion)(&dvi);

            if(SUCCEEDED(hr))
            {
               dwVersion = PACKVERSION(dvi.dwMajorVersion, dvi.dwMinorVersion);
            }
        }

        FreeLibrary(hinstDll);
    }
    return dwVersion;
}

/** Function source : C:\Data\Code\C\openwide\owDLLUtil.c */
BOOL isWinXP(void)
{
	return GetDllVersion("Shell32.dll") >= PACKVERSION(6,00);
}



/* Copied on : Tue Jul 12 19:00:01 2005 */
/** Function source : C:\Data\Code\C\openwide\owUtil.c */
void regCloseKey(HKEY hk)
{
	RegCloseKey(hk);
}

/** Function source : C:\Data\Code\C\openwide\owUtil.c */
HKEY regCreateKey(HKEY hkParent, const char *szSubKey){
	LONG res;
	HKEY hk;
	res = RegCreateKeyEx(hkParent, szSubKey, 0L, NULL, REG_OPTION_NON_VOLATILE,
							KEY_READ | KEY_WRITE, NULL, &hk, NULL);
	return ( res == ERROR_SUCCESS ) ? hk : NULL;
}

/** Function source : C:\Data\Code\C\openwide\owUtil.c */
int regDeleteKey(HKEY hkParent, const char *szSubKey)
{
	DWORD rv;
	rv = ( SHDeleteKey(hkParent, szSubKey) == ERROR_SUCCESS );
	return rv;
}

/** Function source : C:\Data\Code\C\openwide\owUtil.c */
int regEnumValues(HKEY hkRoot, RegValEnumProc fp, LPVOID param)
{
	DWORD ccBuf = regGetMaxValueNameLength(hkRoot, NULL);
	if(!ccBuf) return 0;
	char *buf = malloc(ccBuf * sizeof(char));
	if(!buf) return 0;

	int i=0, res;
	LONG rCode;
	DWORD tmp;
	DWORD dwType;

	while(1)
	{
		tmp = ccBuf;
		rCode = RegEnumValue(hkRoot, i, buf, &tmp, NULL, &dwType, NULL, NULL );
		if(rCode != ERROR_SUCCESS)
		{
			if(rCode != ERROR_NO_MORE_ITEMS)
				Warn("Error code %d : \r\n%s\r\non calling RegEnumValue()", rCode, geterrmsg() );
			break;
		}
		res = fp(hkRoot, buf, dwType, param);
		if(!res)
			break;
		i++;
	}
	free(buf);
	return 1;
}

/** Function source : C:\Data\Code\C\openwide\owUtil.c */
DWORD regGetDWORD(HKEY hkRoot, const char *szValue, int *pSuccess){
	DWORD dwType, dwSize = 0;
	LONG res;
	DWORD dword = 0L;

	if( pSuccess )
		*pSuccess = 0;
	res = RegQueryValueEx(hkRoot, szValue, NULL, &dwType, NULL, &dwSize);
	if( res == ERROR_SUCCESS
		&& (dwType == REG_DWORD || REG_DWORD_LITTLE_ENDIAN)
	    && dwSize == sizeof(dword))
	{
		res = RegQueryValueEx(hkRoot, szValue, NULL, NULL, (BYTE*)&dword, &dwSize);
		if( res == ERROR_SUCCESS && pSuccess)
			*pSuccess = 1;
	}
	return dword;
}

/** Function source : C:\Data\Code\C\openwide\owUtil.c */
int regGetMaxValueNameLength(HKEY hk, LPDWORD pValueDataLen)
{
	DWORD dwNLen, dwDLen;

	if(!pValueDataLen)
		pValueDataLen = &dwDLen;

	if( RegQueryInfoKey(
			hk, NULL, NULL, NULL, NULL,
			NULL, NULL, NULL, &dwNLen, pValueDataLen,
			NULL, NULL)	== ERROR_SUCCESS )
		return ++dwNLen;
	return 0;
}

/** Function source : C:\Data\Code\C\openwide\owUtil.c */
HKEY regOpenKey(HKEY hkParent, const char *szSubKey){
	LONG res;
	HKEY hk;

	if(!szSubKey) return NULL;
	res = RegOpenKeyEx(hkParent, szSubKey, 0L, KEY_READ | KEY_WRITE, &hk);
	return ( res == ERROR_SUCCESS ) ? hk : NULL;
}

/** Function source : C:\Data\Code\C\openwide\owUtil.c */
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

/** Function source : C:\Data\Code\C\openwide\owUtil.c */
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

/** Function source : C:\Data\Code\C\openwide\owUtil.c */
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

/** Function source : C:\Data\Code\C\openwide\owUtil.c */
int regWriteBinaryData(HKEY hkRoot, const char *szValue, BYTE *buf, int bufSize ){
	LONG res;
	res = RegSetValueEx(hkRoot, szValue, 0L, REG_BINARY, buf, bufSize);
	return (res == ERROR_SUCCESS);
}

/** Function source : C:\Data\Code\C\openwide\owUtil.c */
int regWriteDWORD(HKEY hkRoot, const char *szValue, DWORD dwData){
	LONG res;
	res = RegSetValueEx(hkRoot, (LPCTSTR)szValue, 0L, REG_DWORD,
						(BYTE *)&dwData,
						sizeof(dwData));
	return (res == ERROR_SUCCESS);
}

/** Function source : C:\Data\Code\C\openwide\owUtil.c */
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

/* Copied on : Tue Jul 12 19:00:24 2005 */
/** Function source : C:\Data\Code\C\openwide\owUtil.c */
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

/* Copied on : Tue Jul 12 19:07:35 2005 */
/** Function source : C:\Data\Code\C\openwide\owUtil.c */
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

