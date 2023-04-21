// RDT extract.cpp : Defines the entry point for the console application.
//
#define _CRT_SECURE_NO_WARNINGS

#include "stdafx.h"
#include <Windows.h>
#include <vector>
#include <algorithm>
#include "tinyxml2.h"
#include "FileHandle.h"

#include <Mmsystem.h>
#pragma comment(lib, "winmm.lib")

std::string DecodeStringEU(u8 *data);

#define MAGIC(a,b,c,d)	a | (b << 8) | (c << 16) | (d << 24)

void test_mmio()
{
	HMMIO hmmio = mmioOpenA("D:\\Program Files\\BIOHAZARD 2 PC\\common\\Sound\\BGM\\main00.sap", NULL, MMIO_ALLOCBUF);
	u32 r;

	WAVEFORMATEX wfx;

	mmioRead(hmmio, (HPSTR)&r, 4);
	mmioRead(hmmio, (HPSTR)&r, 4);

	MMCKINFO ckwav, ckdat;
	ckwav.fccType = FOURCC("WAVE");
	mmioDescend(hmmio, &ckwav, NULL, 32);
	mmioDescend(hmmio, &ckdat, &ckwav, 0);
	mmioRead(hmmio, (HPSTR)&wfx, ckdat.cksize);

	mmioClose(hmmio, 0);
}

typedef struct tagRdtHeader
{
	char nSprite;
	char nCut;			// Number of objects at offset 7, seems to be number of cameras of the room
	char nOmodel;		// Number of objects at offset 10
	char nItem;			// unused?
	char nDoor;			// unused?
	char nRoom_at;		// unused?
	char Reverb_lv;		// unused?
	u8 nSprite_max;		// 
	u32 pEdt0,			// 08
		pVh0,			// 0c
		pVb0;			// 10
	u32 padd0,			// 14
		padd1,			// 18
		pRbj_end;		// 1c
	u32 pSca;			// 20
	u32 pRcut;			// 24
	u32 pVcut;			// 28
	u32 pLight;			// 2c
	u32 pOmodel;		// 30
	u32 pFloor, pBlock;	// 34, 38
	u32 pMessage[2];	// 3c, 40
	u32 pScd[3];		// 44, 48, 4c
	u32 pEsp_hed, pEsp_end;	// 50, 54
	u32 pEsp_tim, pEsp_tim_end;	// 58, 5c
	u32 pRbj;			// 60
} RDT_HEADER;

typedef struct tagPair
{
	unsigned short id,
		ptr;
} PAIR;

void ExtractSCD0(LPCSTR filename, LPCSTR folder_out)
{
	CreateDirectoryA(folder_out, NULL);

	u8 *buffer;
	FILE *f = fopen(filename, "rb+");

	fseek(f, 0, SEEK_END);
	size_t size = ftell(f);
	fseek(f, 0, SEEK_SET);

	buffer = new u8[size];
	fread(buffer, size, 1, f);

	RDT_HEADER *head = (RDT_HEADER*)buffer;

	// hook onto scd data
	u16 *ptr = (u16*)&buffer[head->pScd[1]];
	u8 *end = &buffer[head->pScd[2]];

	FILE *o;

	char str[MAX_PATH];
	int count = ptr[0] / 2;
	// extract regular ones
	for (int i = 0; i < count - 1; i++)
	{
		sprintf(str, "%s\\main_%02d.scd", folder_out, i);
		o = fopen(str, "wb+");
		fwrite(&((u8*)ptr)[ptr[0]], ptr[i + 1] - ptr[i], 1, o);
		fclose(o);
	}
	// extract last one
	sprintf(str, "%s\\main_%02d.scd", folder_out, count - 1);
	o = fopen(str, "wb+");
	fwrite(&((u8*)ptr)[ptr[count - 1]], end - &((u8*)ptr)[ptr[count - 1]], 1, o);
	fclose(o);
	fclose(f);

	delete[] buffer;
}

bool sort_pair(PAIR &p0, PAIR &p1)
{
	return p0.ptr < p1.ptr;
}

size_t find_ptr(std::vector<PAIR> &pair, int id)
{
	for (size_t i = 0, si = pair.size(); i < si; i++)
		if (pair[i].id == id)
			return i;

	return -1;
}

