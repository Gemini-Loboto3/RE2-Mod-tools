#define _CRT_SECURE_NO_WARNINGS

#include "stdafx.h"
#include "Image.h"
#include "memstream.h"
#include <stdlib.h>
#include <stdio.h>
#include "lodepng.h"

/****************************************
 * General functions for images			*
 ****************************************/
__inline u32 BmpIntToInt(BYTE *bmp)
{
	// return little-endian int from pointer
	return bmp[3]|(bmp[2]<<8)|(bmp[1]<<16)|(bmp[0]<<24);
}

__inline u32 BmpIntToInt(u32 bmp)
{
	// convert big-endian int to little-endian
	return (bmp<<24)|((bmp&0xFF00)<<8)|((bmp&0xFF0000)>>8)|(bmp>>24);
}

USHORT RgbToClut(RGBQUAD rgb, u32 transparent)
{
	u32 t=(*(u32*)&rgb) & 0xFFFFFF;
	USHORT clut;
	// turn pure pink into transparent black
	if(t==transparent) clut=0x0000;
	// otherwise do regular conversion
	else clut=TIMRGB(rgb.rgbRed,rgb.rgbGreen,rgb.rgbBlue);

	return clut;
}

RGBQUAD ClutToRgb(USHORT clut, u32 transparent)
{
	RGBQUAD rgb={0};
	// process transparent
	if(clut==0x0000) memcpy(&rgb,&transparent,sizeof(transparent));
	else
	{
		rgb.rgbRed=(clut&0x3F)<<3;
		rgb.rgbGreen=((clut>>5)&0x3F)<<3;
		rgb.rgbBlue=((clut>>10)&0x3F)<<3;
	}
	return rgb;
}

RGBQUAD B5G5R5ToRgb(USHORT clut, u32 transparent)
{
	RGBQUAD rgb={0};
	// process transparent
	if(clut==0x0000) memcpy(&rgb,&transparent,sizeof(transparent));
	else
	{
		rgb.rgbBlue=(clut&0x3F)<<3;
		rgb.rgbGreen=((clut>>5)&0x3F)<<3;
		rgb.rgbRed=((clut>>10)&0x3F)<<3;
	}
	return rgb;
}

int ImageToClut(Image *image, int x, int y, USHORT *paldest, int count)
{
	RGBQUAD pixel;
	int p;

	// do not allow paletted images
	if(image->depth!=24) return 0;
	for(int i=0; i<count; i++, x++)
	{
		// skip to next line if necessary
		if(x>image->width){x=0;y++;}
		p=image->GetPixelAt(x,y);
		memcpy(&pixel,&p,4);
		paldest[i]=RgbToClut(pixel);
	}
	// success
	return 1;
}

int ImageToRgb(Image *image, int x, int y, RGBQUAD *paldest, int count)
{
	for(int i=0; i<count; i++, x++)
	{
		// skip to next line if necessary
		if(x>image->width){x=0;y++;}
		int pixel=image->GetPixelIndex(x,y);
		memcpy(&paldest[i],&pixel,4);
	}
	// success
	return 1;
}

int GetImageType(LPCTSTR name)
{
	FILE *file;
	BYTE magic[4];
	file=fopen(name,"rb");
	fread(magic,4,1,file);
	fclose(file);

	return GetImageType(magic);
}

int GetImageType(BYTE *magic)
{
	if(memcmp(magic,"BM",2)==0) return IMAGE_BMP;
	else if(*((UINT*)magic)=0x10) return IMAGE_TIM;
	else return -1;
}

// convert tim 4bpp line to bitmap
void ConvertB5G5R5(BYTE *src_row, BYTE *dst_row, int w)
{
	for(int x=0; x<w; x+=2)
	{
		USHORT color=(src_row[x]<<8)|(src_row[x+1]);
		*((USHORT*)&dst_row[x])=((color&0xF000)>>12)|((color&0x0F00)>>4)|((color&0x00F0)<<4)|((color&0x000F)<<12);
	}
}

// convert tim 8bpp line to bitmap
void ConvertB8G8R8(BYTE *src_row, BYTE *dst_row, int w)
{
	for(int x=0; x<w; x+=2)
		*((USHORT*)&dst_row[x])=*((USHORT*)&src_row[x]);
}

// convert tim 15bpp line to bitmap
void ConvertDirect15(BYTE *src_row, BYTE *dst_row, int w)
{
	RGBTRIPLE *p=(RGBTRIPLE*)dst_row;
	for(int x=0; x<w; x++, src_row+=2)
	{
		RGBQUAD q=ClutToRgb(*((u16*)src_row));
		memcpy(&p[x],&q,sizeof(RGBTRIPLE));
	}
}

// convert BGR 15bpp line to bitmap
void ConvertDirect15BGR(BYTE *src_row, BYTE *dst_row, int w)
{
	RGBTRIPLE *p=(RGBTRIPLE*)dst_row;
	for(int x=0; x<w; x++, src_row+=2)
	{
		RGBQUAD q=B5G5R5ToRgb(*((u16*)src_row));
		memcpy(&p[x],&q,sizeof(RGBTRIPLE));
	}
}

