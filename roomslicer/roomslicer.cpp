// roomslicer.cpp : Defines the entry point for the console application.
//
#define _CRT_SECURE_NO_WARNINGS

#include "stdafx.h"
#include "depack_adt.h"
#include <stdlib.h>
#include <stdio.h>
#include "lodepng.h"
#include "FileHandle.h"

class CBufferFile
{
public:
	CBufferFile() { data = NULL; size = 0; }
	~CBufferFile() { if (data) delete[] data; }

	void Open(LPCSTR filename)
	{
		CFile f(filename);
		if (f.IsOpen())
			return;

		size = f.GetSize();

		data = new u8[size];
		f.Read(data, size);
	}

	u8 *data;
	size_t size;
};

int is_dupe(CBufferFile &o, CBufferFile &n)
{
	// new file doesn't exist, ignore
	if (n.size == 0)
		return 0;
	// if the original doesn't exist either, there is a difference
	if (o.size == 0)
		return 1;

	// different filesize, must be different
	//if (o.size != n.size)
	//	return 1;

	// check shorter buffer and see if there are differences
	if (memcmp(o.data, n.data, o.size < n.size ? o.size : n.size) != 0)
		return 1;

	// size and contents match, ignore
	return 0;
}

void ck_dupe(LPCSTR stro, LPCSTR strm, LPCSTR strd)
{
	CBufferFile ino, inm;
	ino.Open(stro);
	inm.Open(strm);

	// non dupe found, write to non_dupe folder
	if (is_dupe(ino, inm))
	{
		FILE *ind = fopen(strd, "wb");
		fwrite(inm.data, inm.size, 1, ind);
		fclose(ind);
	}
}

void dump_new(LPCSTR original, LPCSTR modified, LPCSTR non_dupe)
{
	//FILE *ino, *inm;
	char stro[MAX_PATH], strm[MAX_PATH], strd[MAX_PATH];

	for (int stage = 0; stage < 7; stage++)
	{
		for (int room = 0; room < 32; room++)
		{
			for (int img = 0; img < 16; img++)
			{
				printf("Processing %X%02X_%02d\n", stage + 1, room, img);
				sprintf(stro, "%s\\ROOM_%X%02X_%02d.png", original, stage + 1, room, img);
				sprintf(strm, "%s\\ROOM_%X%02X_%02d.png", modified, stage + 1, room, img);
				sprintf(strd, "%s\\ROOM_%X%02X_%02d.png", non_dupe, stage + 1, room, img);
				ck_dupe(stro, strm, strd);
				sprintf(stro, "%s\\ROOM_%X%02X_%02d_mask.png", original, stage + 1, room, img);
				sprintf(strm, "%s\\ROOM_%X%02X_%02d_mask.png", modified, stage + 1, room, img);
				sprintf(strd, "%s\\ROOM_%X%02X_%02d_mask.png", non_dupe, stage + 1, room, img);
				ck_dupe(stro, strm, strd);
			}
		}
	}
}

void DecompileADT(LPCSTR in_file, LPCTSTR out_prefix)
{
	u8 *buffer;
	int size;
	MEM_STREAM in;
	char name[MAX_PATH];

	MemStreamOpen(&in, in_file);

	u32 magic;
	MemStreamRead(&in, &magic, 4);
	MemStreamSeek(&in, 0, SEEK_SET);

	adt_depack(&in, &buffer, &size);

	switch (magic)
	{
	case 0xfffd7fff:	// 320x240 background, arranged as 256x256 + 64x128x2
		{
			Image img;
			adt_surface((u16*)buffer, img);
			img.SavePng(out_prefix);
		}
		break;
	case 0xfffc7ddf:	// game background + mask
	case 0xfffdff8b:
		break;
	case 0xffff7fbb:	// ?? block of single TIM?
		break;
	case 0xfffefddf:	// raw TIM
	case 0xffffeddf:	// raw TIM, for shadow
	case 0xfffdffdc:	// raw TIM, font page 0
	case 0xfffdffeb:	// raw TIM, font page x
		{
			sprintf(name, "%s.png", out_prefix);
			Image img;
			Tim tim;
			tim.LoadTim(buffer);
			img.CreateFromTim(&tim, 0, 0);
			img.SavePng(name);
		}
		break;
	case 0xfffbf77f:	// TIM sequence
	case 0xfffe677f:
	case 0xfffd9f7f:
		{
			Tim tim;
			Image img;
			int seek = 0;
			for (int i = 0; seek < size; i++)
			{
				seek += tim.LoadTim(&buffer[seek]);
				img.CreateFromTim(&tim, 0, 0);
				sprintf(name, "%s_%02d.png", out_prefix, i);
				img.SavePng(name);
			}
		}
		break;
	}

	FILE *out = fopen("test.bin", "wb");
	fwrite(buffer, size, 1, out);
	fclose(out);

	free(buffer);
	MemStreamClose(&in);
}