void ExtractSCD1(LPCSTR filename, LPCSTR folder_out)
{
	CreateDirectoryA(folder_out, NULL);

	u8 *buffer;
	FILE *f = fopen(filename, "rb+");

	fseek(f, 0, SEEK_END);
	size_t size = ftell(f);
	fseek(f, 0, SEEK_SET);

	buffer = new u8[size];
	fread(buffer, size, 1, f);

	RDT_HEADER *head = (RDT_HEADER*)buffer;

	// hook onto scd data
	u16 *ptr = (u16*)&buffer[head->pScd[2]];
	u8 *end = &buffer[head->pMessage[0]];

	FILE *o;

	char str[MAX_PATH];
	int count = ptr[0] / 2;

	// find lowest value for count
	unsigned short lowest = 0xffff;
	for(int i = 0; i < lowest / 2; i++)
	{
		if (ptr[i] < lowest)
		{
			lowest = ptr[i];
			if (i >= lowest / 2)
				break;
		}
	}

	std::vector<PAIR> pair;
	for (int i = 0; i < count; i++)
	{
		PAIR p;
		p.ptr = ptr[i];
		p.id = i;
		pair.push_back(p);
	}
	std::sort(pair.begin(), pair.end(), sort_pair);
	size_t last = (u32)end - (u32)&((u8*)ptr)[pair[0].ptr];
	{
		PAIR p;
		p.id = -1;
		p.ptr = (unsigned short) last;
		pair.push_back(p);
	}

	// extract regular ones
	for (int i = 0; i < count; i++)
	{
		sprintf(str, "%s\\sub_%02d.scd", folder_out, i);
		o = fopen(str, "wb+");
		fwrite(&((u8*)ptr)[pair[i].ptr], pair[i + 1].ptr - pair[i].ptr, 1, o);
		fclose(o);
	}
	fclose(f);

	delete[] buffer;
}

void BuildSCD(LPCSTR folder_in, LPCSTR out_file)
{
	char str[MAX_PATH];
	FILE *out = fopen(out_file, "wb+");

	u8 *buffers[32];
	size_t sizes[32];
	memset(buffers, 0, sizeof(buffers));

	int cnt = 0;
	for(int i = 0;; i++)
	{
		sprintf(str, "%s\\main_%02d.scd", folder_in, i);
		FILE *in = fopen(str, "rb+");
		if (!in) break;

		fseek(in, 0, SEEK_END);
		sizes[i] = ftell(in);
		fseek(in, 0, SEEK_SET);

		buffers[i] = new u8[sizes[i]];
		fread(buffers[i], sizes[i], 1, in);

		fclose(in);
		cnt++;
	}

	u16 *ptr = new u16[cnt];

	fseek(out, 2 * cnt, SEEK_SET);
	for (int i = 0; i < cnt; i++)
	{
		ptr[i] = (u16)ftell(out);
		fwrite(buffers[i], sizes[i], 1, out);
		delete[] buffers[i];
	}
	// write pointers
	fseek(out, 0, SEEK_SET);
	fwrite(ptr, 2 * cnt, 1, out);

	fclose(out);
}

std::string DecodeString(u8 *data);
using namespace tinyxml2;

typedef struct tagItemMix
{
	u8 with,
		type,
		result,
		pix_no;
} MIX_DATA;

typedef struct tagItemData
{
	u8 Item_max,
		Item_attribute,
		Item_num_col,
		Item_mix_num;
	MIX_DATA *Item_mix_data;
} ITEM_DATA;

typedef struct tagFileData
{
	u16 Page_max,
		Back_h;
} FILE_DATA;

std::string NameToID(std::string &str)
{
	if (str.size() == 0)
		return "";

	std::string out = "ID_";
	for (size_t i = 0, si = str.size(); i < si; i++)
	{
		char c = str[i];
		if (c == ' ')
			out += '_';
		else if (c == '.' ||
			c == '-');
		else if (c == '&')
			out += '*';
		else out += (char)toupper(str[i]);
	}

	return out;
}

void FixDuplicates(std::vector<std::string> &str)
{
	for (size_t i = 0, si = str.size(); i < si; i++)
	{
		std::string s = str[i];
		int dup = 1;
		for (size_t j = i + 1; j < si; j++)
		{
			if (s == str[j])
			{
				str[j] += "_";
				str[j] += dup + '0';
				dup++;
			}
		}
		if (dup != 1)
			str[i] = s + "_0";
	}
}

std::string col2String(u8 col)
{
	static LPCSTR colors[]=
	{
		"GREEN",
		"RED",
		"YELLOW",
		"BLUE",
		"NONE"
	};
	std::string str;
	str = colors[col & 0x7f];
	if (col & 0x80)
		str += ",PERCENTAGE";
	return str;
}