// convert tim 24bpp line to bitmap
void ConvertDirect24(BYTE *src_row, BYTE *dst_row, int w)
{
	RGBTRIPLE *p=(RGBTRIPLE*)dst_row;
	for(int x=0; x<w; x++)
	{
		p[x].rgbtRed=src_row[x*3];
		p[x].rgbtGreen=src_row[x*3+1];
		p[x].rgbtBlue=src_row[x*3+2];
	}
}

void ConvertTex24(BYTE *src_row, BYTE *dst_row, int w)
{
	RGBQUAD *line;
	RGBTRIPLE *p=(RGBTRIPLE*)dst_row;
	for(int x=0; x<w; x++)
	{
		line=(RGBQUAD*)&src_row[x*3];
		p[x].rgbtBlue=line->rgbRed;
		p[x].rgbtGreen=line->rgbBlue;
		p[x].rgbtRed=line->rgbGreen;
	}
}

void* ImageToTrueColor(Image *img, COLORREF transmask)
{
	BYTE *image=new BYTE[img->width*img->height*4];
	ImageToTrueColor(img,image,transmask);
	return image;
}

void ImageToTrueColor(Image *img, BYTE *dest, COLORREF transmask)
{
	UINT pixel=0;
	for(int y=0, seek=0; y<img->height; y++)
	{
		for(int x=0; x<img->width; x++, seek+=4)
		{
			if(img->depth<=8)
			{
				int val=img->GetPixelAt(x,y);
				memcpy(&pixel,&img->palette[val],3);
			}
			else if(img->depth==16)				// 16 bpp
			{
				int val=img->GetPixelAt(x,y);
				pixel=RGB((val&0x1F)*8,((val>>5)&0x1F)*8,((val>>10)&0x1F)*8);
			}
			else pixel=img->GetPixelAt(x,y);	// 24 bpp

			dest[seek]=(BYTE)pixel;
			dest[seek+1]=(BYTE)(pixel>>8);
			dest[seek+2]=(BYTE)(pixel>>16);
			// deal with transparencies
			if(pixel==transmask) dest[seek+3]=0;
			else dest[seek+3]=0xFF;
		}
//		Align(seek,4);
	}
}

void* PalImageToTrueColor(Image *img, RGBQUAD *pal, COLORREF transmask)
{
	BYTE *image=new BYTE[img->width*img->height*4];
	if(img->depth>8) return 0;	// can't be converted
	PalImageToTrueColor(img,pal,image,transmask);
	return image;
}

void PalImageToTrueColor(Image *img, RGBQUAD *pal, BYTE *dest, COLORREF transmask)
{
	UINT pixel=0;
	for(int y=0, seek=0; y<img->height; y++)
	{
		for(int x=0; x<img->width; x++, seek+=4)
		{
			int val=img->GetPixelAt(x,y);
			memcpy(&pixel,&pal[val],3);

			dest[seek]=(BYTE)pixel;
			dest[seek+1]=(BYTE)(pixel>>8);
			dest[seek+2]=(BYTE)(pixel>>16);
			// deal with transparencies
			if(pixel==transmask) dest[seek+3]=0;
			else dest[seek+3]=0xFF;
		}
	}
}

/****************************************
 * Playstation images					*
 ****************************************/
Tim::Tim()
{
	image=NULL;
	clut=NULL;
};

Tim::~Tim()
{
	Reset();
};

void Tim::Reset()
{
	if(image) delete[] image;
	if(clut) delete[] clut;
}

void Tim::Copy(const Tim *tim)
{
	Reset();

	if(tim->clut)
	{
		clut=new u16[tim->clut_w*tim->clut_h];
		memcpy(clut,tim->clut,tim->clut_w*tim->clut_h*2);
	}

	if(tim->image)
	{
		image=new u8[tim->img_wp*tim->img_h*2];
		memcpy(image,tim->image,tim->img_wp*tim->img_h*2);
	}

	this->clut_x=tim->clut_x, this->clut_y=tim->clut_y;
	this->clut_w=tim->clut_w, this->clut_h=tim->clut_h;
	this->img_x=tim->img_x, this->img_y=tim->img_y;
	this->img_w=tim->img_w, this->img_h=tim->img_h;
	this->img_wp=tim->img_wp;
	this->img_depth=tim->img_depth;

	//if(tim.clut)
	//{
	//	clut=new u16[tim.clut_w*tim.clut_h];
	//	memcpy(clut,tim.clut,tim.clut_w*tim.clut_h*2);
	//}

	//if(tim.image)
	//{
	//	image=new u8[tim.img_wp*tim.img_h*2];
	//	memcpy(image,tim.image,tim.img_wp*tim.img_h*2);
	//}

	//this->clut_x=tim.clut_x, this->clut_y=tim.clut_y;
	//this->clut_w=tim.clut_w, this->clut_h=tim.clut_h;
	//this->img_x=tim.img_x, this->img_y=tim.img_y;
	//this->img_w=tim.img_w, this->img_h=tim.img_h;
	//this->img_wp=tim.img_wp;
	//this->img_depth=tim.img_depth;
}

