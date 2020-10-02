#include <stdafx.h>
#include "3DModel.h"
#include "memstream.h"
#include "crc.h"
#include <list>
#include <set>

#define FAST_STRIP	1
#define ALT_STRIP	1

int FindIndexInHash(std::vector<u32> &hash_tbl, u32 hash)
{
	for(size_t i = 0, si = hash_tbl.size(); i < si; i++)
		if(hash == hash_tbl[i])
			return i;

	return -1;
}

////////////////////////////////////////
inline int IsVertexEqual(MD1_VERTEX *a, MD1_VERTEX *b)
{
	if(a->x==b->x && a->y==b->y && a->z==b->z) return 1;

	return 0;
}

void CreateVertexPool(std::vector<MD1_VERTEX> &src, std::vector<MD1_VERTEX> &dst)
{
	for(int i=0, si=(int)src.size(); i<si; i++)
	{
		int found=-1;
		// search for an identical vertex
		for(int j=0, sj=(int)dst.size(); j<sj; j++)
		{
			if(IsVertexEqual(&src[i],&dst[j]))
			{
				found=j;
				break;
			}
		}
		if(found==-1) dst.push_back(src[i]);
	}
}

int SearchVertexPool(std::vector<MD1_VERTEX> &pool, MD1_VERTEX *v)
{
	for(int i=0, si=(int)pool.size(); i<si; i++)
		if(IsVertexEqual(&pool[i],v))
			return i;
	// nothing was found
	return 0;
}

void Strip(CModelObject *_tri, CModelObject *_quad)
{
	// pool of new verteces
	std::vector<MD1_VERTEX> _v, _n;
	std::vector<MD1_VERTEX> lv, ln;

	// merge vertex lists
	for(int j=0, sj=(int)_tri->vertex.size(); j<sj; j++)
	{
		lv.push_back(_tri->vertex[j]);
		ln.push_back(_tri->normal[j]);
	}
	for(int j=0, sj=(int)_quad->vertex.size(); j<sj; j++)
	{
		lv.push_back(_quad->vertex[j]);
		ln.push_back(_quad->normal[j]);
	}

	// create a pool of new unique verteces
	CreateVertexPool(lv,_v);
	// create a pool of new unique normals
	CreateVertexPool(ln,_n);

	// now fill the map data with the new vertex
	// TRIANGLES
	for(int i=0, si=(int)_tri->tri.size(); i<si; i++)
	{
		MD1_TRIANGLE *_t=&_tri->tri[i];
		_t->v0=SearchVertexPool(_v,&_tri->vertex[_t->v0]);
		_t->v1=SearchVertexPool(_v,&_tri->vertex[_t->v1]);
		_t->v2=SearchVertexPool(_v,&_tri->vertex[_t->v2]);
		_t->n0=SearchVertexPool(_n,&_tri->normal[_t->n0]);
		_t->n1=SearchVertexPool(_n,&_tri->normal[_t->n1]);
		_t->n2=SearchVertexPool(_n,&_tri->normal[_t->n2]);
	}
	// QUADS
	for(int i=0, si=(int)_quad->quad.size(); i<si; i++)
	{
		_quad->quad[i].v0=SearchVertexPool(_v,&_quad->vertex[_quad->quad[i].v0]);
		_quad->quad[i].v1=SearchVertexPool(_v,&_quad->vertex[_quad->quad[i].v1]);
		_quad->quad[i].v2=SearchVertexPool(_v,&_quad->vertex[_quad->quad[i].v2]);
		_quad->quad[i].v3=SearchVertexPool(_v,&_quad->vertex[_quad->quad[i].v3]);
		_quad->quad[i].n0=SearchVertexPool(_n,&_quad->normal[_quad->quad[i].n0]);
		_quad->quad[i].n1=SearchVertexPool(_n,&_quad->normal[_quad->quad[i].n1]);
		_quad->quad[i].n2=SearchVertexPool(_n,&_quad->normal[_quad->quad[i].n2]);
		_quad->quad[i].n3=SearchVertexPool(_n,&_quad->normal[_quad->quad[i].n3]);
	}

	// replace verteces and normals
	_tri->vertex=_v;
	_tri->normal=_n;
	_quad->vertex=_v;
	_quad->normal=_n;
}

