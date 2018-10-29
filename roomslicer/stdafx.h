// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#include <windows.h>

void Logger(char *fmt, ...);

#ifdef _DEBUG
#define DLOG		//Logger
#else
#define	DLOG
#endif


// TODO: reference additional headers your program requires here
typedef unsigned char	u8;
typedef unsigned short	u16;
typedef unsigned int	u32;
typedef signed char		s8;
typedef signed short	s16;
typedef signed int		s32;

#define NAKED	__declspec(naked)