//void Tim::operator = (const Tim *tim)
//{
//	Reset();
//
//	clut=new u16[tim->clut_w*tim->clut_h];
//	image=new u8[tim->img_wp*tim->img_h*2];
//
//	memcpy(clut,tim->clut,tim->clut_w*tim->clut_h*2);
//	memcpy(image,tim->image,tim->img_wp*tim->img_h*2);
//
//	this->clut_x=tim->clut_x, this->clut_y=tim->clut_y;
//	this->clut_w=tim->clut_w, this->clut_h=tim->clut_h;
//	this->img_x=tim->img_x, this->img_y=tim->img_y;
//	this->img_w=tim->img_w, this->img_h=tim->img_h;
//	this->img_wp=tim->img_wp;
//	this->img_depth=tim->img_depth;
//}

void Tim::CreatePixel(int width, int height, int x, int y, int depth)
{
	int w;
	switch(depth)
	{
	case 4:		// 4BPP
		w=width/4;
		break;
	case 8:		// 8BPP
		w=width/2;
		break;
	case 15:	// 16BPP
		depth=16;	// fix 15/16 mismatch
	case 16:
		w=width;
		break;
	case 24:
		w=(width*3)/2;
		break;
	default:
		w=0;
	}

	if(image) delete[] image;
	image=(BYTE*)new BYTE[w*height*2];
	ZeroMemory(image,w*height*2);

	img_w=width;
	img_wp=w;
	img_h=height;
	img_x=x;
	img_y=y;
	img_depth=depth;
}

void Tim::CreateClut(int width, int height, int x, int y)
{
	if(clut) delete[] clut;
	clut=new u16[width*height];

	clut_w=width;
	clut_h=height;
	clut_x=x;
	clut_y=y;
}

void Tim::ConvertBmp(void *img_ptr, int _width, int _height)
{
	Image *img=(Image*)img_ptr;
	u8* dest=image;
	for(int y=0; y<_height; y++)
	{
		switch(img_depth)
		{
		case 4:
			for(int x=0; x<_width; x+=2, dest++)
				*dest=img->GetPixelAt(x,y)|(img->GetPixelAt(x+1,y)<<4);
			break;
		case 8:
			for(int x=0; x<_width; x++, dest++)
				*dest=img->GetPixelAt(x,y);
			break;
		case 16:	// not implemented yet
			for(int x=0; x<_width; x++, dest+=2)
			{
				u32 p=img->GetPixelIndex(x,y) & 0xFFFFFF;
				*(u16*)dest=TIMRGB(((p>>16) &0xFF),	// r
					((p>>8)&0xFF),					// g
					(p)&0xFF);						// b
			}
			break;
		case 24:	// not implemented yet
			for(int x=0; x<_width; x++)
			{
				u32 p=img->GetPixelIndex(x,y);
				*dest++=p>>16;
				*dest++=p>>8;
				*dest++=p;
			}
			break;
		}
	}
}

void* Tim::LoadTim(LPCTSTR filename)
{
	return NULL;
	//BYTE *buffer;
	//if(!StoreFile(filename,buffer)) return 0;
	//void *tim=LoadTim(buffer);
	//delete[] buffer;

	//return tim;
}

size_t Tim::LoadTim(BYTE *buffer)
{
	size_t size = 8;

	TIM_HEADER *tim_header;
	TIM_CLUT *clut_head;
	TIM_PIXEL *tim_pixel;
	USHORT *tim_clut;
	int clut_size;

	tim_header=(TIM_HEADER*)buffer;
	if(tim_header->id!=0x10) return 0;

	// check depth data
	int clut_bit=tim_header->flag&7;
	int clut_sec=(tim_header->flag>>3)&1;

	// step to next section
	int seek=sizeof(TIM_HEADER);
	// read clut segment if there is one
	if(clut_sec)
	{
		clut_head=(TIM_CLUT*)(buffer+seek);
		size += clut_head->bnum;
		tim_clut=(USHORT*)(buffer+seek+sizeof(TIM_CLUT));
		clut_size=(clut_head->bnum-sizeof(TIM_CLUT))/sizeof(USHORT);
		// copy clut data
		clut_w=clut_head->w;
		clut_h=clut_head->h;
		clut_x=clut_head->x;
		clut_y=clut_head->y;
		if(clut_size>0)
		{
			// allocate clut
			if(clut) delete[] clut;
			clut=new USHORT[clut_size];
			// copy to class clut
			CopyMemory(clut,tim_clut,clut_size*sizeof(USHORT));
		}
		else
			clut_sec=0;
		// step to next section
		seek+=clut_head->bnum;
	}

	// read pixel segment
	tim_pixel=(TIM_PIXEL*)(buffer+seek);
	size += tim_pixel->bnum;
	int pixel_size=tim_pixel->w*tim_pixel->h*2;
	BYTE *pixel=buffer+seek+sizeof(TIM_PIXEL);
	// allocate new image
	if(image) delete[] image;
	image=(BYTE*)new BYTE[pixel_size];
	// copy to class image
	CopyMemory(image,pixel,pixel_size);

	img_x=tim_pixel->x;
	img_y=tim_pixel->y;
	img_wp=tim_pixel->w;
	img_h=tim_pixel->h;
	// do depth detection
	switch(clut_bit)
	{
	case PMODE_4:
		img_w=tim_pixel->w*4;
		img_depth=4;	// 4BPP
		// fill with grayscale whether it's clut-less
		if(!clut_sec)
		{
			if(clut) delete[] clut;
			clut=new USHORT[16];
			for(int i=0; i<16; i++) clut[i]=TIMRGB(i*16,i*16,i*16);
		}
		break;
	case PMODE_8:	// 8BPP
		img_w=tim_pixel->w*2;
		img_depth=8;
		// fill with grayscale otherwise
		if(!clut_sec)
		{
			if(clut) delete[] clut;
			clut=new USHORT[256];
			for(int i=0; i<256; i++) clut[i]=TIMRGB(i,i,i);
		}
		break;
	case PMODE_15:	// 16BPP
		img_w=tim_pixel->w;
		img_depth=16;
		break;
	case PMODE_24:
		img_w=(tim_pixel->w*2)/3;
		img_depth=24;	// 24BPP
		break;
	default:
		return 0;
	}

	return size;
}