void CModelObject::Strip()
{
	// pool of new verteces
	std::vector<MD1_VERTEX> _v, _n;

	// create a pool of new unique verteces
	CreateVertexPool(vertex,_v);
	// create a pool of new unique normals
	CreateVertexPool(normal,_n);

	// now fill the map data with the new vertex
	// TRIANGLES
	for(int i=0, si=(int)tri.size(); i<si; i++)
	{
		tri[i].v0=SearchVertexPool(_v,&vertex[tri[i].v0]);
		tri[i].v1=SearchVertexPool(_v,&vertex[tri[i].v1]);
		tri[i].v2=SearchVertexPool(_v,&vertex[tri[i].v2]);
		tri[i].n0=SearchVertexPool(_n,&normal[tri[i].n0]);
		tri[i].n1=SearchVertexPool(_n,&normal[tri[i].n1]);
		tri[i].n2=SearchVertexPool(_n,&normal[tri[i].n2]);
	}
	// QUADS
	for(int i=0, si=(int)quad.size(); i<si; i++)
	{
		quad[i].v0=SearchVertexPool(_v,&vertex[quad[i].v0]);
		quad[i].v1=SearchVertexPool(_v,&vertex[quad[i].v1]);
		quad[i].v2=SearchVertexPool(_v,&vertex[quad[i].v2]);
		quad[i].v3=SearchVertexPool(_v,&vertex[quad[i].v3]);
		quad[i].n0=SearchVertexPool(_n,&normal[quad[i].n0]);
		quad[i].n1=SearchVertexPool(_n,&normal[quad[i].n1]);
		quad[i].n2=SearchVertexPool(_n,&normal[quad[i].n2]);
		quad[i].n3=SearchVertexPool(_n,&normal[quad[i].n3]);
	}

	// replace verteces and normals
	vertex=_v;
	normal=_n;
}

void CModel::ReadGeometry(u8 *data, MD1_OBJECT *obj, int count)
{
	for(int i=0; i<count; i++, obj++)
	{
		CModelObject *_tri=new CModelObject;
		CModelObject *_quad=new CModelObject;

		// TRIANGLES
		// vertexes
		MD1_VERTEX *v=(MD1_VERTEX*)&data[obj->triangles.vertex_offset];
		for(u32 j=0; j<obj->triangles.vertex_count; j++)
			_tri->vertex.push_back(v[j]);
		// normals
		v=(MD1_VERTEX*)&data[obj->triangles.normal_offset];
		for(u32 j=0; j<obj->triangles.normal_count; j++)
			_tri->normal.push_back(v[j]);
		// triangles
		MD1_TRIANGLE *t=(MD1_TRIANGLE*)&data[obj->triangles.tri_offset];
		MD1_TRIANGLE_MAP *tm=(MD1_TRIANGLE_MAP*)&data[obj->triangles.tri_map_offset];
		for(u32 j=0; j<obj->triangles.tri_count; j++)
		{
			_tri->tri.push_back(t[j]);
			_tri->tri_map.push_back(tm[j]);
		}

		// QUADS
		// vertexes
		v=(MD1_VERTEX*)&data[obj->quads.vertex_offset];
		for(u32 j=0; j<obj->quads.vertex_count; j++)
			_quad->vertex.push_back(v[j]);
		// normals
		v=(MD1_VERTEX*)&data[obj->quads.normal_offset];
		for(u32 j=0; j<obj->quads.normal_count; j++)
			_quad->normal.push_back(v[j]);
		// quads
		MD1_QUAD *q=(MD1_QUAD*)&data[obj->quads.quad_offset];
		MD1_QUAD_MAP *qm=(MD1_QUAD_MAP*)&data[obj->quads.quad_map_offset];
		for(u32 j=0; j<obj->quads.quad_count; j++)
		{
			_quad->quad.push_back(q[j]);
			_quad->quad_map.push_back(qm[j]);
		}

		// add to the model pool
		tri.push_back(_tri);
		quad.push_back(_quad);
	}
}

