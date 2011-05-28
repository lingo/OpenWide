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


#ifndef	C__Data_Code_C_openwide_openwidedll_proto_h_H
/**
 * @author  Luke Hudson
 * @licence GPL2
 */

#define	C__Data_Code_C_openwide_openwidedll_proto_h_H
#pragma once

#include	<windows.h>

#define	DLLEXPORT __declspec(dllexport)

// Different view and focus options for the Open/Save dialogs
enum	ViewMode	{V_LGICONS, V_SMICONS, V_LIST, V_DETAILS, V_THUMBS, V_TILES, V_MAX};

enum	FocusMode	{F_DIRLIST, F_FNAME, F_FTYPE, F_PLACES, F_LOOKIN, F_MAX};

enum	CtrlIDs2k
{
	CID_DIRLIST = 0x1,
	CID_TOOLBAR = 0x1,
	CID_FNAME	= 0x47C,
	CID_FTYPE	= 0x470,
	CID_PLACES	= 0x4A0,
	CID_LOOKIN	= 0x471,
	CID_DIRLISTPARENT = 0x461,
};

enum	CommandIDs2k
{
	CMD_2K_LGICONS = 28713,
	CMD_2K_SMICONS = 28714,
	CMD_2K_LIST = 28715,
	CMD_2K_DETAILS = 28716,
	CMD_2K_THUMBS = 28721,
};

enum	CommandIDsXP
{
	CMD_XP_DETAILS = CMD_2K_DETAILS,//30978,
	CMD_XP_LIST = CMD_2K_LIST,//30979,
	CMD_XP_LGICONS = CMD_2K_LGICONS,//30980,
	CMD_XP_TILES = 28718,//30981,
	CMD_XP_THUMBS = 28717,//30982,
};


enum OW_COMMANDS {
	OW_ABOUT_CMDID = 0x1010,
	OW_EXPLORE_CMDID = 0x1020,
	OW_TBUTTON_CMDID = 0x1030,
	OW_ADDFAV_CMDID = 0x1040,
	OW_FAVOURITE_CMDID = 0x1050,
	OW_SHOWDESK_CMDID	= 0x1060,
};

#define PACKVERSION(major,minor) MAKELONG(minor,major)

#define	OW_MATCH_STYLE			0x82CC20C4
#define	OW_MATCH_STYLE_W7		0x82CC02C4
#define	OW_MATCH_EXSTYLE		0x00010501
#define	OW_MATCH_EXSTYLE_W7		0x00010101


#define	OW_SHARED_FILE_MAPPING	("openwidedll_shared_memfile")
#define	OW_MUTEX_NAME			("openwidedll_mem_mutex")
#define	OW_PROP_NAME			("openwidedll_window_property")
#define	OW_OVERLAY_CLASS		("openwidedll_overlay_window_class")

#define	OW_REGKEY_NAME	("Software\\Lingo\\OpenWide")
#define	OW_REGKEY_EXCLUDES_NAME	("Software\\Lingo\\OpenWide\\Excludes")

#define	OW_2K_MINWIDTH			565
#define	OW_2K_MINHEIGHT			349

#define	OW_XP_MINWIDTH			563
#define	OW_XP_MINHEIGHT			419

#define	OW_7_MINWIDTH			625
#define	OW_7_MINHEIGHT			434

#define	OW_LISTVIEW_STYLE	(LVS_EX_FULLROWSELECT)


// functions from file C:\Data\Code\C\openwide\openwidedll.c //
int rmvHook(void);
int setHook(void);

DWORD GetDllVersion(LPCTSTR lpszDllName);
BOOL isWinXP(void);






typedef struct OWSharedData
{
	HWND		hwListener;
	POINT		ptOrg;
	SIZE		szDim;
	
	int			iView;
	int			iFocus;

	BOOL		bStartMin : 1;
	BOOL		bDisable  : 1;
	BOOL		bShowIcon : 1;
	BOOL		bPadding  : 29;

//	HHOOK		hHook;
	int			refCount;
	int			iCloseMsg;
} OWSharedData, *POWSharedData;










#endif	// C__Data_Code_C_openwide_openwidedll_proto_h



