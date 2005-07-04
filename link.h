#ifndef	C__Code_C_openwide_link_h_H
#define	C__Code_C_openwide_link_h_H
#pragma once

#include <windows.h>

#ifndef DLLEXPORT
#define	DLLEXPORT __declspec(dllexport)
#endif

typedef struct FavLink
{
	//struct FavLink * next;
	int			idCmd;
	int			iLen;
	char		szFav[MAX_PATH+4];
} FavLink, *PFavLink;

typedef int (*FPFaveIter)(PFavLink pFav, void * pData);

struct OWSharedData;
typedef struct OWSharedData OWSharedData, *POWSharedData;

#include	"link_proto.h"

#endif	// C__Code_C_openwide_link_h
