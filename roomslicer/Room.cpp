#define _CRT_SECURE_NO_WARNINGS

#include <stdafx.h>
#include "Room.h"
#include <algorithm>

//CRoom::CRoom()
//{
//}
//
//CRoom::~CRoom()
//{
//}

typedef struct tagU16
{
	u16 id;
	u16 val;
} U16;

bool sort_u32(U32 &a, U32 &b)
{
	return a.val < b.val;
}

bool sort_rdt_enum(RDT_ENUM &a, RDT_ENUM &b)
{
	return a.val < b.val;
}

bool sort_u16(U16 &a, U32 &b)
{
	return a.val < b.val;
}

int FindByType(RdtMain find, std::vector<RDT_ENUM> &tbl)
{
	for (size_t i = 0, si = tbl.size(); i < si; i++)
		if (tbl[i].id == find /*&& tbl[i].val != 0*/)
			return (int)i;

	return -1;
}

void CRoom::Open(LPCSTR filename)
{
	main.clear();
	scd_ptr[0].clear();
	scd_ptr[1].clear();

	CBufferFile f;
	if (!f.Open(filename))
		return;

	RDT_HEADER *h = (RDT_HEADER*)f.data;
	memcpy(&head, h, sizeof(head));

	// gather all RDT pointers and sort them in increasing number
	// this will be needed to spit out the correct data
	RDT_ENUM p;
	for (int i = 0, si = ((int)&h[1] - (int)&h->pEdt0) / 4; i < si; i++)
	{
		u32 val = ((u32*)(&h->pEdt0))[i];
		if (!val) continue;
		p.id = (RdtMain)i;
		p.val = val;
		main.push_back(p);
	}

	// add more shit to pointer master table
	if (h->obj)
	{
		u32 *ptr = (u32*)&f.data[h->obj];
		for (int i = 0, si = h->nOmodel * 2; i < si; i++)
		{
			p.id = (RdtMain)-1;
			p.val = ptr[i];
			main.push_back(p);
		}
	}

	p.id = RDT_END;
	p.val = f.size;
	main.push_back(p);
	std::sort(main.begin(), main.end(), sort_rdt_enum);

	for (int i = 0; i < 2; i++)
	{
		if (h->scd[i + 1] == 0) continue;
		// add pointers and sort them
		u16 *ptr = (u16*)&f.data[h->scd[i + 1]];
		for (int j = 0, sj = ptr[0] / 2; j < sj; j++)
		{
			U32 p;
			p.id = j;
			p.val = ptr[j];
			scd_ptr[i].push_back(p);
		}
		std::sort(scd_ptr[i].begin(), scd_ptr[i].end(), sort_u32);
	}

	//size_t scd_size[2];
	int found;
	RdtMain type;
	
	int t0 = FindByType(RDT_SCD1, main);
	if (t0 != -1)
	{
		scd_size[0] = main[t0 + 1].val - main[t0].val;
		U32 p;
		p.id = -1;
		p.val = scd_size[0];
		scd_ptr[0].push_back(p);

		scd[0] = new u8[scd_size[0]];
		memcpy(scd[0], &f.data[h->scd[1]], scd_size[0]);

		AnalyseScd(&scd[0][scd_ptr[0][0].val], scd_ptr[0][1].val - scd_ptr[0][0].val);
	}
	else scd_size[0] = 0;

	t0 = FindByType(RDT_SCD2, main);
	if (t0 != -1)
	{
		type = RDT_ESP;

		scd_size[1] = main[t0 + 1].val - main[t0].val;
		U32 p;
		p.id = -1;
		p.val = scd_size[1];
		scd_ptr[1].push_back(p);

		scd[1] = new u8[scd_size[1]];
		memcpy(scd[1], &f.data[h->scd[2]], scd_size[1]);
	}
	else scd_size[1] = 0;
}

void CRoom::DumpScd(LPCSTR folder_out)
{
	char path[MAX_PATH];

	if (scd[0])
	{
		sprintf_s(path, sizeof(path), "%s_0.scd", folder_out);
		FILE *fout = fopen(path, "wb+");
		fwrite(scd[0], scd_size[0], 1, fout);
		fclose(fout);
	}
	if (scd[1])
	{
		sprintf_s(path, sizeof(path), "%s_1.scd", folder_out);
		FILE *fout = fopen(path, "wb+");
		fwrite(scd[1], scd_size[1], 1, fout);
		fclose(fout);
	}
}
