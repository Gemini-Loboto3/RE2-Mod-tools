//#include <stdafx.h>
#include <stdafx.h>
#include <stdio.h>
#include "memstream.h"
#include "FileHandle.h"

u8* MemStreamOpen(MEM_STREAM *str, LPCTSTR name)
{
	CFile in(name);
	if (!in.IsOpen())
		return NULL;
	str->size = str->alloc = in.GetSize();

	str->data = new u8[str->size];
	str->pos = 0;
	in.Read(str->data, str->size);

	return str->data;
}

u8* MemStreamOpen(MEM_STREAM *str, u8* data, int size)
{
	str->size=str->alloc=size;
	str->data=new u8[size];
	memcpy(str->data,data,size);
	str->pos=0;

	return str->data;
}

void MemStreamFlush(MEM_STREAM *str, LPCTSTR name)
{
	CFile f(name, false);
	f.Write(str->data, str->size);
	f.Close();
}

void MemStreamCreate(MEM_STREAM *str)
{
	str->size=0;
	str->alloc=MEMFILE_ALLOC;
	str->data=new u8[MEMFILE_ALLOC];
	str->pos=0;
}

int MemStreamReadByte(MEM_STREAM *str)
{
	if(str->pos>=str->size) return EOF;
	return str->data[str->pos++];
}

int MemStreamRead(MEM_STREAM *str, void *read, int size)
{
	u8* dest=(u8*)read;
	int i;

	for(i=0; i<size; i++, dest++)
	{
		int res=MemStreamReadByte(str);
		if(res==EOF) break;
		else *dest=(u8)res;
	}
	return i;
}

__inline void MemStreamWriteByte(MEM_STREAM *str, u8 val)
{
	if(str->pos+1>=str->alloc)
	{
		u8* temp=new u8[str->alloc+MEMFILE_ALLOC];
		//u8* temp=new u8[str->alloc*2];	// using a different growth method, less fragmentation expected
		// copy old data to new buffer
		memcpy(temp,str->data,str->alloc);
		delete[] str->data;
		// assign new buffer and size
		str->data=temp;
		str->alloc+=MEMFILE_ALLOC;
		//str->alloc*=2;					// again, new allocation growth
	}
	str->data[str->pos++]=val;
	if(str->pos>str->size) str->size=str->pos;
}

int MemStreamWrite(MEM_STREAM *str, void *write, int size)
{
	u8 *src=(u8*)write;
	for(int i=0; i<size; i++, src++) MemStreamWriteByte(str,*src);

	return size;
}

int MemStreamSeek(MEM_STREAM *str, int pos, int mode)
{
	switch(mode)
	{
	case SEEK_SET:
		str->pos=pos;
		if(pos>str->size) return EOF;
		break;
	case SEEK_CUR:
		str->pos+=pos;
		if(str->pos>str->size) return EOF;
		break;
	case SEEK_END:
		str->pos=str->size-pos;
		if(str->pos<0) {str->pos=0; return EOF;}
		break;
	default:	// just in case
		return EOF;
	}

	return str->pos;
}

void MemStreamClose(MEM_STREAM *str)
{
	delete[] str->data;
}
