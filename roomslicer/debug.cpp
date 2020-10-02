#define _CRT_SECURE_NO_WARNINGS
#include <stdafx.h>

typedef struct tagRoomJump
{
	u16 Be_flg;
	s16 X, Z,
		nFloor;
	char Name[14];
	char dm01[4];
} ROOM_JUMP;

void Dump_debug_names(LPCSTR filename, LPCSTR outname)
{
	CBufferFile f;
	FILE *log = fopen(outname, "wb+");
	f.Open(filename);

	ROOM_JUMP *jmp = (ROOM_JUMP*)&f.data[0x7ef8c];
	for (int i = 0; i < 343; i++)
	{
		fprintf(log, "\t{ %d, 0, %2d, %6d, %6d, \"%s\" },\r\n", jmp->Be_flg, jmp->nFloor, jmp->X, jmp->Z, jmp->Name);
		jmp++;
	}

	fclose(log);
}