void ExtractItems()
{
	FILE *in = fopen("D:\\Program Files\\BIOHAZARD 2 PC\\bio2 1.10.exe", "rb+");
	fseek(in, 0, SEEK_END);
	size_t s = ftell(in);
	u8 *exe = new u8[s];
	fseek(in, 0, SEEK_SET);
	fread(exe, s, 1, in);

	u16 *ptr = (u16*)&exe[0x1324D8];
	u8 *str = &exe[0x130DB0];
	XMLDocument xml;
	xml.SetBOM(true);

	XMLElement *sss = xml.NewElement("Strings");
	for (int i = 0; i < 232 / 2; i++)
	{
		XMLElement *el = xml.NewElement("Text");
		std::string s = DecodeString(&str[ptr[i]]);
		el->SetText(s.c_str());
		sss->InsertEndChild(el);
	}
	xml.InsertEndChild(sss);
	xml.SaveFile("system.xml");

	ptr = (u16*)&exe[0x12f1f8];
	str = &exe[0x12eb98];
	xml.Clear();
	ITEM_DATA *data = (ITEM_DATA*)&exe[0x13DE28];

	std::vector<std::string> item_id;
	for (int i = 0; i < 280 / 2; i++)
	{
		std::string s = DecodeString(&str[ptr[i]]);
		item_id.push_back(NameToID(s));
	}
	FixDuplicates(item_id);

	static LPCSTR mix_type[]=
	{
		"IM_TYPE_0",
		"IM_TYPE_1",
		"IM_TYPE_2",
		"IM_TYPE_3",
		"IM_COMBINE",
		"IM_RELOAD_0",
		"IM_RELOAD_1",
		"IM_LAUNCH_X",
		"IM_LAUNCH_F",
		"IM_LAUNCH_A",
		"IM_TYPE_10",
		"IM_HERB",
	};
	static FILE_DATA file_page[] =
	{
		5,0x70, 5,0x50, 1,0x50, 2,0x50,
		9,0x50, 9,0x50, 9,0x50, 6,0x70,
		4,0x70, 6,0x50, 1,0x80, 2,0x90,
		3,0x90, 2,0x90, 3,0x70, 8,0x50,
		6,0x50, 9,0x50, 5,0x50, 1,0x90,
		6,0x50, 5,0x50, 4,0x60, 15,0x50,
		18,0x50
	};

	XMLElement *items = xml.NewElement("Items");
	for (int i = 0; i < 280 / 2; i++)
	{
		XMLElement *el = xml.NewElement("Item");
		std::string s = DecodeString(&str[ptr[i]]);
		el->SetAttribute("name", s.c_str());
		// regular items
		if (i < 101)
		{
			el->SetAttribute("id", item_id[i].c_str());
			XMLElement *sub = xml.NewElement("Data");
			sub->SetAttribute("max", data[i].Item_max);
			if (data[i].Item_attribute != 0)
			{
				sub->SetAttribute("attr", item_id[data[i].Item_attribute + 104 + _countof(file_page)].c_str());
			}
			sub->SetAttribute("col", col2String(data[i].Item_num_col).c_str());
			if (data[i].Item_mix_num)
			{
				MIX_DATA *mix = (MIX_DATA*)&exe[(u32)data[i].Item_mix_data-0x400000];
				for (int j = 0, js = data[i].Item_mix_num; j < js; j++)
				{
					XMLElement *x = xml.NewElement("Mix");
					x->SetAttribute("with", item_id[mix[j].with].c_str());
					x->SetAttribute("result", item_id[mix[j].result].c_str());
					x->SetAttribute("type", mix_type[mix[j].type]);
					if(mix[j].pix_no != 255)
						x->SetAttribute("pic",  mix[j].pix_no);
					sub->InsertEndChild(x);
				}
			}
			el->InsertEndChild(sub);
		}
		// FILEs
		else if (i >= 104 && i < 104 + _countof(file_page))
		{
			XMLElement *sub = xml.NewElement("File");
			sub->SetAttribute("pages", file_page[i - 104].Page_max);
			sub->SetAttribute("back_h", file_page[i - 104].Back_h);
			el->InsertEndChild(sub);
		}
		// alternate names for keys and some items
		else if (i >= 104 + _countof(file_page))
		{
			el->SetAttribute("id", item_id[i].c_str());
		}
		items->InsertEndChild(el);
	}
	xml.InsertEndChild(items);
	xml.SaveFile("item.xml");

	delete[] exe;
}

#define OFFSET	0x81E8

