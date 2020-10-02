#pragma once
#include <vector>

enum RdtMain
{
	RDT_KAN0,
	RDT_VH0,
	RDT_VB0,
	RDT_KAN1,
	RDT_VH1,
	RDT_VB1,
	RDT_SCA,
	RDT_RID,
	RDT_RVD,
	RDT_LIT,
	RDT_OBJ,
	RDT_FLR,
	RDT_BLK,
	RDT_TEX0,
	RDT_TEX1,
	RDT_SCD0,
	RDT_SCD1,
	RDT_SCD2,
	RDT_ESP,
	RDT_EFF,
	RDT_ETIM,
	RDT_MTIM,
	RDT_RBJ,
	RDT_END
};

typedef struct tagU32
{
	int id;
	u32 val;
} U32;

typedef struct tagRdtEnum
{
	RdtMain id;
	u32 val;
} RDT_ENUM;

typedef struct tagRdtHeader
{
	char nSprite;
	char nCut;			// Number of objects at offset 7, seems to be number of cameras of the room
	char nOmodel;		// Number of objects at offset 10
	char nItem;			// unused?
	char nDoor;			// unused?
	char nRoom_at;		// unused?
	char Reverb_lv;		// unused?
	u8 nSprite_max;		// 0x08
	u32 pEdt0,			// 0x10
		pVh0,			// 0x14
		pVb0;			// 0x18
	u32 kan1,			// 0x1C
		vh1,			// 0x20
		vb1;			// 0x24
	u32 sca;
	u32 rid;			// 
	u32 rvd;			// 0x28
	u32 lit;			// 0x2C
	u32 obj;			// 0x30
	u32 flr, blk;		// 0x34, 0x38
	u32 tex[2];			// 0x3c, 0x40
	u32 scd[3];			// 0x44, 0x48, 0x4C
	u32 pEsp, pEff;		// 0x50, 0x54
	u32 pTim[2];		// 0x58, 0x5C
	u32 rbj;			// 0x60
} RDT_HEADER;

class CRoom
{
public:
	CRoom()
	{
		scd[0] = scd[1] = nullptr;
	}
	~CRoom()
	{
		if (scd[0]) delete[] scd[0];
		if (scd[1]) delete[] scd[1];
	}

	void Open(LPCSTR filename);
	void DumpScd(LPCSTR folder_out);

	RDT_HEADER head;
	std::vector<RDT_ENUM> main;
	std::vector<U32> scd_ptr[2];

	u8 *scd[2];
	size_t scd_size[2];
};

void AnalyseScd(u8 *pScd, int size);
