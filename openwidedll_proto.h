#ifndef	C__Data_Code_C_openwide_openwidedll_proto_h_H
#define	C__Data_Code_C_openwide_openwidedll_proto_h_H
#pragma once

#include	<windows.h>

#define	DLLEXPORT __declspec(dllexport)

// functions from file C:\Data\Code\C\openwide\openwidedll.c //
int DLLEXPORT rmvHook(void);
int DLLEXPORT setHook(HWND hwLB);

#endif	// C__Data_Code_C_openwide_openwidedll_proto_h