void ExtractItemsPlatinum(LPCSTR exe_name, LPCSTR folder)
{
	//FILE *in = fopen("D:\\Program Files\\Merdal Knight\\Resident Evil - Mortal Night v1.80\\Episode I.exe", "rb+");
	FILE *in = fopen(exe_name, "rb+");
	fseek(in, 0, SEEK_END);
	size_t s = ftell(in);
	u8 *exe = new u8[s];
	fseek(in, 0, SEEK_SET);
	fread(exe, s, 1, in);

	u16 *ptr = (u16*)&exe[0x1324D8];
	u8 *str = &exe[0x130DB0];
	XMLDocument xml;
	xml.SetBOM(true);

	//XMLElement *sss = xml.NewElement("Strings");
	//for (int i = 0; i < 232 / 2; i++)
	//{
	//	XMLElement *el = xml.NewElement("Text");
	//	std::string s = DecodeString(&str[ptr[i]]);
	//	el->SetText(s.c_str());
	//	sss->InsertEndChild(el);
	//}
	//xml.InsertEndChild(sss);
	//xml.SaveFile("system.xml");

	ptr = (u16*)&exe[0x12f1f8 + OFFSET];
	str = &exe[0x12eb98 + OFFSET];
	xml.Clear();
	ITEM_DATA *data = (ITEM_DATA*)&exe[0x145BA8];

	std::vector<std::string> item_id;
	for (int i = 0; i < 280 / 2; i++)
	{
		std::string s = DecodeString(&str[ptr[i]]);
		item_id.push_back(NameToID(s));
	}
	FixDuplicates(item_id);

	static LPCSTR mix_type[] =
	{
		"IM_TYPE_0",
		"IM_TYPE_1",
		"IM_TYPE_2",
		"IM_TYPE_3",
		"IM_COMBINE",
		"IM_RELOAD_0",
		"IM_RELOAD_1",
		"IM_LAUNCH_X",
		"IM_LAUNCH_F",
		"IM_LAUNCH_A",
		"IM_TYPE_10",
		"IM_HERB",
	};
	static FILE_DATA file_page[] =
	{
		5,0x70, 5,0x50, 1,0x50, 2,0x50,
		9,0x50, 9,0x50, 9,0x50, 6,0x70,
		4,0x70, 6,0x50, 1,0x80, 2,0x90,
		3,0x90, 2,0x90, 3,0x70, 8,0x50,
		6,0x50, 9,0x50, 5,0x50, 1,0x90,
		6,0x50, 5,0x50, 4,0x60, 15,0x50,
		18,0x50
	};

	XMLElement *items = xml.NewElement("Items");
	for (int i = 0; i < 280 / 2; i++)
	{
		printf("Convert %d\n", i);
		XMLElement *el = xml.NewElement("Item");
		std::string s = DecodeString(&str[ptr[i]]);
		el->SetAttribute("name", s.c_str());
		// regular items
		if (i < 101)
		{
			el->SetAttribute("id", item_id[i].c_str());
			XMLElement *sub = xml.NewElement("Data");
			sub->SetAttribute("max", data[i].Item_max);
			if (data[i].Item_attribute != 0)
			{
				sub->SetAttribute("attr", item_id[data[i].Item_attribute + 104 + _countof(file_page)].c_str());
			}
			sub->SetAttribute("col", col2String(data[i].Item_num_col).c_str());
			if (data[i].Item_mix_num)
			{
				MIX_DATA *mix = (MIX_DATA*)&exe[(u32)data[i].Item_mix_data - 0x401200];
				for (int j = 0, js = data[i].Item_mix_num; j < js; j++)
				{
					XMLElement *x = xml.NewElement("Mix");
					x->SetAttribute("with", item_id[mix[j].with].c_str());
					x->SetAttribute("result", item_id[mix[j].result].c_str());
					x->SetAttribute("type", mix_type[mix[j].type]);
					if (mix[j].pix_no != 255)
						x->SetAttribute("pic", mix[j].pix_no);
					sub->InsertEndChild(x);
				}
			}
			el->InsertEndChild(sub);
		}
		// FILEs
		else if (i >= 104 && i < 104 + _countof(file_page))
		{
			XMLElement *sub = xml.NewElement("File");
			sub->SetAttribute("pages", file_page[i - 104].Page_max);
			sub->SetAttribute("back_h", file_page[i - 104].Back_h);
			el->InsertEndChild(sub);
		}
		// alternate names for keys and some items
		else if (i >= 104 + _countof(file_page))
		{
			el->SetAttribute("id", item_id[i].c_str());
		}
		items->InsertEndChild(el);
	}
	xml.InsertEndChild(items);
	xml.SaveFile((std::string(folder) + std::string("\\item.xml")).c_str());

	delete[] exe;
}

void ExtractItemsPlatinumIT(LPCSTR exe_name, LPCSTR folder)
{
	//FILE *in = fopen("D:\\Program Files\\Merdal Knight\\Resident Evil - Mortal Night v1.80\\Episode I.exe", "rb+");
	FILE *in = fopen(exe_name, "rb+");
	fseek(in, 0, SEEK_END);
	size_t s = ftell(in);
	u8 *exe = new u8[s];
	fseek(in, 0, SEEK_SET);
	fread(exe, s, 1, in);

	u16 *ptr = (u16*)&exe[0x1379f0];
	u8 *str = &exe[0x137230];
	XMLDocument xml;
	xml.SetBOM(true);

	xml.Clear();
	ITEM_DATA *data = (ITEM_DATA*)&exe[0x145610];

	std::vector<std::string> item_id;
	for (int i = 0; i < 280 / 2; i++)
	{
		std::string s = DecodeStringEU(&str[ptr[i]]);
		item_id.push_back(NameToID(s));
	}
	FixDuplicates(item_id);

	static LPCSTR mix_type[] =
	{
		"IM_TYPE_0",
		"IM_TYPE_1",
		"IM_TYPE_2",
		"IM_TYPE_3",
		"IM_COMBINE",
		"IM_RELOAD_0",
		"IM_RELOAD_1",
		"IM_LAUNCH_X",
		"IM_LAUNCH_F",
		"IM_LAUNCH_A",
		"IM_TYPE_10",
		"IM_HERB",
	};
	static FILE_DATA file_page[] =
	{
		5,0x70, 5,0x50, 1,0x50, 2,0x50,
		9,0x50, 9,0x50, 9,0x50, 6,0x70,
		4,0x70, 6,0x50, 1,0x80, 2,0x90,
		3,0x90, 2,0x90, 3,0x70, 8,0x50,
		6,0x50, 9,0x50, 5,0x50, 1,0x90,
		6,0x50, 5,0x50, 4,0x60, 15,0x50,
		18,0x50
	};

	XMLElement *items = xml.NewElement("Items");
	for (int i = 0; i < 280 / 2; i++)
	{
		printf("Convert %d\n", i);
		XMLElement *el = xml.NewElement("Item");
		std::string s = DecodeStringEU(&str[ptr[i]]);
		el->SetAttribute("name", s.c_str());
		// regular items
		if (i < 101)
		{
			el->SetAttribute("id", item_id[i].c_str());
			XMLElement *sub = xml.NewElement("Data");
			sub->SetAttribute("max", data[i].Item_max);
			if (data[i].Item_attribute != 0)
			{
				sub->SetAttribute("attr", item_id[data[i].Item_attribute + 104 + _countof(file_page)].c_str());
			}
			sub->SetAttribute("col", col2String(data[i].Item_num_col).c_str());
			if (data[i].Item_mix_num)
			{
				MIX_DATA *mix = (MIX_DATA*)&exe[(u32)data[i].Item_mix_data - 0x401800];
				for (int j = 0, js = data[i].Item_mix_num; j < js; j++)
				{
					XMLElement *x = xml.NewElement("Mix");
					x->SetAttribute("with", item_id[mix[j].with].c_str());
					x->SetAttribute("result", item_id[mix[j].result].c_str());
					x->SetAttribute("type", mix_type[mix[j].type]);
					if (mix[j].pix_no != 255)
						x->SetAttribute("pic", mix[j].pix_no);
					sub->InsertEndChild(x);
				}
			}
			el->InsertEndChild(sub);
		}
		// FILEs
		else if (i >= 104 && i < 104 + _countof(file_page))
		{
			XMLElement *sub = xml.NewElement("File");
			sub->SetAttribute("pages", file_page[i - 104].Page_max);
			sub->SetAttribute("back_h", file_page[i - 104].Back_h);
			el->InsertEndChild(sub);
		}
		// alternate names for keys and some items
		else if (i >= 104 + _countof(file_page))
		{
			el->SetAttribute("id", item_id[i].c_str());
		}
		items->InsertEndChild(el);
	}
	xml.InsertEndChild(items);
	xml.SaveFile((std::string(folder) + std::string("\\item.xml")).c_str());

	delete[] exe;
}

