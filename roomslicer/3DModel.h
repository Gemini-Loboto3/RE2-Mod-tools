#pragma once
#include <stdafx.h>
#include <vector>

enum ModelType
{
	MT_PLD,	// 1.5 pld
	MT_EMD	// 1.5 emd
};

#ifndef ClearPtrVector
	#define ClearPtrVector(x)	while(!x.empty()) {	delete x.back(); x.pop_back(); }
#endif

typedef struct tagVertex
{
	float x, y, z;
} VERTEX;

typedef struct anim_reference
{
	//u32 crc;		// crc associated with an EMR_ENTRY
	//int newid;		// fill with C3DAnimation::BuildReference()
	int group;		// frame group index
	int index;		// frame entry index
	int length;
} ANIM_REFERENCE;

//////////////////////////////////
// EDD - animation directives   //
//////////////////////////////////
typedef struct tagEddHeader
{
	u16 count;
	u16 ptr;
} EDD_HEADER;

typedef struct tagEddEntry
{
	u32 index:12;	/* EMR index */
	u32 length:10;	/* seems related to how speed is handled */
	u32 snd:10;		/* triggers a sound */
} EDD_ENTRY;

typedef struct tagEddHeader1
{
	u16 count;
	u16 ptr;
} EDD_HEADER1;

typedef struct tagEddEntry1
{
	u16 index;
	u16 length;
} EDD_ENTRY1;

typedef struct tagEddHeader2
{
	u16 count;		/* how many frames in the animation */
	u16 ptr;		/* where the sequence is store in the EDD bank */
	u16 frame;		/* starting frame in the EMR bank */
	u16 pad;		/* reserved, always 0 */
} EDD_HEADER2;

typedef struct tagEddEntry2
{
	u16 index:7;	/* relative index in the EMR bank */
	u16 jump:1;		/* when this is true, skip the frame */
	u16 snd:8;		/* seems more like a sound to trigger */
} EDD_ENTRY2;

//////////////////////////////////
// EMR - animation resources	//
//////////////////////////////////
#undef EMR_HEADER		// suppress error with WinGDI.h using this as a macro

typedef struct tagEmrHeader
{
    u16 offset;			/* Relative offset to emd_sec2_relpos[] array */
    u16 length;			/* Relative offset to EMR_ENTRY[] array, which is also length of relpos+armature+mesh numbers  */
    u16 count;			/* Number of objects in the mesh */
    u16 size;			/* Size of each element */
} EMR_HEADER, EMRR_HEADER;

/* Relative position of each mesh in the object */
typedef struct tagEmrJoint
{
    s16 x;
    s16 y;
    s16 z;
} EMR_JOINT/*, EMR_ANGLE*/;

typedef struct tagEmrAngle
{
	s16 x, y, z;
} EMR_ANGLE;

typedef struct tagEmrArmature
{
    u16 mesh_count;		/* Number of meshes linked to this one */
    u16 offset;			/* Relative offset to mesh numbers (emc_sec2_mesh[] array) */
} EMR_ARMATURE;

typedef struct tagEmrEntry
{
	s16 x_offset;	/* distance from reference point */
	s16 y_offset;
	s16 z_offset;

	s16 x_speed;	/* speed at which moving the model */
	s16 y_speed;
	s16 z_speed;
} EMR_ENTRY;

typedef struct tagEmrEntry2
{
	s16 x_speed;	/* speed at which moving the model */
	s16 y_speed;
	s16 z_speed;

	s16 y_offset;
} EMR_ENTRY2;

//////////////////////////////////
// MD1 - RE2 3d model data		//
//////////////////////////////////
typedef struct tagMd1Header
{
	u32 length;		/* Section length in bytes */
	u32 unknown;
	int obj_count;	/* Number of objects in model */
} MD1_HEADER;

typedef struct tagMd1HeaderTrial
{
	u32 length;		/* Section length in bytes */
	int obj_count;	/* Number of objects in model */
	u32 unknown;
} MD1_HEADER_TRIAL;

typedef struct tagMd1Triangles
{
	u32 vertex_offset;		/* Offset to vertex data, array of MD1_VERTEX */
	u32 vertex_count;		/* Vertex count */
	u32 normal_offset;		/* Offset to normal data, array of MD1_VERTEX */
	u32 normal_count;		/* Normal count */
	u32 tri_offset;			/* Offset to triangle data, array of MD1_TRIANGLE */
	u32 tri_count;			/* Triangle count */
	u32 tri_map_offset;	/* Offset to triangle texture data, array of MD1_TRIANGLE_MAP */
} MD1_TRIANGLES;

typedef struct tagMd1Quads
{
	u32 vertex_offset;		/* Offset to vertex data, array of MD1_VERTEX */
	u32 vertex_count;		/* Vertex count */
	u32 normal_offset;		/* Offset to normal data, array of MD1_VERTEX */
	u32 normal_count;		/* Normal count */
	u32 quad_offset;		/* Offset to quad index data, array of MD1_QUAD */
	u32 quad_count;			/* Quad count */
	u32 quad_map_offset;	/* Offset to quad texture data, array of MD1_QUAD_MAP */
} MD1_QUADS;

typedef struct tagMd1Object
{
    MD1_TRIANGLES triangles;
    MD1_QUADS quads;
} MD1_OBJECT;