int Tim::Save(u8* &dest)
{
	TIM_HEADER head;
	head.id=0x10;
	if(clut) head.flag=1<<3;
	else head.flag=0;
	// define clut segment
	TIM_CLUT clt;
	clt.bnum=clut_w*clut_h*2+12;
	clt.x=clut_x;
	clt.y=clut_y;
	clt.w=clut_w;
	clt.h=clut_h;
	// define pixel segment
	TIM_PIXEL pixel;
	switch(this->img_depth)
	{
	case 4:
		//head.flag|=PMODE_4;
		pixel.w=img_w/4;
		break;
	case 8:
		head.flag|=PMODE_8;
		pixel.w=img_w/2;
		break;
	case 16:
		head.flag|=PMODE_15;
		pixel.w=img_w;
		break;
	case 24:
		head.flag|=PMODE_24;
		pixel.w=img_w*3/2;
	}
	pixel.bnum=pixel.w*img_h*2+12;
	pixel.x=img_x;
	pixel.y=img_y;
	pixel.h=img_h;

	int fsize=sizeof(head)+ ( this->img_depth<=8 ? sizeof(clt)+(clt.bnum-12) : 0 )+sizeof(pixel)+(pixel.bnum-12);
	dest=new u8[fsize];

	u8* b=dest;
	memcpy(b,&head,sizeof(head)); b+=sizeof(head);
	if(this->img_depth<=8)
	{
		memcpy(b,&clt,sizeof(clt)); b+=sizeof(clt);
		memcpy(b,clut,clt.bnum-12); b+=clt.bnum-12;
	}
	memcpy(b,&pixel,sizeof(pixel)); b+=sizeof(pixel);
	memcpy(b,image,pixel.bnum-12);

	return fsize;
}

void Tim::Save(LPCTSTR filename)
{
	TIM_HEADER head;
	head.id=0x10;
	if(clut) head.flag=1<<3;
	else head.flag=0;

	TIM_CLUT clt;
	TIM_PIXEL pixel;

	// define clut segment
	if(clut)
	{
		clt.bnum=clut_w*clut_h*2+12;
		clt.x=clut_x;
		clt.y=clut_y;
		clt.w=clut_w;
		clt.h=clut_h;
	}

	if(image)
	{
		// define pixel segment
		switch(this->img_depth)
		{
		case 4:
			//head.flag|=PMODE_4;
			pixel.w=img_w/4;
			break;
		case 8:
			head.flag|=PMODE_8;
			pixel.w=img_w/2;
			break;
		case 16:
			head.flag|=PMODE_15;
			pixel.w=img_w;
			break;
		case 24:
			head.flag|=PMODE_24;
			pixel.w=img_w*3/2;
		}
		pixel.bnum=pixel.w*img_h*2+12;
		pixel.x=img_x;
		pixel.y=img_y;
		pixel.h=img_h;
	}

	FILE *out=fopen(filename,"wb+");
	fwrite(&head,sizeof(head),1,out);
	if(clut)
	{
		fwrite(&clt,sizeof(clt),1,out);
		fwrite(clut,clt.bnum-12,1,out);
	}
	if(image)
	{
		fwrite(&pixel,sizeof(pixel),1,out);
		fwrite(image,pixel.bnum-12,1,out);
	}
	fclose(out);
}

u16 Tim::GetClutColor(int x, int y)
{
	if(clut)
	{
		// wrap coordinates if out of range
		x%=clut_w;
		y%=clut_h;

		return clut[x+clut_w*y];
	}

	return 0;
}

/****************************************
 * Bitmap								*
 ****************************************/
Bitmap::Bitmap()
{
	image=NULL;
}

Bitmap::~Bitmap()
{
	if(image) delete[] image;
}