void ExtractTextGeneric(LPCSTR exe_name, LPCSTR out_name, u32 ptr, u32 text, int count)
{
	// cache data
	FILE *in = fopen(exe_name, "rb+");
	fseek(in, 0, SEEK_END);
	size_t s = ftell(in);
	u8 *exe = new u8[s];
	fseek(in, 0, SEEK_SET);
	fread(exe, s, 1, in);

	// begin extraction
	u16 *_ptr = (u16*)&exe[ptr];
	u8 *str = &exe[text];
	XMLDocument xml;
	xml.SetBOM(true);

	XMLElement *items = xml.NewElement("Strings");
	// extract text
	for (int i = 0; i < count; i++)
	{
		std::string s = DecodeString(&str[_ptr[i]]);
		XMLElement *sub = xml.NewElement("Text");
		sub->SetText(s.c_str());
		items->InsertEndChild(sub);
	}

	xml.InsertEndChild(items);
	xml.SaveFile(out_name);

	delete[] exe;
}

void ExtractTextGenericEU(LPCSTR exe_name, LPCSTR out_name, u32 ptr, u32 text, int count)
{
	// cache data
	FILE *in = fopen(exe_name, "rb+");
	fseek(in, 0, SEEK_END);
	size_t s = ftell(in);
	u8 *exe = new u8[s];
	fseek(in, 0, SEEK_SET);
	fread(exe, s, 1, in);

	// begin extraction
	u16 *_ptr = (u16*)&exe[ptr];
	u8 *str = &exe[text];
	XMLDocument xml;
	xml.SetBOM(true);

	XMLElement *items = xml.NewElement("Strings");
	// extract text
	for (int i = 0; i < count; i++)
	{
		std::string s = DecodeStringEU(&str[_ptr[i]]);
		XMLElement *sub = xml.NewElement("Text");
		sub->SetText(s.c_str());
		items->InsertEndChild(sub);
	}

	xml.InsertEndChild(items);
	xml.SaveFile(out_name);

	delete[] exe;
}

typedef struct tagBranchData
{
	u8 count,
		x,
		width,
		reserve;	// always zero
	u16 string;
} BRANCH_DATA;

void ExtractTextChoice(LPCSTR exe_name, LPCSTR out_name, u32 ptr, u32 text, u32 data, int count, bool bJapanese)
{
	// cache data
	FILE *in = fopen(exe_name, "rb+");
	fseek(in, 0, SEEK_END);
	size_t s = ftell(in);
	u8 *exe = new u8[s];
	fseek(in, 0, SEEK_SET);
	fread(exe, s, 1, in);

	// begin extraction
	u16 *_ptr = (u16*)&exe[ptr];
	if (!bJapanese) _ptr = &_ptr[1];
	u8 *str = &exe[text];
	BRANCH_DATA *pData = (BRANCH_DATA*)&exe[data];

	XMLDocument xml;
	xml.SetBOM(true);
	XMLElement *items = xml.NewElement("Choices");
	// extract text
	for (int i = 0; i < count; i++)
	{
		XMLElement *choice = xml.NewElement("Choice");
		choice->SetAttribute("Base_x", pData[i].x);
		choice->SetAttribute("Width", pData[i].width);
		for (int j = 0, sj = pData[i].count * 2; j < sj; j+=2)
		{
			std::string s = DecodeString(&str[_ptr[j + pData[i].string * 2]]);
			XMLElement *sub = xml.NewElement("String");
			sub->SetText(s.c_str());
			choice->InsertEndChild(sub);
		}
		items->InsertEndChild(choice);
	}

	xml.InsertEndChild(items);
	xml.SaveFile(out_name);

	delete[] exe;
}