typedef struct tagMd2Object
{
	u32 vertex_offset;
	u32 normal_offset;
	u32 vertex_count;
	u32 tri_offset;
	u32 quad_offset;
	u16 tri_count;
	u16 quad_count;
} MD2_OBJECT;

typedef struct tagMd1Vertex
{
	s16 x;
	s16 y;
	s16 z;
	s16 reserve;
} MD1_VERTEX, MD2_VERTEX;

/* Triangle */
typedef struct tagMd1Triangle
{
	u16 n0;					/* Index of normal data for vertex 0 */
	u16 v0;					/* Index of vertex data for vertex 0 */
	u16 n1;
	u16 v1;
	u16 n2;
	u16 v2;
} MD1_TRIANGLE;

/* Triangle texture information */
typedef struct tagMd1TriangleMap
{
	u8 u0;					/* u,v texture coordinates of vertex 0 */
	u8 v0;
	u16 clutid;				/* Texture clut id, bits 0-5 */
	u8 u1;
	u8 v1;
	u16 page;				/* Texture page */
	u8 u2;
	u8 v2;
	u16 zero;
} MD1_TRIANGLE_MAP;

/* Quad */
typedef struct tagMd1Quad
{
    u16 n0;					/* Index of normal data for vertex 0 */
    u16 v0;					/* Index of vertex data for vertex 0 */
    u16 n1;					/* Note: a quad is 2 triangles: v0,v1,v3 and v1,v3,v2 */
    u16 v1;					/*  If you draw it directly, uses v0,v1,v3,v2 for a right order */    
    u16 n2;
    u16 v2;
    u16 n3;
    u16 v3;    
} MD1_QUAD;

/* Quad texture information */
typedef struct tagMd1QuadMap
{
    u8 u0;					/* u,v texture coordinates of vertex 0 */
    u8 v0;
    u16 clutid;				/* Texture clut id, bits 0-5 */
    u8 u1;
    u8 v1;
    u16 page;				/* Texture page */
    u8 u2;
    u8 v2;
    u16 zero1;
    u8 u3;
    u8 v3;
    u16 zero2;
} MD1_QUAD_MAP;

typedef struct tagMd1QuadMapEx
{
    u16 u0;					/* u,v texture coordinates of vertex 0 */
    u16 v0;
    u16 clutid;				/* Texture clut id, bits 0-5 */
    u16 u1;
    u16 v1;
    u16 page;				/* Texture page */
    u16 u2;
    u16 v2;
    u16 u3;
    u16 v3;
} MD1_QUAD_MAP_EX;

//////////////////////////////////
// 3D FORMAT WRAPPERS			//
//////////////////////////////////

/* holds 3D geometry data for an object in a model */
class CModelObject
{
public:
	CModelObject() {}

	void Copy(CModelObject &obj)
	{
		vertex.clear();
		normal.clear();
		tri.clear();
		tri_map.clear();
		quad.clear();
		quad_map.clear();
		quad_map_ex.clear();

		vertex=obj.vertex;
		normal=obj.normal;
		tri=obj.tri;
		tri_map=obj.tri_map;
		quad=obj.quad;
		quad_map=obj.quad_map;
		quad_map_ex=obj.quad_map_ex;
	}

	void operator = (const CModelObject &obj)
	{
		vertex.clear();
		normal.clear();
		tri.clear();
		tri_map.clear();
		quad.clear();
		quad_map.clear();
		quad_map_ex.clear();

		vertex=obj.vertex;
		normal=obj.normal;
		tri=obj.tri;
		tri_map=obj.tri_map;
		quad=obj.quad;
		quad_map=obj.quad_map;
		quad_map_ex=obj.quad_map_ex;
	}

	void Strip();

	std::vector<MD1_VERTEX> vertex;
	std::vector<MD1_VERTEX> normal;

	std::vector<MD1_TRIANGLE> tri;
	std::vector<MD1_TRIANGLE_MAP> tri_map;

	std::vector<MD1_QUAD> quad;
	std::vector<MD1_QUAD_MAP> quad_map;
	std::vector<MD1_QUAD_MAP_EX> quad_map_ex;
};

/* necessary after parsing nsbmd gl calls */
void Strip(CModelObject *_tri, CModelObject *_quad);

/* holds all geometry data for a 3D model */
class CModel
{
public:
	CModel() {}
	~CModel() { Reset();}

	void Copy(CModel &model)
	{
		Reset();
		for(int i=0; i<(int)model.tri.size(); i++)
		{
			CModelObject *t=new CModelObject;
			t->Copy(*model.tri[i]);
			tri.push_back(t);
		}
		for(int i=0; i<(int)model.quad.size(); i++)
		{
			CModelObject *q=new CModelObject;
			q->Copy(*model.quad[i]);
			quad.push_back(q);
		}
	}

	void Reset() { ClearPtrVector(tri); ClearPtrVector(quad); }

	int OpenMd1(LPCSTR filename);
	int OpenMd1(u8* data);

	int Write(LPCSTR filename);
	int Write(u8* &out);

	void ReadGeometry(u8 *data, MD1_OBJECT *obj, int count);		// RE2

	std::vector<CModelObject*> tri;
	std::vector<CModelObject*> quad;
};