int CModel::OpenMd1(LPCTSTR filename)
{
	CBufferFile buffer;
	buffer.Open(filename);

	return OpenMd1(buffer.data);
}

int CModel::OpenMd1(u8 *data)
{
	if(!data) return 0;

	Reset();
	MD1_HEADER *h=(MD1_HEADER*)data;
	MD1_OBJECT *obj=(MD1_OBJECT*)&h[1];

	ReadGeometry((u8*)&h[1],obj,h->obj_count/2);

	return 1;
}

typedef struct tagVertexPool
{
	u32 ptr;
	u32 crc;
	int len;
} VERTEX_POOL;

static std::vector<VERTEX_POOL> pool;

#define ClearVertexPool	pool.clear()

u32 ProcessVertex(std::vector<MD1_VERTEX> &vertex, u32 fpos)
{
	VERTEX_POOL v={0,0,0};

	v.len=(int)vertex.size();
	for(int i=0; i<v.len; i++)
		v.crc^=GetCrc32((u8*)&vertex[i],sizeof(MD1_VERTEX));
	v.ptr=fpos;

	// search a vector in the pool
	for(int i=0, is=(int)pool.size(); i<is; i++)
		if(v.crc==pool[i].crc && v.len==pool[i].len)
			return pool[i].ptr;

	// nothing was found, add to pool
	pool.push_back(v);
	return 0;
}

#include "memstream.h"

int CModel::Write(LPCSTR filename)
{
	CFile out;
	out.Create(filename);
	if(!out.IsOpen())
		return 0;

	u8 *buffer;
	int size = Write(buffer);

	out.Write(buffer, size);
	out.Close();

	delete[] buffer;

	return 1;
}