void* Bitmap::LoadBitmap(BITMAPINFOHEADER *info)
{
	u8* data=(u8*)info, *d=data;

	//memcpy(&InfoHeader,info,sizeof(BITMAPINFOHEADER));
	data+=sizeof(BITMAPINFOHEADER);
	depth=info->biBitCount;
	// read palette if necessary
	if(depth<=8)
	{
		memcpy(palette,data,(1<<depth)*sizeof(RGBQUAD));
		data+=(1<<depth)*sizeof(RGBQUAD);
	}
	// delete any existent buffer
	if(image) delete[] image;

	// read image data
	if(info->biSizeImage!=0) image=new BYTE[info->biSizeImage];

	memcpy(image,data,info->biSizeImage);

	width=info->biWidth;
	height=info->biHeight;
	return image;
}

void* Bitmap::LoadBitmap(u8* data)
{
	BITMAPFILEHEADER FileHeader;
	u8* d=data;

	memcpy(&FileHeader,data,sizeof(BITMAPFILEHEADER));
	data+=sizeof(BITMAPFILEHEADER);
	if(FileHeader.bfType!=('B'|('M'<<8))) return 0;

	memcpy(&InfoHeader,data,sizeof(BITMAPINFOHEADER));
	data+=sizeof(BITMAPINFOHEADER);
	depth=InfoHeader.biBitCount;
	// read palette if necessary
	if(depth<=8)
	{
		memcpy(palette,data,(1<<depth)*sizeof(RGBQUAD));
		data+=(1<<depth)*sizeof(RGBQUAD);
	}
	// delete any existent buffer
	if(image) delete[] image;

	// read image data
	data=&d[FileHeader.bfOffBits];
	if(InfoHeader.biSizeImage!=0) image=new BYTE[InfoHeader.biSizeImage];

	memcpy(image,data,InfoHeader.biSizeImage);

	width=InfoHeader.biWidth;
	height=InfoHeader.biHeight;
	return image;
}

void* Bitmap::LoadBitmap(LPCTSTR filename)
{
	FILE *file;
	BITMAPFILEHEADER FileHeader;
	file=fopen(filename,"rb+");

	fread(&FileHeader,sizeof(BITMAPFILEHEADER),1,file);
	if(FileHeader.bfType!=('B'|('M'<<8))) return 0;

	fread(&InfoHeader,sizeof(BITMAPINFOHEADER),1,file);
	depth=InfoHeader.biBitCount;
	// read palette if necessary
	if(depth<=8) fread(palette,1<<depth,sizeof(RGBQUAD),file);
	// delete any existent buffer
	if(image) delete[] image;

	// read image data
	fseek(file,FileHeader.bfOffBits,SEEK_SET);
	if(InfoHeader.biSizeImage!=0) image=new BYTE[InfoHeader.biSizeImage];

	fread(image,InfoHeader.biSizeImage,1,file);
	fclose(file);

	width=InfoHeader.biWidth;
	height=InfoHeader.biHeight;
	return image;
};

/****************************************
 * General image handler				*
 ****************************************/
Image::Image()
{
	image=NULL;
	rows=NULL;
}

Image::~Image()
{
	if(image) delete[] image;
	if(rows) delete[] rows;
}

void Image::Copy(const Image *source)
{
	Create(source->width,source->height,source->depth,(RGBQUAD*)source->palette);
	BitBlit((Image*)source,0,0,source->width,source->height,0,0,0);
}

void Image::Create(int width, int height, int depth, RGBQUAD *palette)
{
	// copy specs
	this->width=width;
	this->height=height;
	this->depth=depth;
	// bmp actual bytes per row
	int _w=row_w=PadWidth(width,depth);
	// determine bmp image size
	int size=height*_w;
	// allocation
	if(image) delete[] image;
	image=new BYTE[size];
	// reset image
	ZeroMemory(image,size);

	switch(depth)
	{
	case 1:
		// define black and white pal
		ZeroMemory(this->palette,sizeof(RGBQUAD)*sizeof(RGBQUAD));
		this->palette[1].rgbRed=255;
		this->palette[1].rgbGreen=255;
		this->palette[1].rgbBlue=255;
		this->palette[1].rgbReserved=0;
		break;
	case 4:
	case 8:
		CopyMemory(this->palette,palette,(1<<depth)*sizeof(RGBQUAD));
		break;
	}

	InitRows();
}

void* Image::CreateFromBmp(Bitmap *bmp)
{
	// copy specs
	width=bmp->width;
	height=bmp->height;
	depth=bmp->depth;
	// determine bmp image size
	int size=height*PadWidth(width,depth);
	// bytes per row
	row_w=PadWidth(width,depth);
	// allocation
	if(image) delete[] image;
	image=new BYTE[size];
	// copy or reset palette field
	if(depth<=8) CopyMemory(palette,bmp->palette,256*sizeof(RGBQUAD));
	else ZeroMemory(palette,256*sizeof(RGBQUAD));
	// copy image
	CopyMemory(image,bmp->image,size);

	InitRows();
	return image;
}