void ListFiles(LPCSTR folder, LPCTSTR filter, std::vector<std::string> &names)
{
	HANDLE hFind = INVALID_HANDLE_VALUE;
	char szDir[MAX_PATH];
	sprintf(szDir, "%s\\%s", folder, filter);
	WIN32_FIND_DATA ffd;

	// find the first file in the directory
	hFind = FindFirstFile(szDir, &ffd);

	if (INVALID_HANDLE_VALUE == hFind)
		return;

	// list all mod files in the main folder
	do
	{
		if (!(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
		{
			sprintf(szDir, "%s\\%s", folder, ffd.cFileName);
			names.push_back(szDir);
		}
	} while (FindNextFile(hFind, &ffd) != 0);

	FindClose(hFind);
}

void _FixRdtText(LPCTSTR filename)
{
	MEM_STREAM f;
	MemStreamOpen(&f, filename);

	// patch text entries
	u32 ptr;
	MemStreamSeek(&f, 0x3C, SEEK_SET);
	MemStreamRead(&f, &ptr, sizeof(ptr));
	if (ptr != 0)
	{
		MemStreamWrite(&f, &ptr, sizeof(ptr));
		MemStreamFlush(&f, filename);
	}
	MemStreamClose(&f);
}

void FixRdtText(LPCTSTR folder)
{
	std::vector<std::string> names;
	ListFiles(folder, "*.rdt", names);

	for (size_t i = 0, si = names.size(); i < si; i++)
		_FixRdtText(names[i].c_str());
}

void DecompressFile(LPCSTR filename, LPCSTR outfolder)
{
	MEM_STREAM str;
	char name[MAX_PATH + 1];
	MemStreamOpen(&str, filename);

	CreateDirectory(outfolder, NULL);

	u32 *ptr = (u32*)str.data;

	u8 *out;
	int size;
	for (int i = 0; i < 32; i++)
	{
		// skip dummy pages
		if (ptr[i] == NULL && i > 0)
			continue;
		MemStreamSeek(&str, ptr[i] + 128, SEEK_SET);
		if (str.pos >= str.size)
			break;
		adt_depack(&str, &out, &size);

		Image img;
		Tim tim;
		tim.LoadTim(out);
		img.CreateFromTim(&tim, 0);
		sprintf(name, "%s\\%02d.png", outfolder, i);
		img.SavePng(name);

		delete[] out;
	}
}

int main()
{
	DecompressFile("D:\\Program Files\\BIOHAZARD 2 PC\\mod_mn1_de\\common\\file\\FILE03U.BIN", "D:\\Program Files\\BIOHAZARD 2 PC\\mod_mn1_de\\common\\file\\FILE03U");
	DecompressFile("D:\\Program Files\\BIOHAZARD 2 PC\\mod_mn1_de\\common\\file\\FILE04U.BIN", "D:\\Program Files\\BIOHAZARD 2 PC\\mod_mn1_de\\common\\file\\FILE04U");
	DecompressFile("D:\\Program Files\\BIOHAZARD 2 PC\\mod_mn1_de\\common\\file\\FILE09U.BIN", "D:\\Program Files\\BIOHAZARD 2 PC\\mod_mn1_de\\common\\file\\FILE09U");
	DecompressFile("D:\\Program Files\\BIOHAZARD 2 PC\\mod_mn1_de\\common\\file\\FILE15U.BIN", "D:\\Program Files\\BIOHAZARD 2 PC\\mod_mn1_de\\common\\file\\FILE15U");
	DecompressFile("D:\\Program Files\\BIOHAZARD 2 PC\\mod_mn1_de\\common\\file\\FILE18U.BIN", "D:\\Program Files\\BIOHAZARD 2 PC\\mod_mn1_de\\common\\file\\FILE18U");
	//FixRdtText("D:\\Program Files\\BIOHAZARD 2 PC\\mod_mn1_de\\pl0\\rdt");

	//DecompileADT("D:\\Program Files\\BIOHAZARD 2 PC\\common\\Data\\end00.adt", "end00");
	//DecompileADT("D:\\Program Files\\BIOHAZARD 2 PC\\common\\Data\\EXTITLE1.adt", "end00");
	//DecompileADT("D:\\Program Files\\BIOHAZARD 2 PC\\common\\Data\\font0.adt", "font0");
	//DecompileADT("D:\\Program Files\\BIOHAZARD 2 PC\\common\\Data\\kage.adt", "kage");
	//DecompileADT("D:\\Program Files\\BIOHAZARD 2 PC\\common\\bin\\pcstaff.adt", "pcstaff");
	//DecompileADT("D:\\Program Files\\BIOHAZARD 2 PC\\common\\data\\open14.adt", "open14.png");

#if 1
	//extract_roomcut("D:\\Program Files\\Merdal Night\\Resident Evil Mortal Night (D.E.) - EP1\\RE Mortal Night (DE) - EP1\\Common\\Bin\\roomcut.bin", "ROOM_merdal");
#else
	extract_roomcut("D:\\Program Files\\BIOHAZARD 2 PC\\common\\bin\\roomcut.bin", "ROOM");
#endif

	//dump_new("ROOM", "ROOM_merdal", "merda");

    return 0;
}

