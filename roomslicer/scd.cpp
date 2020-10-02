#include <stdafx.h>

typedef struct tagOpcode
{
	int size;	// opcode size
} OPCODE;

static OPCODE op_tbl[143]=
{
	{ 1 },	//.word Nop                # 0
	{ 1 },	//.word Evt_end            # 1
	{ 1 },	//.word Evt_next           # 2
	{ 4 },	//.word Evt_chain          # 3
	{ 4 },	//.word Evt_exec           # 4
	{ 2 },	//.word Evt_kill           # 5
	{ 4 },	//.word Ifel_ck            # 6
	{ 4 },	//.word Else_ck            # 7
	{ 2 },	//.word Endif              # 8
	{ 1 },	//.word Sleep              # 9
	{ 3 },	//.word Sleeping           # 10
	{ 1 },	//.word Wsleep             # 11
	{ 1 },	//.word Wsleeping          # 12
	{ 6 },	//.word For                # 13
	{ 2 },	//.word Next               # 14
	{ 4 },	//.word While              # 15
	{ 1 },	//.word Ewhile             # 16
	{ 4 },	//.word Do                 # 17
	{ 1 },	//.word Edwhile           # 18
	{ 4 },	//.word Switch             # 19
	{ 6 },	//.word Case               # 20
	{ 2 },	//.word Default            # 21
	{ 2 },	//.word Eswitch            # 22
	{ 6 },	//.word Goto               # 23
	{ 2 },	//.word Gosub              # 24
	{ 1 },	//.word Return             # 25
	{ 1 },	//.word Break              # 26
	{ 6 },	//.word For2               # 27
	{ 1 },	//.word Break_point        # 28
	{ 4 },	//.word Work_copy          # 29
	{ 1 },	//.word Nop                # 30
	{ 1 },	//.word Nop                # 31
	{ 1 },	//.word Nop                # 32
	{ 4 },	//.word Ck                 # 33
	{ 4 },	//.word Set                # 34
	{ 6 },	//.word Cmp                # 35
	{ 4 },	//.word Save               # 36
	{ 3 },	//.word Copy               # 37
	{ 6 },	//.word Calc               # 38
	{ 4 },	//.word Calc2              # 39
	{ 1 },	//.word Sce_rnd            # 40
	{ 2 },	//.word Cut_chg            # 41
	{ 1 },	//.word Cut_old            # 42
	{ 6 },	//.word Message_on         # 43
	{ 20 },	//.word Aot_set            # 44
	{ 38 },	//.word Obj_model_set      # 45
	{ 3 },	//.word Work_set           # 46
	{ 4 },	//.word Speed_set          # 47
	{ 1 },	//.word Add_speed          # 48
	{ 1 },	//.word Add_aspeed         # 49
	{ 8 },	//.word Pos_set            # 50
	{ 8 },	//.word Dir_set            # 51
	{ 4 },	//.word Member_set         # 52
	{ 3 },	//.word Member_set2        # 53
	{ 12 },	//.word Se_on              # 54
	{ 4 },	//.word Sca_id_set         # 55
	{ 3 },	//.word Flr_set            # 56
	{ 8 },	//.word Dir_ck             # 57
	{ 16 },	//.word Sce_espr_on        # 58
	{ 32 },	//.word Door_aot_set       # 59
	//.word Cut_auto           # 60
	//.word Member_copy        # 61
	//.word Member_cmp         # 62
	//.word Plc_motion         # 63
	//.word Plc_dest           # 64
	//.word Plc_neck           # 65
	//.word Plc_ret            # 66
	//.word Plc_flg            # 67
	//.word Sce_em_set         # 68
	//.word Col_chg_set        # 69
	//.word Aot_reset          # 70
	//.word Aot_on             # 71
	//.word Super_set          # 72
	//.word Super_reset        # 73
	//.word Plc_gun            # 74
	//.word Cut_replace        # 75
	//.word Sce_espr_kill      # 76
	//.word Door_model_set     # 77
	//.word Item_aot_set       # 78
	//.word Sce_key_ck         # 79
	//.word Sce_trg_ck         # 80
	//.word Sce_bgm_control    # 81
	//.word Sce_espr_control   # 82
	//.word Sce_fade_set       # 83
	//.word Sce_espr3d_on      # 84
	//.word Member_calc        # 85
	//.word Member_calc2       # 86
	//.word Sce_bgmtbl_set     # 87
	//.word Plc_rot            # 88
	//.word Xa_on              # 89
	//.word Weapon_chg         # 90
	//.word Plc_cnt            # 91
	//.word Sce_shake_on       # 92
	//.word Mizu_div_set       # 93
	//.word Keep_Item_ck       # 94
	//.word Xa_vol             # 95
	//.word Kage_set           # 96
	//.word Cut_be_set         # 97
	//.word Sce_Item_lost      # 98
	//.word Plc_gun_eff        # 99
	//.word Sce_espr_on2       # 100
	//.word Sce_espr_kill2     # 101
	//.word Plc_stop           # 102
	//.word Aot_set_4p         # 103
	//.word Door_aot_set_4p    # 104
	//.word Item_aot_set_4p    # 105
	//.word Light_pos_set      # 106
	//.word Light_kido_set     # 107
	//.word Rbj_reset          # 108
	//.word Sce_scr_move       # 109
	//.word Parts_set          # 110
	//.word Movie_on           # 111
	//.word Splc_ret           # 112
	//.word Splc_sce           # 113
	//.word Super_on           # 114
	//.word Mirror_set         # 115
	//.word Sce_fade_adjust    # 116
	//.word Sce_espr3d_on2     # 117
	{ 3 },	//.word Sce_Item_get       # 118
	//.word Sce_line_start     # 119
	//.word Sce_line_main      # 120
	//.word Sce_line_end       # 121
	//.word Sce_parts_bomb     # 122
	//.word Sce_parts_down     # 123
	//.word Light_color_set    # 124
	//.word Light_pos_set2     # 125
	//.word Light_kido_set2    # 126
	//.word Light_color_set2   # 127
	//.word Se_vol             # 128
};