void* Image::CreateFromTim(Tim *tim, int pal, COLORREF colormask)
{
	// copy specs
	width=tim->img_w;
	height=tim->img_h;
	depth=tim->img_depth;
	// bmp actual bytes per row
	int _w=row_w=PadWidth(width,depth);
	// tim row size
	int w=width*depth/8;
	// determine bmp image size
	int size=height*_w;
	// allocation
	if(image) delete[] image;
	image=new BYTE[size];
	ZeroMemory(image,size);
	// copy or reset palette field
	if(depth<=8) ClutToPalette(&tim->clut[pal*PalSize(depth)],colormask);
	else ZeroMemory(palette,256*sizeof(RGBQUAD));

	// copy and flip image
	BYTE *row=tim->image+w*height-w;
	BYTE *brow=image;
	for(int y=0; y<height; y++, row-=w, brow+=_w)
	{
		switch(depth)
		{
		case 4:
			for(int x=0; x<w; x+=2)
			{
				USHORT color=(row[x]<<8)|(row[x+1]);
				*((USHORT*)&brow[x])=((color&0xF000)>>12)|((color&0x0F00)>>4)|((color&0x00F0)<<4)|((color&0x000F)<<12);
			}
			break;
		case 8:
			for(int x=0; x<w; x+=2) *((USHORT*)&brow[x])=*((USHORT*)&row[x]);
			break;
		case 16:
			for(int x=0; x<width; x++)
			{
				USHORT color=*((USHORT*)&row[x*2]);
				if(color==0)
					*((USHORT*)&brow[x*2])=(USHORT)TIMRGB(colormask&0xFF,(colormask>>8)&0xFF,(colormask>>16)&0xFF);	// transparent black
				else *((USHORT*)&brow[x*2])=((color&0x1F)<<10)|(color&0x3E0)|((color>>10)&0x1F);
			}
			break;
		case 24:
			for(int x=0; x<width; x++)
			{
				brow[x*3+2]=row[x*3];
				brow[x*3+1]=row[x*3+1];
				brow[x*3]=row[x*3+2];
			}
		}
	}

	InitRows();
	return image;
}

void* Image::CreateFromSpec(BYTE *img, RGBQUAD *pal, int type, int _w, int _h)
{
	// setup depth
	switch(type)
	{
	case IMG_MONO:
		depth=1;
		break;
	case IMG_BMP4:
	case IMG_B5G5R5:
		depth=4;
		break;
	case IMG_BMP8:
	case IMG_B8G8R8:
		depth=8;
		break;
	case IMG_DIRECT15:
	case IMG_DIRECT15BGR:
	case IMG_BMP24:
	case IMG_DIRECT24:
	case IMG_TEX24:
		depth=24;
		break;
	default:
		return 0;
	}
	Create(_w,_h,depth,pal);
	int w=width*depth/8;

	BYTE *dest=image;
	for(int y=0; y<_h; y++)
	{
		switch(type)
		{
		// does not flip the image
		case IMG_MONO:
		case IMG_BMP4:
		case IMG_BMP8:
		case IMG_BMP24:
			CopyMemory(dest,img,row_w);
			dest+=row_w;
			img+=row_w;
			break;
		// following cases flip the image
		case IMG_B5G5R5:
			ConvertB5G5R5(img,rows[y],w);
			img+=_w/2;
			break;
		case IMG_B8G8R8:
			ConvertB8G8R8(img,rows[y],w);
			img+=_w;
			break;
		case IMG_DIRECT15:
			ConvertDirect15(img,rows[y],_w);
			img+=_w*2;
			break;
		case IMG_DIRECT15BGR:
			ConvertDirect15BGR(img,rows[y],_w);
			img+=_w*2;
			break;
		case IMG_DIRECT24:
			ConvertDirect24(img,rows[y],_w);
			img+=_w*3;
			break;
			// does not flit
		case IMG_TEX24:
			ConvertTex24(img,rows[_h-1-y],_w);
			img+=_w*3;
			break;
		}
	}

	return image;
}

void Image::SaveBitmap(LPCTSTR name)
{
	BITMAPINFOHEADER info;
	BITMAPFILEHEADER header;
	FILE *file;

	int palsize;
	if(depth<=8) palsize=sizeof(RGBQUAD)*(1<<depth);
	else palsize=0;
	// set header
	header.bfType='B'|('M'<<8);
	header.bfOffBits=sizeof(BITMAPINFOHEADER)+palsize+sizeof(BITMAPFILEHEADER);
	header.bfSize=header.bfOffBits+(PadWidth(width,depth)*height);
	header.bfReserved1=0;
	header.bfReserved2=0;
	// set info header
	ZeroMemory(&info,sizeof(BITMAPINFOHEADER));
	info.biSize=sizeof(BITMAPINFOHEADER);
	info.biSizeImage=PadWidth(width,depth)*height;
	info.biBitCount=depth;
	info.biWidth=width;
	info.biHeight=height;
	info.biPlanes=1;
	// flush everything
	file=fopen(name,"wb");
	fwrite(&header,sizeof(BITMAPFILEHEADER),1,file);
	fwrite(&info,info.biSize,1,file);
	if(palsize>0) fwrite(palette,palsize,1,file);
	fwrite(image,info.biSizeImage,1,file);
	fclose(file);
}

