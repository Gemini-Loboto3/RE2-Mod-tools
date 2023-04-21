// roomslicer.cpp : Defines the entry point for the console application.
//
#define _CRT_SECURE_NO_WARNINGS

#include "stdafx.h"
#include "depack_adt.h"
#include <stdlib.h>
#include <stdio.h>
#include "lodepng.h"
#include "FileHandle.h"
#include "Bitmap.h"
#include "tinyxml2.h"
#include "Room.h"

using namespace tinyxml2;

void Dump_debug_names(LPCSTR filename, LPCSTR outname);

int is_dupe(CBufferFile &o, CBufferFile &n)
{
	// new file doesn't exist, ignore
	if (n.size == 0)
		return 0;
	// if the original doesn't exist either, there is a difference
	if (o.size == 0)
		return 1;

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

void Bilinear_scale(CBitmap* src, CBitmap* dst, float scalex, float scaley);

void DecompileADT(LPCSTR in_file, LPCTSTR out_prefix)
{
	u8 *buffer;
	int size;
	MEM_STREAM in;
	char name[MAX_PATH];

	if (MemStreamOpen(&in, in_file) == nullptr)
		return;

	u32 magic;
	MemStreamRead(&in, &magic, 4);
	MemStreamSeek(&in, 0, SEEK_SET);

	adt_depack(&in, &buffer, &size);

	switch (magic)
	{
	case 0xfff69feb:	// RE3
		{
			CBitmap bmp, dst;
			bmp.CreateFromTim(buffer, 1);
			sprintf(name, "%s.png", out_prefix);
#if 1
			bmp.SavePng(name, true);
#else
			Bilinear_scale(&bmp, &dst, .5f, .5f);
			dst.SavePng(name, true);
#endif
		}
		break;
	case 0xfffdff8b:	// tex216.adt & tex316.adt
		{
			CBitmap bmp;
			bmp.CreateFromTim(buffer, 1);
			sprintf(name, "%s.png", out_prefix);
			bmp.SavePng(name);
			sprintf(name, "%s.bin", out_prefix);
			FILE *out = fopen(name, "wb");
			fwrite(&buffer[0x20014], 96, 1, out);
			fclose(out);
		}
		break;
	case 0xfffd7fff:	// 320x240 background, arranged as 256x256 + 64x128x2
		{
			Image img;
			adt_surface((u16*)buffer, img);
			img.SavePng(out_prefix);
		}
		break;
	case 0xfffc7ddf:	// game background + mask
	//case 0xfffdff8b:
		break;
	case 0xffff7fbb:	// ?? block of single TIM?
		break;
	case 0xfffefddf:	// raw TIM
	case 0xffffeddf:	// raw TIM, for shadow
	case 0xfffdffdc:	// raw TIM, font page 0
	case 0xfffdffeb:	// raw TIM, font page x
	case 0x3fb33713:	// raw TIM, font page x
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

void DecompileItemITP(LPCSTR filename, LPCSTR folder)
{
	MEM_STREAM str;
	MemStreamOpen(&str, filename);

	u32 *ptr = (u32*)str.data;
	size_t count = ptr[0] / 4;

	char out_name[MAX_PATH + 1];

	for (size_t i = 0; i < count; i++)
	{
		u8 *out;
		int size;
		MemStreamSeek(&str, ptr[i], SEEK_SET);
		adt_depack(&str, &out, &size);

		Tim tim;
		tim.LoadTim(out);

		Image img;
		img.CreateFromTim(&tim, 0);

		sprintf_s(out_name, sizeof(out_name), "%s\\item_%03d.png", folder, i);
		img.SavePng(out_name);

		delete[] out;
	}
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

#define ID_TO_ROOM_STAGE(i)	(i / 32) + 1
#define ID_TO_ROOM_ROOM(i)	(i % 32)

void ExtractEspdata(LPCSTR filename, LPCSTR outname)
{
	MEM_STREAM str;
	MemStreamOpen(&str, filename);

	u32 ptr[448][8];
	MemStreamRead(&str, ptr, sizeof(ptr));
	char name[MAX_PATH];

	int count = 0;
	Set_color_mode(0);

	for (int i = 0; i < 448; i++)
	{
		for (int j = 0; j < 8; j++)
		{
			// skip invalid entries
			if (ptr[i][j] == 0) continue;

			u8 *outbuf;
			int size;
			MemStreamSeek(&str, ptr[i][j], SEEK_SET);
			adt_depack(&str, &outbuf, &size);

			CBitmap tim;
			tim.CreateFromTim(outbuf, 1);

			sprintf(name, "%s\\esp_%X%02X_%02d.bin", outname, ID_TO_ROOM_STAGE(i), ID_TO_ROOM_ROOM(i), j);
			FILE *bin = fopen(name, "wb+");
			fwrite(&outbuf[131092], 96, 1, bin);
			fclose(bin);

			sprintf(name, "%s\\esp_%X%02X_%02d.png", outname, ID_TO_ROOM_STAGE(i), ID_TO_ROOM_ROOM(i), j);
			tim.SavePng(name);

			delete[] outbuf;
			count++;
		}
	}

	printf("Extracted %d sprites\n", count);

	MemStreamClose(&str);
}

typedef struct tagAttackWeaponEm
{
	u32 Damage, SubDamage;
	s16 Jyo_ue, Jyo_ge;
	s16 Tyu_ue, Tyu_ge;
	s16 Ge_ue, Ge_ge;
} ATTACK_WEAPON_EM;		// .111fake

void DumpDamageTable(ATTACK_WEAPON_EM *pTbl, LPCSTR outname)
{
	LPCSTR weapon_names[]=
	{
		"Knife",
		"H&K VP70",
		"Browning HP",
		"Custom Handgun",
		"Magnum",
		"Custom Magnum",
		"Shotgun",
		"Custom Shotgun",
		"Grenade Launcher (Explosive)",
		"Grenade Launcher (Incendiary)",
		"Grenade Launcher (Acid)",
		"Bow Gun",
		"Colt S.A.A.",
		"Spark Shot",
		"Sub Machine Gun",
		"Flame Thrower",
		"Rocket Launcher",
		"Gatling Gun",
		"Beretta"
	};

	XMLDocument xml;
	xml.SetBOM(1);
	auto root = xml.NewElement("Damage");

	for (int i = 0; i < 19; i++)
	{
		//xml.NewComment("test");
		auto e = xml.NewElement("Entry");

		auto s = xml.NewElement("Close");
		s->SetAttribute("Dmg", pTbl[i].Damage & 0x3FF);
		s->SetAttribute("Sub", pTbl[i].SubDamage & 7);
		s->SetAttribute("Timer", (pTbl[i].SubDamage >> 9) & 0x3F);
		s->SetAttribute("Max", pTbl[i].Jyo_ue);
		s->SetAttribute("Min", pTbl[i].Jyo_ge);
		e->InsertEndChild(s);

		s = xml.NewElement("Mid");
		s->SetAttribute("Dmg", (pTbl[i].Damage >> 10) & 0x3FF);
		s->SetAttribute("Sub", (pTbl[i].SubDamage >> 3) & 7);
		s->SetAttribute("Timer", (pTbl[i].SubDamage >> 16) & 0x3F);
		s->SetAttribute("Max", pTbl[i].Tyu_ue);
		s->SetAttribute("Min", pTbl[i].Tyu_ge);
		e->InsertEndChild(s);

		s = xml.NewElement("Far");
		s->SetAttribute("Dmg", (pTbl[i].Damage >> 20) & 0x3FF);
		s->SetAttribute("Sub", (pTbl[i].SubDamage >> 6) & 7);
		s->SetAttribute("Timer", (pTbl[i].SubDamage >> 23) & 0x3F);
		s->SetAttribute("Max", pTbl[i].Ge_ue);
		s->SetAttribute("Min", pTbl[i].Ge_ge);
		e->InsertEndChild(s);

		root->InsertEndChild(xml.NewComment(weapon_names[i]));
		root->InsertEndChild(e);
	}

	xml.InsertEndChild(root);
	xml.SaveFile(outname);
}

// Sourcenext version
void ExtractDamageTables(LPCSTR exe_name, LPCSTR folder_out)
{
	CFile f;
	f.Open(exe_name);

	u8 *exe = new u8[f.GetSize()];
	f.Read(exe, f.GetSize());
	f.Close();

	u32 *ptr = (u32*)&exe[0x539F90 - 0x400000];
	char path[MAX_PATH];

	// 24*2*2 tables
	for (int i = 0; i < 96; i++)
	{
		sprintf_s(path, sizeof(path), "%s\\wp_dmg_em%02X.xml", folder_out, i + 0x10);
		DumpDamageTable((ATTACK_WEAPON_EM*)&exe[ptr[i] - 0x400000], path);
	}

	delete[] exe;
}

// Platinum version
void ExtractDamageTablesPlatinum(LPCSTR exe_name, LPCSTR folder_out)
{
	CFile f;
	f.Open(exe_name);

	u8 *exe = new u8[f.GetSize()];
	f.Read(exe, f.GetSize());
	f.Close();

	u32 *ptr = (u32*)&exe[0x542C48 - 0x401200];
	char path[MAX_PATH];

	// 24*2 tables
	for (int i = 0; i < 48; i++)
	{
		sprintf_s(path, sizeof(path), "%s\\wp_dmg_em%02X.xml", folder_out, i + 0x10);
		DumpDamageTable((ATTACK_WEAPON_EM*)&exe[ptr[i] - 0x401200], path);
	}

	delete[] exe;
}

void Obj_load_texture(int Work_no, u8 *pTim, u32 Room_id, const char *outname)
{
	CBitmap::TIM_CHUNK *pal = (CBitmap::TIM_CHUNK*)&pTim[8];
	int palcnt = pal->h;
	CBitmap::TIM_CHUNK *pix;
	pix = (CBitmap::TIM_CHUNK*)&((u16*)&pal[1])[pal->w*pal->h];
	int pixcnt = pix->h / 64;

	CBitmap canvas;
	canvas.Create(pix->w * 2, pix->h);
	// irregularly split TIM
	if (palcnt > 1 && pixcnt != palcnt)
	{
		// attempt to fix
		CBitmap bmp;
		// 64, 64, 128
		if (Room_id == 0x012)
		{
			for (int i = 0; i < 2; i++)
			{
				bmp.CreateFromTim(pTim, i);
				canvas.Blt(bmp, 0, i * 64, 0, i * 64, bmp.w, 64);
			}
			bmp.CreateFromTim(pTim, 2);
			canvas.Blt(bmp, 0, 128, 0, 128, bmp.w, 128);
		}
		// 64, 128
		else if ((Room_id == 0x009 && Work_no == 0) || Room_id == 0x404 || Room_id == 0x406 || Room_id == 0x409)
		{
			bmp.CreateFromTim(pTim, 0);
			canvas.Blt(bmp, 0, 0, 0, 0, bmp.w, 64);
			bmp.CreateFromTim(pTim, 1);
			canvas.Blt(bmp, 0, 64, 0, 64, bmp.w, 128);
		}
		// 128, 64, 64
		else
		{
			bmp.CreateFromTim(pTim, 0);
			canvas.Blt(bmp, 0, 0, 0, 0, bmp.w, 128);

			// copies another 128 slice or less
			if (palcnt == 2)
			{
				bmp.CreateFromTim(pTim, 1);
				canvas.Blt(bmp, 0, 128, 0, 128, bmp.w, bmp.h - 128);
			}
			// copies two 64 slices
			else
			{
				for (int i = 1; i < palcnt; i++)
				{
					bmp.CreateFromTim(pTim, i);
					canvas.Blt(bmp, 0, i * 64 + 64, 0, i * 64 + 64, bmp.w, 64);
				}
			}
		}
	}
	// evenly split ???x64 TIM
	else if (palcnt > 1)
	{
		for (int i = 0; i < palcnt; i++)
		{
			CBitmap bmp;
			bmp.CreateFromTim(pTim, i);
			canvas.Blt(bmp, 0, i * 64, 0, i * 64, bmp.w, 64);
		}
	}
	// single palette tim, copy VERBATIM
	else
	{
		CBitmap bmp;
		bmp.CreateFromTim(pTim, 0);
		canvas.Blt(bmp, 0, 0, 0, 0, bmp.w, bmp.h);
	}

	canvas.SavePng(outname);
}

typedef struct tagObjEntry
{
	u32 texture,
		model;
} OBJ_ENTRY;

void ExtractRdtObjects(LPCSTR in_folder, LPCSTR out_folder, bool is_leon)
{
	char path[MAX_PATH];
	for (int Stage = 1; Stage < 7; Stage++)
	{
		for (int Room = 0; Room < 32; Room++)
		{
			CBufferFile f;
			sprintf_s(path, sizeof(path), "%s\\ROOM%X%02X%d.RDT", in_folder, Stage, Room, is_leon ? 0 : 1);
			if (!f.Open(path)) continue;

			RDT_HEADER *head = (RDT_HEADER*)f.data;
			if (!head->obj || !head->nOmodel) continue;

			printf("Dumping %s\n", path);

			OBJ_ENTRY *obj = (OBJ_ENTRY*)&f.data[head->obj];
			// count unique objects
			u32 cur = -1, mdl = -1;
			for (int i = 0, cnt = 0, work = 0; i < head->nOmodel; i++)
			{
				if (cur == obj[i].texture) continue;

				if (obj[i].texture)
				{
					if (obj[i].model)
					{
						sprintf_s(path, sizeof(path), "%s\\obj_%X%02X_%02d.png", out_folder, Stage, Room, cnt);
						Obj_load_texture(cnt, &f.data[obj[i].texture], ((Stage - 1) << 8) | Room, path);
					}
					else
					{
						sprintf_s(path, sizeof(path), "%s\\int_%X%02X_%02d.png", out_folder, Stage, Room, cnt);
						Obj_load_texture(cnt, &f.data[obj[i].texture], ((Stage - 1) << 8) | Room, path);
					}
				}

				cur = obj[i].texture;
				cnt++;
			}
		}
	}
}

void Extract_StTim(LPCSTR in_name, LPCSTR out_name)
{
	char path[MAX_PATH];
	CBufferFile f;
	if (!f.Open(in_name)) return;

	CreateDirectoryA(out_name, NULL);
	u8 *data = f.data;

	for (size_t i = 0, pos = 0, size = f.size; pos < size; i++)
	{
		sprintf_s(path, sizeof(path), "%s\\%d.png", out_name, i);
		// paletted shit
		if ( i == 2 )
		{
			CTim tim;
			pos += tim.Open(&data[pos]);
			tim.SavePng(path);
		}
		else
		{
			CBitmap bmp;
			pos += bmp.CreateFromTim(&data[pos], 0);
			bmp.SavePng(path);
		}
	}
}

void print_info()
{
	printf("Room Slicer - A general extractor for Resident Evil 2 / Biohazard 2 (PC).\n");
}

void print_usage()
{
	printf("Usage: <option> <input> <output>\n"
		"Options:\n"
		"-dmg <input.exe> <output folder> [p]");
}

void GatherRDT_data(LPCSTR filename, LPCSTR out)
{
	printf("Processing %s\n", filename);

	CRoom room;
	room.Open(filename);
	room.DumpScd(out);
}

void DumpScd(LPCSTR folder_in, LPCSTR folder_out, bool is_leon)
{
	char path0[MAX_PATH],
		path1[MAX_PATH];

	for (int Stage = 1; Stage < 8; Stage++)
	{
		for (int Room = 0; Room < 32; Room++)
		{
			if (Stage == 7 && Room == 5)
				return;
			sprintf_s(path0, sizeof(path0), "%s\\ROOM%X%02X%d.RDT", folder_in, Stage, Room, is_leon ? 0 : 1);
			sprintf_s(path1, sizeof(path1), "%s\\ROOM%X%02X", folder_out, Stage, Room);
			GatherRDT_data(path0, path1);
		}
	}
}

typedef struct tagFootstep
{
	s8 val[3];
} FOOTSTEP;

void ExtractFootsteps(LPCSTR exe_name, LPCSTR xml_name)
{
	CBufferFile f;
	f.Open(exe_name);

	XMLDocument xml;
	auto root = xml.NewElement("Footsteps");
	FOOTSTEP *foot = (FOOTSTEP*)&f.data[0x122208];
	char name[32];

	for (int i = 0; i < 202; i++)
	{
		if (foot[i].val[0] == -1 &&
			foot[i].val[1] == -1 &&
			foot[i].val[2] == -1)
			continue;
		auto s = xml.NewElement("Entry");

		sprintf_s(name, sizeof(name), "0x%X%02X", (i / 29) + 1, i % 29);
		s->SetAttribute("Room", name);
		if (foot[i].val[0] != -1) s->SetAttribute("Set_0", foot[i].val[0]);
		if (foot[i].val[1] != -1) s->SetAttribute("Set_1", foot[i].val[1]);
		if (foot[i].val[2] != -1) s->SetAttribute("Set_2", foot[i].val[2]);

		//sprintf_s(name, sizeof(name), "ROOM%X%02X", i / 32 + 1, i % 32);
		//root->InsertEndChild(xml.NewComment(name));
		root->InsertEndChild(s);
	}
	xml.InsertEndChild(root);
	xml.SaveFile(xml_name);
}

void DumpRe3GcBgs()
{
	char in[MAX_PATH], out[MAX_PATH];
	for (int Stage = 1; Stage < 6; Stage++)
	{
		for (int Room = 0; Room < 49; Room++)
		{
			for (int Cut = 0; Cut < 32; Cut++)
			{
				sprintf_s(in,  MAX_PATH, "E:\\ISO\\GC\\Resident Evil 3 - Nemesis (USA)\\Bss\\r%d%02X%02X.adt", Stage, Room, Cut);
				sprintf_s(out, MAX_PATH, "E:\\ISO\\GC\\Resident Evil 3 - Nemesis (USA)\\Png 240p\\R%d%02X%02d", Stage, Room, Cut);
				DecompileADT(in, out);
			}
		}
	}
}

void Fix_rdt(const char* name)
{
	CBufferFile f;
	if (!f.Open(name))
		return;

	RDT_HEADER* h = (RDT_HEADER*)f.data;

	if (h->tex[0])
		h->tex[1] = h->tex[0];

	FILE* o = fopen(name, "wb");
	fwrite(f.data, f.size, 1, o);
	fclose(o);
}

int main()
{
	for (int Stage = 1; Stage < 8; Stage++)
	{
		for (int Room = 0; Room < 32; Room++)
		{
			printf("Fixing %X%02X...\n", Stage, Room);
			char name[MAX_PATH];
			sprintf_s(name, MAX_PATH, "ROOM%X%02X0.RDT", Stage, Room);
			Fix_rdt(name);
			sprintf_s(name, MAX_PATH, "ROOM%X%02X1.RDT", Stage, Room);
			Fix_rdt(name);
		}
	}

	//Set_color_mode(1);
	//DecompileADT("E:\\ISO\\GC\\Resident Evil 3 - Nemesis (USA)\\Bss\\rc1150.adt", "E:\\ISO\\GC\\Resident Evil 3 - Nemesis (USA)\\Png\\rc1150");
	//DecompileADT("E:\\ISO\\GC\\Resident Evil 3 - Nemesis (USA)\\Bss\\rc115c.adt", "E:\\ISO\\GC\\Resident Evil 3 - Nemesis (USA)\\Png\\rc115c");
	//DumpRe3GcBgs();

	//FixRdtText("D:\\Program Files\\BIOHAZARD 2 PC\\mod_leonc\\pl0\\rdt");

	//Dump_debug_names("main.exe", "debug.txt");

	//Extract_StTim("D:\\Program Files\\BIOHAZARD 2 PC\\common\\data\\st0_jp.tim", "st0_jp");
	//Extract_StTim("D:\\Program Files\\BIOHAZARD 2 PC\\common\\data\\st1_jp.tim", "st1_jp");
	//Extract_StTim("D:\\Program Files\\BIOHAZARD 2 PC\\common\\data\\st2_jp.tim", "st2_jp");

	//DecompileADT("D:\\Program Files\\BIOHAZARD 2 PC\\claire2.5\\COMMON\\DATU\\FONT0P.ADT", "font0p");
	//DecompileADT("D:\\Program Files\\BIOHAZARD 2 PC\\COMMON\\DATA\\FONT0.ADT", "font0");

	//ExtractFootsteps("D:\\Program Files\\BIOHAZARD 2 PC\\bio2 1.10.exe", "xml\\footsteps.xml");

	//DumpScd("D:\\Program Files\\BIOHAZARD 2 PC\\pl1\\rdt", "scd_1", false);

	//Extract_StTim("D:\\Program Files\\BIOHAZARD 2 PC\\common\\data\\st0_jp.tim", "st");
	//Extract_StTim("D:\\Program Files\\BIOHAZARD 2 PC\\mod_mn1_de\\common\\data\\st0_jp.tim", "D:\\Program Files\\BIOHAZARD 2 PC\\mod_mn1_de\\common\\data\\st0_jp");

	//ExtractRdtObjects("D:\\Program Files\\BIOHAZARD 2 PC\\mod_mn1_de\\pl0\\rdt", "obj_hunk", true);
	//ExtractRdtObjects("D:\\Program Files\\BIOHAZARD 2 PC\\pl0\\rdt", "obj_leon", true);
	//ExtractRdtObjects("D:\\Program Files\\BIOHAZARD 2 PC\\pl1\\rdt", "obj_claire", false);

	//ExtractDamageTables("D:\\Program Files\\BIOHAZARD 2 PC\\bio2 1.10.exe", "xml");
	//ExtractDamageTablesPlatinum("D:\\Program Files\\Merdal Night\\Resident Evil Mortal Night (D.E.) - EP1\\RE Mortal Night (DE) - EP1\\Episode I.exe", "xml_mortal");
	//ExtractEspdata("D:\\Program Files\\BIOHAZARD 2 PC\\common\\bin\\espdat1.bin", "esp1");
	//ExtractEspdata("D:\\Program Files\\BIOHAZARD 2 PC\\common\\bin\\espdat2.bin", "esp2");

	//extract_roomcut("D:\\Program Files\\BIOHAZARD 2 PC\\mod_destiny\\COMMON\\bin\\roomcut.bin", "ROOM_destiny");
	//dump_new("ROOM", "ROOM_destiny", "destiny");

	//DecompileItemITP("D:\\Program Files\\BIOHAZARD 2 PC\\mod_mn1_de\\common\\bin\\ITEMDATA.BIN", "item");

	//DecompressFile("D:\\Program Files\\BIOHAZARD 2 PC\\mod_mn1_de\\common\\file\\FILE03U.BIN", "D:\\Program Files\\BIOHAZARD 2 PC\\mod_mn1_de\\common\\file\\FILE03U");
	//DecompressFile("D:\\Program Files\\BIOHAZARD 2 PC\\mod_mn1_de\\common\\file\\FILE04U.BIN", "D:\\Program Files\\BIOHAZARD 2 PC\\mod_mn1_de\\common\\file\\FILE04U");
	//DecompressFile("D:\\Program Files\\BIOHAZARD 2 PC\\mod_mn1_de\\common\\file\\FILE09U.BIN", "D:\\Program Files\\BIOHAZARD 2 PC\\mod_mn1_de\\common\\file\\FILE09U");
	//DecompressFile("D:\\Program Files\\BIOHAZARD 2 PC\\mod_mn1_de\\common\\file\\FILE15U.BIN", "D:\\Program Files\\BIOHAZARD 2 PC\\mod_mn1_de\\common\\file\\FILE15U");
	//DecompressFile("D:\\Program Files\\BIOHAZARD 2 PC\\mod_mn1_de\\common\\file\\FILE18U.BIN", "D:\\Program Files\\BIOHAZARD 2 PC\\mod_mn1_de\\common\\file\\FILE18U");
	//FixRdtText("D:\\Program Files\\BIOHAZARD 2 PC\\mod_mn1_de\\pl0\\rdt");

	//DecompileADT("D:\\Program Files\\BIOHAZARD 2 PC\\common\\Data\\tex216.adt", "tex216");
	//DecompileADT("D:\\Program Files\\BIOHAZARD 2 PC\\common\\Data\\tex316.adt", "tex316");
	//DecompileADT("D:\\Program Files\\BIOHAZARD 2 PC\\common\\Data\\end00.adt", "end00");
	//DecompileADT("D:\\Program Files\\BIOHAZARD 2 PC\\common\\Data\\EXTITLE1.adt", "end00");
	//DecompileADT("D:\\Program Files\\BIOHAZARD 2 PC\\common\\Data\\font0.adt", "font0");
	//DecompileADT("D:\\Program Files\\BIOHAZARD 2 PC\\common\\Data\\kage.adt", "kage");
	//DecompileADT("D:\\Program Files\\BIOHAZARD 2 PC\\common\\bin\\pcstaff.adt", "pcstaff");
	//DecompileADT("D:\\Program Files\\BIOHAZARD 2 PC\\common\\data\\open14.adt", "open14.png");

	//DecompressFile("D:\\Program Files\\BIOHAZARD 2 PC\\mod_destiny\\COMMON\\file\\FILE00U.BIN", "D:\\Program Files\\BIOHAZARD 2 PC\\mod_destiny\\COMMON\\file\\FILE00U");
	//DecompressFile("D:\\Program Files\\BIOHAZARD 2 PC\\mod_destiny\\COMMON\\file\\FILE01U.BIN", "D:\\Program Files\\BIOHAZARD 2 PC\\mod_destiny\\COMMON\\file\\FILE01U");
	//DecompressFile("D:\\Program Files\\BIOHAZARD 2 PC\\mod_destiny\\COMMON\\file\\FILE02U.BIN", "D:\\Program Files\\BIOHAZARD 2 PC\\mod_destiny\\COMMON\\file\\FILE02U");
	//DecompressFile("D:\\Program Files\\BIOHAZARD 2 PC\\mod_destiny\\COMMON\\file\\FILE03U.BIN", "D:\\Program Files\\BIOHAZARD 2 PC\\mod_destiny\\COMMON\\file\\FILE03U");
	//DecompressFile("D:\\Program Files\\BIOHAZARD 2 PC\\mod_destiny\\COMMON\\file\\FILE05U.BIN", "D:\\Program Files\\BIOHAZARD 2 PC\\mod_destiny\\COMMON\\file\\FILE05U");
	//DecompressFile("D:\\Program Files\\BIOHAZARD 2 PC\\mod_destiny\\COMMON\\file\\FILE06U.BIN", "D:\\Program Files\\BIOHAZARD 2 PC\\mod_destiny\\COMMON\\file\\FILE06U");
	//DecompressFile("D:\\Program Files\\BIOHAZARD 2 PC\\mod_destiny\\COMMON\\file\\FILE07U.BIN", "D:\\Program Files\\BIOHAZARD 2 PC\\mod_destiny\\COMMON\\file\\FILE07U");
	//DecompressFile("D:\\Program Files\\BIOHAZARD 2 PC\\mod_destiny\\COMMON\\file\\FILE0AU.BIN", "D:\\Program Files\\BIOHAZARD 2 PC\\mod_destiny\\COMMON\\file\\FILE0AU");
	//DecompressFile("D:\\Program Files\\BIOHAZARD 2 PC\\mod_destiny\\COMMON\\file\\FILE0BU.BIN", "D:\\Program Files\\BIOHAZARD 2 PC\\mod_destiny\\COMMON\\file\\FILE0BU");
	//DecompressFile("D:\\Program Files\\BIOHAZARD 2 PC\\mod_destiny\\COMMON\\file\\FILE12U.BIN", "D:\\Program Files\\BIOHAZARD 2 PC\\mod_destiny\\COMMON\\file\\FILE12U");
	//DecompressFile("D:\\Program Files\\BIOHAZARD 2 PC\\mod_destiny\\COMMON\\file\\FILE13U.BIN", "D:\\Program Files\\BIOHAZARD 2 PC\\mod_destiny\\COMMON\\file\\FILE13U");
	//DecompressFile("D:\\Program Files\\BIOHAZARD 2 PC\\mod_destiny\\COMMON\\file\\FILE14U.BIN", "D:\\Program Files\\BIOHAZARD 2 PC\\mod_destiny\\COMMON\\file\\FILE14U");
	//DecompressFile("D:\\Program Files\\BIOHAZARD 2 PC\\mod_destiny\\COMMON\\file\\FILE15U.BIN", "D:\\Program Files\\BIOHAZARD 2 PC\\mod_destiny\\COMMON\\file\\FILE15U");
	//DecompressFile("D:\\Program Files\\BIOHAZARD 2 PC\\mod_destiny\\COMMON\\file\\FILE16U.BIN", "D:\\Program Files\\BIOHAZARD 2 PC\\mod_destiny\\COMMON\\file\\FILE16U");
	//DecompressFile("D:\\Program Files\\BIOHAZARD 2 PC\\mod_destiny\\COMMON\\file\\FILE17U.BIN", "D:\\Program Files\\BIOHAZARD 2 PC\\mod_destiny\\COMMON\\file\\FILE17U");
	//DecompressFile("D:\\Program Files\\BIOHAZARD 2 PC\\mod_destiny\\COMMON\\file\\FILE18U.BIN", "D:\\Program Files\\BIOHAZARD 2 PC\\mod_destiny\\COMMON\\file\\FILE18U");

#if 1
	//extract_roomcut("D:\\Program Files\\Merdal Night\\Resident Evil Mortal Night (D.E.) - EP1\\RE Mortal Night (DE) - EP1\\Common\\Bin\\roomcut.bin", "ROOM_merdal");
#else
	//extract_roomcut("D:\\Program Files\\BIOHAZARD 2 PC\\common\\bin\\roomcut.bin", "ROOM");
#endif

	//dump_new("ROOM", "ROOM_merdal", "merda");

    return 0;
}