int CModel::Write(u8* &out)
{
	int count=(int)tri.size();
	ClearVertexPool;

	MEM_STREAM str;
	MemStreamCreate(&str);

	MD1_HEADER header;
	MD1_OBJECT *obj=new MD1_OBJECT[count];

	u32 base_ptr=sizeof(header);
	u32 res;
	// skip header+object section
	MemStreamSeek(&str,base_ptr+sizeof(*obj)*count,SEEK_SET);

	for(int i=0; i<count; i++)
	{
		// TRIANGLES
		// vertexes
		obj[i].triangles.vertex_count=(u32)tri[i]->vertex.size();
		res=ProcessVertex(tri[i]->vertex,MemStreamTell(&str)-base_ptr);
		if(res==0)
		{
			obj[i].triangles.vertex_offset=MemStreamTell(&str)-base_ptr;
			for(int j=0, js=(int)tri[i]->vertex.size(); j<js; j++)
				MemStreamWrite(&str,&tri[i]->vertex[j],sizeof(MD1_VERTEX));
		}
		else obj[i].triangles.vertex_offset=res;
		// normals
		obj[i].triangles.normal_count=(u32)tri[i]->normal.size();
		res=ProcessVertex(tri[i]->normal,MemStreamTell(&str)-base_ptr);
		if(res==0)
		{
			obj[i].triangles.normal_offset=MemStreamTell(&str)-base_ptr;
			for(int j=0, js=(int)tri[i]->normal.size(); j<js; j++)
				MemStreamWrite(&str,&tri[i]->normal[j],sizeof(MD1_VERTEX));
		}
		else obj[i].triangles.normal_offset=res;
		// triangles
		obj[i].triangles.tri_offset=MemStreamTell(&str)-base_ptr;
		obj[i].triangles.tri_count=(u32)tri[i]->tri.size();
		for(int j=0, js=(int)tri[i]->tri.size(); j<js; j++)
			MemStreamWrite(&str,&tri[i]->tri[j],sizeof(MD1_TRIANGLE));

		// QUADS
		// vertexes
		obj[i].quads.vertex_count=(u32)quad[i]->vertex.size();
		res=ProcessVertex(quad[i]->vertex,MemStreamTell(&str)-base_ptr);
		if(res==0)
		{
			obj[i].quads.vertex_offset=MemStreamTell(&str)-base_ptr;
			for(int j=0, js=(int)quad[i]->vertex.size(); j<js; j++)
				MemStreamWrite(&str,&quad[i]->vertex[j],sizeof(MD1_VERTEX));
		}
		else obj[i].quads.vertex_offset=res;
		// normals
		obj[i].quads.normal_count=(u32)quad[i]->normal.size();
		res=ProcessVertex(quad[i]->normal,MemStreamTell(&str)-base_ptr);
		if(res==0)
		{
			obj[i].quads.normal_offset=MemStreamTell(&str)-base_ptr;
			for(int j=0, js=(int)quad[i]->normal.size(); j<js; j++)
				MemStreamWrite(&str,&quad[i]->normal[j],sizeof(MD1_VERTEX));
		}
		else obj[i].quads.normal_offset=res;
		// quads
		obj[i].quads.quad_offset=MemStreamTell(&str)-base_ptr;
		obj[i].quads.quad_count=(u32)quad[i]->quad.size();
		for(int j=0, js=(int)quad[i]->quad.size(); j<js; j++)
			MemStreamWrite(&str,&quad[i]->quad[j],sizeof(MD1_QUAD));
	}

	// fill header
	header.obj_count=count*2;
	header.unknown=0;
	header.length=MemStreamTell(&str);

	// fill maps
	for(int i=0; i<count; i++)
	{
		// triangle maps
		obj[i].triangles.tri_map_offset=MemStreamTell(&str)-base_ptr;
		for(int j=0, js=(int)tri[i]->tri_map.size(); j<js; j++)
			MemStreamWrite(&str,&tri[i]->tri_map[j],sizeof(MD1_TRIANGLE_MAP));
		// quad maps
		obj[i].quads.quad_map_offset=MemStreamTell(&str)-base_ptr;
		for(int j=0, js=(int)quad[i]->quad_map.size(); j<js; j++)
			MemStreamWrite(&str,&quad[i]->quad_map[j],sizeof(MD1_QUAD_MAP));
	}

	// flush header data and seek at the end
	MemStreamSeek(&str,0,SEEK_SET);
	MemStreamWrite(&str,&header,sizeof(header));
	MemStreamWrite(&str,obj,sizeof(*obj)*count);
	MemStreamSeek(&str,0,SEEK_END);

	out=str.data;
	return str.size;
}

typedef struct tagColladaVertex
{
	float x, y, z;
} COLLADA_VERTEX;

typedef struct tagColladaUV
{
	float u, v;
} COLLADA_UV;

typedef struct tagColladaPoly
{
	int v, n, uv;
} COLLADA_POLY;

static u8 CU(float u, int w)
{
	if(u<0.f) u+=1.0f;
	int ret=(int)(u*(float)w);

	if(ret==0) ret=1;
	if(ret>=w-1) ret=w-2;

	return ret;
}

static u8 CU(float u, int w, int pw)
{
	if(u<0.f) u+=1.0f;
	int ret=(int)(u*(float)pw)%w;

	if(ret==0) ret=1;
	if(ret>=w-1) ret=w-2;

	return ret;
}

static u8 CV(float v, int h)
{
	if(v<0.f) v+=1.0f;
	int ret=(int)-(v*(float)h);

	if(ret<=0) ret=h+ret;
	if(ret==0) ret=1;
	if(ret>=h-1) ret=h-2;

	return ret;
}

#define U_W	128.f