void Image::SavePng(LPCTSTR name)
{
	u32 *image = new u32[this->width * this->height], *out = image;

	for (int y = 0; y < this->height; y++)
		for (int x = 0; x < this->width; x++)
			*out++ = this->GetPixelPng(x, y);

	lodepng::encode(name, (u8*)image, this->width, this->height, LCT_RGBA);

	delete[] image;
}

void* Image::LoadFromFile(LPCTSTR name)
{
	void *data=NULL;
	Bitmap bmp;
	Tim tim;

	// detect format
	switch(GetImageType(name))
	{
	case IMAGE_BMP:
		if(!bmp.LoadBitmap(name)) return 0;
		data=CreateFromBmp(&bmp);
		break;
	case IMAGE_TIM:
		if(!tim.LoadTim(name)) return 0;
		data=CreateFromTim(&tim,0);
		break;
	}
	return data;
}

HBITMAP Image::GetHandle()
{
	HBITMAP handle;
	BITMAPINFO *info;
	BYTE *bmpimg;

	// allocate header
	info=(BITMAPINFO*)new BYTE[sizeof(BITMAPINFOHEADER)+sizeof(RGBQUAD)*256];
	ZeroMemory(info,sizeof(BITMAPINFOHEADER));
	// copy from original header
	info->bmiHeader.biSize=sizeof(BITMAPINFOHEADER);
	info->bmiHeader.biSizeImage=PadWidth(width,depth)*height;
	info->bmiHeader.biBitCount=depth;
	info->bmiHeader.biWidth=width;
	info->bmiHeader.biHeight=height;
	info->bmiHeader.biPlanes=1;
	// copy palette data if necessary
	if(depth<=8) CopyMemory(info->bmiColors,palette,sizeof(RGBQUAD)*256);
	else ZeroMemory(info->bmiColors,sizeof(RGBQUAD)*256);
	// init handle
	handle=CreateDIBSection(NULL,info,DIB_RGB_COLORS,(void**)&bmpimg,NULL,0);
	// copy image data
	CopyMemory(bmpimg,image,info->bmiHeader.biSizeImage);
	// free shit and return handle
	delete[] info;
	
	return handle;
}

void Image::ClutToPalette(USHORT *clut, COLORREF transmask)
{
	if(depth>8) return;

	int palsize=(1<<depth);
	for(int i=0; i<palsize; i++)
	{
		if(clut[i]==0)
		{
			palette[i].rgbRed=0;
			palette[i].rgbGreen=0;
			palette[i].rgbBlue=0;
			palette[i].rgbReserved=0;
			//palette[i].rgbRed=GetRValue(transmask);
			//palette[i].rgbGreen=GetGValue(transmask);
			//palette[i].rgbBlue=GetBValue(transmask);
			//palette[i].rgbReserved=0;
		}
		else
		{
			palette[i].rgbRed=(clut[i]&0x1F)<<3;
			palette[i].rgbGreen=((clut[i]>>5)&0x1F)<<3;
			palette[i].rgbBlue=((clut[i]>>10)&0x1F)<<3;
			palette[i].rgbReserved=0xff;
		}
	}
}

u32 Image::GetPixelAt(int x, int y)
{
	if(x<0 || x>=width) return 0;
	if(y<0 || y>=height) return 0;

	ULONG pixel, shift;
	BYTE *line8=rows[y];
	ULONG *line32=(ULONG*)rows[y];

	switch(depth)
	{
	case 1:
		shift=7-(x%8);
		pixel=(line8[x/8]>>shift)&1;
		break;
	case 4:
		shift=(7-(x%8))*4;
		pixel=(BmpIntToInt((BYTE*)&line32[x/8])>>shift)&0xF;
		break;
	case 8:
		shift=(3-(x%4))*8;
		pixel=(BmpIntToInt((BYTE*)&line32[x/4])>>shift)&0xFF;
		break;
	case 16:
		shift=BmpIntToInt((BYTE*)&line32[x/2]);
		if(x%2) pixel=shift&0xFFFF;
		else pixel=shift>>16;
		break;
	case 24:
		CopyMemory(&pixel,&line8[x*3],sizeof(RGBQUAD)-1);
		pixel&=0xFFFFFF;
		break;
	case 32:
		CopyMemory(&pixel,&line8[x*4],sizeof(RGBQUAD));
		break;
	default:	// other cases always return 0
		pixel=0;
	}

	return pixel;
}