void ExtractTextChoiceEU(LPCSTR exe_name, LPCSTR out_name, u32 ptr, u32 text, u32 data, int count)
{
	// cache data
	FILE *in = fopen(exe_name, "rb+");
	fseek(in, 0, SEEK_END);
	size_t s = ftell(in);
	u8 *exe = new u8[s];
	fseek(in, 0, SEEK_SET);
	fread(exe, s, 1, in);

	// begin extraction
	u16 *_ptr = (u16*)&exe[ptr];
	u8 *str = &exe[text];
	BRANCH_DATA *pData = (BRANCH_DATA*)&exe[data];

	XMLDocument xml;
	xml.SetBOM(true);
	XMLElement *items = xml.NewElement("Choices");
	// extract text
	for (int i = 0; i < count; i++)
	{
		XMLElement *choice = xml.NewElement("Choice");
		choice->SetAttribute("Base_x", pData[i].x);
		choice->SetAttribute("Width", pData[i].width);
		for (int j = 0, sj = pData[i].count; j < sj; j++)
		{
			std::string s = DecodeStringEU(&str[_ptr[j + pData[i].string]]);
			XMLElement *sub = xml.NewElement("String");
			sub->SetText(s.c_str());
			choice->InsertEndChild(sub);
		}
		items->InsertEndChild(choice);
	}

	xml.InsertEndChild(items);
	xml.SaveFile(out_name);

	delete[] exe;
}

void ExtractRoomText(LPCSTR room_name, LPCSTR out_name)
{
	CFile file;
	file.Open(room_name);

	if (!file.IsOpen()) return;

	u8 *buffer = new u8[file.GetSize()];
	file.Read(buffer, file.GetSize());

	RDT_HEADER *head = (RDT_HEADER*)buffer;

	if (head->pMessage[1] != 0)
	{
		u8 *text = &buffer[head->pMessage[1]];
		u16 *ptr = (u16*)text;

		XMLDocument xml;
		xml.SetBOM(true);
		XMLElement *items = xml.NewElement("Strings");

		for (int i = 0, si = ptr[0] / 2; i < si; i++)
		{
			std::string s = DecodeString(&text[ptr[i]]);
			XMLElement *sub = xml.NewElement("Text");
			sub->SetText(s.c_str());
			items->InsertEndChild(sub);
		}

		xml.InsertEndChild(items);
		xml.SaveFile(out_name);
	}

	delete[] buffer;
}

void ExtractRoomTextEU(LPCSTR room_name, LPCSTR out_name)
{
	CFile file;
	file.Open(room_name);

	if (!file.IsOpen()) return;

	u8 *buffer = new u8[file.GetSize()];
	file.Read(buffer, file.GetSize());

	RDT_HEADER *head = (RDT_HEADER*)buffer;

	if (head->pMessage[0] != 0)
	{
		u8 *text = &buffer[head->pMessage[0]];
		u16 *ptr = (u16*)text;

		XMLDocument xml;
		xml.SetBOM(true);
		XMLElement *items = xml.NewElement("Strings");

		for (int i = 0, si = ptr[0] / 2; i < si; i++)
		{
			std::string s = DecodeStringEU(&text[ptr[i]]);
			XMLElement *sub = xml.NewElement("Text");
			sub->SetText(s.c_str());
			items->InsertEndChild(sub);
		}

		xml.InsertEndChild(items);
		xml.SaveFile(out_name);
	}

	delete[] buffer;
}

char ToStage(int Stage)
{
	if (Stage >= 1 && Stage <= 0xa)
		return '0' + Stage;
	if (Stage >= 0xa)
		return 'a' + Stage - 0xa;

	return '0';
}

enum SEG
{
	SEG_EDT0,
	SEG_VH0,
	SEG_VB0,
	SEG_PADD0,
	SEG_PADD1,
	SEG_RBJEND,
	SEG_SCA,
	SEG_RCUT,
	SEG_VCUT,
	SEG_LIGHT,
	SEG_MODEL,
	SEG_FLOOR,
	SEG_BLOCK,
	SEG_MES0,
	SEG_MES1,
	SEG_SCD0,
	SEG_SCD1,
	SEG_SCD2,
	SEG_ESPHEAD,
	SEG_ESPEND,
	SEG_ESPTIM,
	SEG_ESPTIMEND,
	SEG_RBJ,
	SEG_END
};

