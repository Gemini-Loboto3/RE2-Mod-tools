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

#include "FileHandle.h"

#define NAKED	__declspec(naked)

class CBufferFile
{
public:
	CBufferFile() { data = NULL; size = 0; }
	~CBufferFile() { if (data) delete[] data; }

	bool Open(LPCSTR filename)
	{
		CFile f(filename);
		if (!f.IsOpen())
			return 0;

		size = f.GetSize();

		data = new u8[size];
		f.Read(data, size);
		return 1;
	}

	u8 *data;
	size_t size;
};