void Image::SetPixelAt(int x, int y, u32 p)
{
	if(x<0 || x>=width) return;
	if(y<0 || y>=height) return;

	ULONG pixel, shift, mask;
	BYTE *line8=rows[y];
	ULONG *line32=(ULONG*)rows[y];

	switch(depth)
	{
	case 1:
		shift=7-(x%8);
		mask=~(1<<shift);
		line8[x/8]=(BYTE)((line8[x/8]&mask)|((p&1)<<shift));
		break;
	case 4:
		shift=(7-(x%8))*4;
		mask=~(0xF<<shift);
		pixel=(BmpIntToInt(line32[x/8])&mask)|(p<<shift);
		line32[x/8]=BmpIntToInt(pixel);
		break;
	case 8:
		shift=(3-(x%4))*8;
		mask=~(0xFF<<shift);
		pixel=(BmpIntToInt(line32[x/4])&mask)|(p<<shift);
		line32[x/4]=BmpIntToInt(pixel);
		break;
	case 16:	// is this really necessary?
		shift=BmpIntToInt((BYTE*)&line32[x/2]);
		if(x%2) pixel=(shift&0xFFFF)|(p<<16);
		else pixel=(shift&0xFFFF0000)|p;
		line32[x/2]=BmpIntToInt(pixel);
		break;
	case 24:
		CopyMemory(&line8[x*3],&p,sizeof(RGBQUAD)-1);
		break;
	case 32:
		CopyMemory(&line8[x*4],&p,sizeof(RGBQUAD));
		break;
	}
}

u32 Image::GetPixelIndex(int x, int y)
{
	ULONG pixel;

	if(depth<=8) memcpy(&pixel,&palette[GetPixelAt(x,y)],sizeof(RGBQUAD));
	else if(depth==16)
	{
		RGBQUAD quad;
		pixel=GetU16Le(GetPixelAt(x,y));
		quad.rgbBlue=(BYTE)((pixel&0x1F)<<3);
		quad.rgbGreen=(BYTE)(((pixel>>5)&0x1F)<<3);
		quad.rgbRed=(BYTE)(((pixel>>10)&0x1F)<<3);
		quad.rgbReserved=0;
		memcpy(&pixel,&quad,sizeof(RGBQUAD));
	}
	else pixel=GetPixelAt(x,y);

	return pixel;
}

u32 Image::GetPixelPng(int x, int y)
{
	ULONG pixel;

	if (depth <= 8) memcpy(&pixel, &palette[GetPixelAt(x, y)], sizeof(RGBQUAD));
	else if (depth == 16)
	{
		RGBQUAD quad;
		pixel = GetU16Le(GetPixelAt(x, y));
		quad.rgbBlue = (BYTE)((pixel & 0x1F) << 3);
		quad.rgbGreen = (BYTE)(((pixel >> 5) & 0x1F) << 3);
		quad.rgbRed = (BYTE)(((pixel >> 10) & 0x1F) << 3);
		quad.rgbReserved = 0xff;
		memcpy(&pixel, &quad, sizeof(RGBQUAD));
	}
	else pixel = GetPixelAt(x, y) | 0xff000000;

	pixel = (pixel & 0xff00ff00) | ((pixel & 0xff) << 16) | ((pixel & 0xff0000) >> 16);

	return pixel;
}

bool Image::BitBlit(Image *src, int sx, int sy, int w, int h, int dx, int dy, u32 direction)
{
	//if(depth<16) return false;

	if(depth>=16)
	{
		switch(direction)
		{
		case dir_normal:
			for(int y=0; y<h; y++)
				for(int x=0; x<w; x++)
					SetPixelAt(dx+x,dy+y,src->GetPixelIndex(sx+x,sy+y));
			break;
		case dir_mirror:
			for(int y=0; y<h; y++)
				for(int x=0, xi=w-1; x<w; x++, xi--)
					SetPixelAt(dx+x,dy+y,src->GetPixelIndex(sx+xi,sy+y));
			break;
		case dir_flip:
			for(int y=0, yi=h-1; y<h; y++, yi--)
				for(int x=0; x<w; x++)
					SetPixelAt(dx+x,dy+y,src->GetPixelIndex(sx+x,sy+yi));
			break;
		case dir_flip+dir_mirror:
			for(int y=0, yi=h-1; y<h; y++, yi--)
				for(int x=0, xi=w-1; x<w; x++, xi--)
					SetPixelAt(dx+x,dy+y,src->GetPixelIndex(sx+xi,sy+yi));
			break;
		}
	}
	else
	{
		switch(direction)
		{
		case dir_normal:
			for(int y=0; y<h; y++)
				for(int x=0; x<w; x++)
					SetPixelAt(dx+x,dy+y,src->GetPixelAt(sx+x,sy+y));
			break;
		case dir_mirror:
			for(int y=0; y<h; y++)
				for(int x=0, xi=w-1; x<w; x++, xi--)
					SetPixelAt(dx+x,dy+y,src->GetPixelAt(sx+xi,sy+y));
			break;
		case dir_flip:
			for(int y=0, yi=h-1; y<h; y++, yi--)
				for(int x=0; x<w; x++)
					SetPixelAt(dx+x,dy+y,src->GetPixelAt(sx+x,sy+yi));
			break;
		case dir_flip+dir_mirror:
			for(int y=0, yi=h-1; y<h; y++, yi--)
				for(int x=0, xi=w-1; x<w; x++, xi--)
					SetPixelAt(dx+x,dy+y,src->GetPixelAt(sx+xi,sy+yi));
			break;
		}
	}
	return true;
}

void Image::InitRows()
{
	if(rows) delete[] rows;
	rows=new BYTE*[height];
	BYTE *pointer=image+row_w*height-row_w;
	for(int i=0; i<height; i++)
	{
		rows[i]=pointer;
		pointer-=row_w;
	}
}