typedef struct tagInItemWork
{
	u8 Item_id;
	u8 pad;
	u16 Num;
	u16 m45;
	u8 flg;
	u8 m7;
} IN_ITEM_WORK;

typedef struct tagOpcodeAotItem
{
	u8 Id, Type;
	u8 nFloor, Super;
	s16 X, Z;
	u16 W, D;
	IN_ITEM_WORK in_item;
} SCE_ITEM_WORK;

typedef struct tagOpcodeAotItemEx
{
	u8 Id, Type;
	u8 nFloor, Super;
	s16 Xz[4][2];
	IN_ITEM_WORK in_item;
} SCE_ITEM_WORK_4P;

void AnalyseScd(u8 *pScd, int size)
{
	for (int pos = 0; pos < size;)
	{
		// do analysis
		switch (pScd[pos])
		{
		case 45:	// Obj_model_set
		{
			struct ob
			{
				u8 op, Om_no;
				u8 Id, Ctr, Wait, Num,
					nFloor, Super;
				u16 Type, Be_flg, Attribute;
				s16 Pos_x, Pos_y, Pos_z;
				s16 Cdir_x, Cdir_y, Cdir_z;
				s16 Ofs_x, Ofs_y, Ofs_z;
				u16 At_w, At_h, At_d;
			} *c = (struct ob*)&pScd[pos];
		}
		break;
		case 78:	// Item_aot_set
		{
			struct iset
			{
				u8 op, sid;
				SCE_ITEM_WORK iw;
				u8 obx, flg;
			} *c = (struct iset*)&pScd[pos];
		}
		break;
		case 105:	// Item_aot_set_4p
		{
			struct iset
			{
				u8 op, sid;
				SCE_ITEM_WORK_4P iw;
				u8 obx, flg;
			} *c = (struct iset*)&pScd[pos];
		}
		break;
		}

		pos += op_tbl[pScd[pos]].size;
	}
}
