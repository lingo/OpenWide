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


#ifndef	C__Data_Code_C_openwide_owSharedUtil_h
/**
 * @author  Luke Hudson
 * @licence GPL2
 */

#define	C__Data_Code_C_openwide_owSharedUtil_h
#pragma once

#include	<windows.h>

/* Prototype file C:\Data\Code\C\openwide\owSharedUtil.h *
 * generated by Pro2 from files:
 * C:\Data\Code\C\openwide\owSharedUtil.c
 * on Tuesday, 12 July 2005 */

typedef int (*RegValEnumProc)(HKEY hk, const TCHAR *keyName, DWORD dwType, LPVOID pparam);


void dbg(char *szError, ...);
char * getDlgItemText(HWND hwnd, UINT uID);
DWORD GetDllVersion(LPCTSTR lpszDllName);
const char * geterrmsg(void);
BOOL isWinXP(void);
BOOL DLLEXPORT isWin7(void);
void regCloseKey(HKEY hk);
HKEY regCreateKey(HKEY hkParent, const char *szSubKey);
int regDeleteKey(HKEY hkParent, const char *szSubKey);
int regEnumValues(HKEY hkRoot, RegValEnumProc fp, LPVOID param);
DWORD regGetDWORD(HKEY hkRoot, const char *szValue, int *pSuccess);
int regGetMaxValueNameLength(HKEY hk, LPDWORD pValueDataLen);
HKEY regOpenKey(HKEY hkParent, const char *szSubKey);
BYTE *regReadBinaryData(HKEY hkRoot, const char *szValueName);
DWORD regReadDWORD(HKEY hkRoot, const char *szValueName, int *pSuccess);
char *regReadSZ(HKEY hk, const char *szValueName);
int regWriteBinaryData(HKEY hkRoot, const char *szValue, BYTE *buf, int bufSize );
int regWriteDWORD(HKEY hkRoot, const char *szValue, DWORD dwData);
int regWriteSZ(HKEY hkRoot, const char *szValueName, const char *szData);
void Warn(char *szError, ...);

#endif	// C__Data_Code_C_openwide_owSharedUtil_h
