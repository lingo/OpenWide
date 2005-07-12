#ifndef	C__Data_Code_C_openwide_openwidedll_proto_h_H
#define	C__Data_Code_C_openwide_openwidedll_proto_h_H
#pragma once

#include	<windows.h>

#define	DLLEXPORT __declspec(dllexport)

enum	ViewMode	{V_LGICONS, V_SMICONS, V_LIST, V_DETAILS, V_THUMBS, V_TILES, V_MAX};

enum	FocusMode	{F_DIRLIST, F_FNAME, F_FTYPE, F_PLACES, F_LOOKIN, F_MAX};

enum	CtrlIDs2k
{
	CID_DIRLIST = 0x1,
	CID_FNAME	= 0x47C,
	CID_FTYPE	= 0x470,
	CID_PLACES	= 0x4A0,
	CID_LOOKIN	= 0x471,
	CID_DIRLISTPARENT = 0x461,
	CID_OVERLAY = 0x1234
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

#define PACKVERSION(major,minor) MAKELONG(minor,major)

#define	OW_MATCH_STYLE			0x82CC20C4
#define	OW_MATCH_EXSTYLE		0x00010501

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