LPCSTR seg_ext[] =
{
	"edt0", "vh0", "vb0", "", "", "rbj_end", "sca", "rcut", "vcut",
	"lit", "mdl", "flr", "blk", "ms0", "ms1", "sc0", "sc1", "sc2",
	"esph", "espe", "esptim", "esptime", "rbj"
};

typedef struct PAIR_SEG
{
	DWORD ptr;
	SEG index;
} PAIR_SEG;

bool sort_seg(PAIR_SEG& a, PAIR_SEG& b)
{
	return a.ptr < b.ptr;
}

#define memalign(x, y) ((x + (y - 1)) & ~(y - 1))

void RDT_fix(LPCSTR filename)
{
	CFile f(filename);

	std::vector<BYTE> data = std::vector<BYTE>(f.GetSize());
	f.Read(data.data(), f.GetSize());

	RDT_HEADER* h = (RDT_HEADER*)data.data();

	std::vector<BYTE> segment[23];
	std::vector<PAIR_SEG> ptr = std::vector<PAIR_SEG>(24);

	DWORD* p = (DWORD*)&h->pEdt0;
	for (int i = 0; i < 23; i++)
	{
		ptr[i].ptr = *p++;
		ptr[i].index = (SEG)i;
	}
	ptr[23].ptr = f.GetSize();
	ptr[23].index = SEG_END;

	std::sort(ptr.begin(), ptr.end(), sort_seg);

	// extract segments
	for (int i = 0; i < 23; i++)
	{
		size_t size = ptr[i + 1].ptr - ptr[i].ptr;
		if (size == 0) continue;
		segment[ptr[i].index] = std::vector<BYTE>(memalign(size, 4));
		memcpy(segment[ptr[i].index].data(), data.data() + ptr[i].ptr, size);

		FILE* fp;
		char path[32];
		sprintf_s(path, sizeof(path), "test_rdt\\seg.%s", seg_ext[ptr[i].index]);
		fopen_s(&fp, path, "wb");
		if (fp)
		{
			fwrite(segment[ptr[i].index].data(), segment[ptr[i].index].size(), 1, fp);
			fclose(fp);
		}
	}
}

int main()
{
	//test_mmio();

	FILE *f;
	char name[MAX_PATH];
	char folder[MAX_PATH];

	RDT_fix("ROOM1000.RDT");

	//CreateDirectoryA("RDT", NULL);
	//CreateDirectoryA("exe", NULL);

	//ExtractItemsPlatinumIT("xml_ita\\LeonI.exe", "xml_ita");
	//ExtractTextGenericEU("xml_ita\\LeonI.exe", "xml_ita\\interact.xml", 0x137F38, 0x137B08, 24);
	//ExtractTextGenericEU("xml_ita\\LeonI.exe", "xml_ita\\system.xml", 0x139990, 0x137F68, 116);
	//ExtractTextChoiceEU("xml_ita\\LeonI.exe", "xml_ita\\choice.xml", 0x139AF0, 0x139A78, 0x137200, 7);

	//for (int stage = 1; stage <= 7/*0x10*/; stage++)
	//{
	//	for (int room = 0; room < 32; room++)
	//	{
	//		char str0[MAX_PATH], str1[MAX_PATH];

	//		sprintf_s(str0, sizeof(str0), "re2ita\\Rdi_c\\ROOM%c%02x1.RDT", ToStage(stage), room);
	//		sprintf_s(str1, sizeof(str1), "re2ita\\pl1\\ROOM%c%02x.xml", ToStage(stage), room);

	//		ExtractRoomTextEU(str0, str1);
	//	}
	//}

	for (int stage = 1; stage <= 7/*0x10*/; stage++)
	{
		for (int room = 0; room < 32; room++)
		{
			char str0[MAX_PATH], str1[MAX_PATH];

			sprintf_s(str0, sizeof(str0), "re2ita\\Rdi_l\\ROOM%c%02x0.RDT", ToStage(stage), room);
			sprintf_s(str1, sizeof(str1), "re2ita\\pl0\\ROOM%c%02x.xml", ToStage(stage), room);

			ExtractRoomTextEU(str0, str1);
		}
	}

	//for (int stage = 1; stage < 7; stage++)
	//{
	//	for (int room = 0; room < 0x18; room++)
	//	{
	//		char str0[MAX_PATH], str1[MAX_PATH];

	//		sprintf_s(str0, sizeof(str0), "D:\\Program Files\\BIOHAZARD 2 PC\\mod_destiny\\PL0\\RDT\\ROOM%x%02x0.RDT", stage, room);
	//		sprintf_s(str1, sizeof(str1), "destiny_room\\ROOM%x%02x.xml", stage, room);

	//		ExtractRoomText(str0, str1);
	//	}
	//}

	//ExtractItems();
	
	//ExtractItemsPlatinum("D:\\Program Files\\BIOHAZARD 2 PC\\mod_destiny\\ResidentEvil2 Destiny.EXE", "destiny");
	//ExtractItemsPlatinum("D:\\Program Files\\Merdal Night\\Resident Evil Mortal Night (D.E.) - EP1\\RE Mortal Night (DE) - EP1\\Episode I.exe", "mortal");
	//ExtractTextGeneric("D:\\Program Files\\Merdal Night\\Resident Evil Mortal Night (D.E.) - EP1\\RE Mortal Night (DE) - EP1\\Episode I.exe", "mortal\\system.xml", 0x13A6C0, 0x138F98, 116);
	//ExtractTextGeneric("D:\\Program Files\\Merdal Night\\Resident Evil Mortal Night (D.E.) - EP1\\RE Mortal Night (DE) - EP1\\Episode I.exe", "mortal\\interact.xml", 0x137BA0, 0x137800, 24);
	//ExtractTextChoice("D:\\Program Files\\Merdal Night\\Resident Evil Mortal Night (D.E.) - EP1\\RE Mortal Night (DE) - EP1\\Episode I.exe", "mortal\\choice.xml", 0x13A888, 0x13A7A8, 0x136738, 7, false);
	//ExtractTextGeneric("D:\\Program Files\\BIOHAZARD 2 PC\\mod_destiny\\ResidentEvil2 Destiny.EXE", "destiny\\interact.xml", 0x137BA0, 0x137800, 24);
	//ExtractTextGeneric("D:\\Program Files\\BIOHAZARD 2 PC\\mod_destiny\\ResidentEvil2 Destiny.EXE", "destiny\\system.xml", 0x13A6C0, 0x138F98, 116);

#if 0
	static u32 rdt_tbl[] = { 0x103, 0x107, 0x202 };
	// extract SN main scd
	//for (int i = 0; i < _countof(rdt_tbl); i++)
	//{
	//	sprintf(name, "E:\\Storage\\BIOHAZARD 2 PC\\BIOHAZARD 2 PC\\pl0\\Rdt\\ROOM%03X0.RDT", rdt_tbl[i]);
	//	sprintf(folder, "RDT\\ROOM%03X0_s", rdt_tbl[i]);
	//	ExtractSCD0(name, folder);
	//	ExtractSCD1(name, folder);

	//	sprintf(name, "E:\\Storage\\BIOHAZARD 2 PC\\BIOHAZARD 2 PC\\pl1\\Rdt\\ROOM%03X1.RDT", rdt_tbl[i]);
	//	sprintf(folder, "RDT\\ROOM%03X1_s", rdt_tbl[i]);
	//	ExtractSCD0(name, folder);
	//	ExtractSCD1(name, folder);
	//}
	// extract "fixed" main scd
	for (int i = 0; i < _countof(rdt_tbl); i++)
	{
		sprintf(name, "E:\\ISO\\PSX\\Resident Evil 2 (USA) (Disc 2)\\rdt fixed\\ROOM%03X0.RDT", rdt_tbl[i]);
		sprintf(folder, "RDT\\ROOM%03X0_f", rdt_tbl[i]);
		ExtractSCD0(name, folder);
		ExtractSCD1(name, folder);

		sprintf(name, "E:\\ISO\\PSX\\Resident Evil 2 (USA) (Disc 2)\\rdt fixed\\ROOM%03X1.RDT", rdt_tbl[i]);
		sprintf(folder, "RDT\\ROOM%03X1_f", rdt_tbl[i]);
		ExtractSCD0(name, folder);
		ExtractSCD1(name, folder);
	}
	for (int i = 0; i < _countof(rdt_tbl); i++)
	{
		sprintf(name, "..\\ddraw\\data\\music fix\\ROOM%03X0_0.scd", rdt_tbl[i]);
		sprintf(folder, "RDT\\ROOM%03X0_s", rdt_tbl[i]);
		BuildSCD(folder, name);

		sprintf(name, "..\\ddraw\\data\\music fix\\ROOM%03X0_1.scd", rdt_tbl[i]);
		sprintf(folder, "RDT\\ROOM%03X1_s", rdt_tbl[i]);
		BuildSCD(folder, name);
	}
#endif

	for (int i = 0; i < 1; i++)
	{
		for (int Stage = 0xA; Stage <= 0xF; Stage++)
		{
			for (int Room = 0; Room < 256; Room++)
			{
				RDT_HEADER head;
				sprintf(name, "E:\\Storage\\BIOHAZARD 2 PC\\BIOHAZARD 2 PC\\pl%d\\Rdt\\ROOM%X%02X%d.RDT", i, Stage, Room, i);
				f = fopen(name, "rb+");
				// file doesn't exit? skip
				if (!f) continue;
				fread(&head, sizeof(head), 1, f);
				// doesn't have english text, skip
				if (head.pMessage[1] != 0)
				{
					u8 buffer[2048];
					fseek(f, head.pMessage[1], SEEK_SET);
					fread(buffer, sizeof(buffer), 1, f);

					u16 *ptr = (u16*)buffer;
					int count = ptr[0] / 2;
					u8 *p = &buffer[ptr[count - 1]];
					while (*p++ != 0xfe);
					if (*p == 0) p++;

					sprintf(name, "RDT\\ROOM%X%02X%d.tex", Stage, Room, i);
					FILE *o = fopen(name, "wb+");
					fwrite(buffer, p - buffer, 1, o);
					fclose(o);
				}

				fclose(f);
			}
		}
	}

    return 0;
}

