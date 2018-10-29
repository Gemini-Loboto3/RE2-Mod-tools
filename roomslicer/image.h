#pragma once
#define GLIB_API
#define GLIB_CLASS

#define JASC_HEADER16			"JASC-PAL\r\n0100\r\n16\r\n"

#define PadWidth(width,depth)	(((depth*width+31)&~31)/8)
#define PalSize(depth)			(1<<depth)

#define GetU16Le(val)			(USHORT)(((val)<<8)|(((USHORT)val)>>8))

#define RGBQ(r,g,b)				(((u8)b)|(((u8)g)<<8)|(((u8)r)<<16))
#define QGET_R(x)				(((x)>>16) & 0xFF)
#define QGET_G(x)				(((x)>>8 ) & 0xFF)
#define QGET_B(x)				((x) & 0xFF)

enum IMAGE_TYPE
{
	IMAGE_BMP,
	IMAGE_TIM,
	IMAGE_PNG
};

GLIB_API int GetImageType(LPCTSTR name);
GLIB_API int GetImageType(BYTE *magic);

GLIB_API void ConvertB5G5R5(BYTE *src_row, BYTE *dst_row, int w);
GLIB_API void ConvertB8G8R8(BYTE *src_row, BYTE *dst_row, int w);
GLIB_API void ConvertDirect15(BYTE *src_row, BYTE *dst_row, int w);
GLIB_API void ConvertDirect15BGR(BYTE *src_row, BYTE *dst_row, int w);
GLIB_API void ConvertDirect24(BYTE *src_row, BYTE *dst_row, int w);
GLIB_API void ConvertTex24(BYTE *src_row, BYTE *dst_row, int w);

///////////////////////////////////////////////////////////////////////////////////
enum TIM_PMODE
{
	PMODE_4,	// 4 bit, using clut
	PMODE_8,	// 8 bit, using clut
	PMODE_15,	// 15 bits, direct
	PMODE_24,	// 24 bits, direct
	PMODE_MIXED	// tim archieve, unsupported!
};

#define TIMRGB(r,g,b)	( ((r)>>3) | (((g)>>3)<<5) | (((b)>>3)<<10) )

typedef struct TIM_HEADER
{
	ULONG id;			// always 0x10
	ULONG flag;			// 0~3: PMode, 4: CLUT section, 5~31: reserved (zero)
} TIM_HEADER;

typedef struct TIM_CLUT
{
	ULONG bnum;			// data length of CLUT block in bytes, includes itself
	USHORT x, y;		// framebuffer coordinates
	USHORT w, h;		// width and height
} TIM_CLUT;

typedef struct TIM_PIXEL
{
	ULONG bnum;			// data length of PIXEL block in bytes, includes itself
	USHORT x, y;		// framebuffer coordinates
	USHORT w, h;		// width (in blocks of 16-bit units) and height
} TIM_PIXEL;

class GLIB_CLASS Tim
{
public:
	Tim();
	~Tim();

	void Copy(const Tim *tim);

	void operator = (const Tim *tim) { Copy(tim); }
	void operator = (const Tim &tim) { Copy(&tim); }

	void Reset();

	void CreatePixel(int width, int height, int x, int y, int depth);
	void CreateClut(int width, int height, int x, int y);
	void ConvertBmp(void *img_ptr, int _width, int _height);

	void* LoadTim(LPCTSTR filename);
	size_t LoadTim(BYTE *buffer);
	int Save(u8* &dest);
	void Save(LPCTSTR filename);
	HBITMAP GetHandle();

	u16 GetClutColor(int x, int y);

	USHORT *clut;					// colors
	BYTE *image;					// pixel data
	int img_w, img_wp, img_h, img_x, img_y, img_depth;	// fast reference data
	int clut_w, clut_h, clut_x, clut_y;
};

///////////////////////////////////////////////////////////////////////////////////
class GLIB_CLASS Bitmap
{
public:
	Bitmap();
	~Bitmap();

	void* LoadBitmap(u8* data);
	void* LoadBitmap(BITMAPINFOHEADER *info);
	void* LoadBitmap(LPCTSTR filename);

	BITMAPINFOHEADER InfoHeader;	// data from the header
	RGBQUAD palette[256];			// colors
	BYTE *image;					// pixel data
	int width, height, depth;		// fast reference data
};

///////////////////////////////////////////////////////////////////////////////////
class GLIB_CLASS Image
{
public:
	Image();
	~Image();

	void Copy(const Image *source);

	void Create(int width, int height, int depth, RGBQUAD *palette);
	void* CreateFromBmp(Bitmap *bmp);
	void* CreateFromTim(Tim *tim, int pal, COLORREF transmask=RGB(255,0,255));
	void* CreateFromSpec(BYTE *image, RGBQUAD *pal, int type, int _w, int _h);

	enum IMG_TYPE
	{
		IMG_MONO,		// bitmap 1 bpp
		IMG_BMP4,		// bitmap 4 bpp
		IMG_BMP8,		// bitmap 8 bpp
		IMG_BMP24,		// bitmap 24 bpp
		IMG_B5G5R5,		// tim 4 bpp
		IMG_B8G8R8,		// tim 8 bpp
		IMG_DIRECT15,	// tim 15 bpp
		IMG_DIRECT15BGR,// BGR 15 bpp
		IMG_DIRECT24,	// tim 24 bpp
		IMG_TEX24		// tex 24 bpp
	};

	void operator = (const Image *source) { Copy(source);  }
	void operator = (const Image &source) { Copy(&source); }

	void SaveBitmap(LPCTSTR name);
	void SavePng(LPCTSTR name);

	void* LoadFromFile(LPCTSTR name);
	HBITMAP GetHandle();
	void ClutToPalette(USHORT *clut, COLORREF transmask=RGB(255,0,255));

	void InitRows();
	u32 GetPixelAt(int x, int y);
	u32 GetPixelIndex(int x, int y);
	u32 GetPixelPng(int x, int y);
	void SetPixelAt(int x, int y, u32 pixel);
	bool BitBlit(Image *src, int sx, int sy, int w, int h, int dx, int dy, u32 direction);

	RGBQUAD palette[256];
	BYTE *image;
	int width, height, depth;

	BYTE **rows;
	int row_w;	// width for each scanline in bytes

	enum direction
	{
		dir_normal,
		dir_mirror,
		dir_flip
	};
};

///////////////////////////////////////////////////////////////////////////////////
class GLIB_CLASS Palette
{
public:
	Palette(){ZeroMemory(palette,sizeof(RGBQUAD)*4);};
	~Palette(){};

	RGBQUAD palette[256];
};

///////////////////////////////////////////////////////////////////////////////////

/* reads 4 bytes using big-endian order */
GLIB_API u32 BmpIntToInt(BYTE *bmp);
/* read int as big endian */
GLIB_API u32 BmpIntToInt(u32 bmp);
/* convert rgb structure to a clut color */
GLIB_API USHORT RgbToClut(RGBQUAD rgb, u32 transparent=0xFF00FF);
/* convert clut color to an rgb structure */
GLIB_API RGBQUAD ClutToRgb(USHORT clut, u32 transparent=0xFF00FF);
GLIB_API RGBQUAD B5G5R5ToRgb(USHORT clut, u32 transparent=0);
/* convert image object to a clut palette */
GLIB_API int ImageToClut(Image *image, int x, int y, USHORT *paldest, int count);
/* convert image object to an rgb palette */
GLIB_API int ImageToRgb(Image *image, int x, int y, RGBQUAD *paldest, int count);
/* returns an image upscaled to 32bpp true color */
GLIB_API void* ImageToTrueColor(Image *img, COLORREF transmask);
GLIB_API void ImageToTrueColor(Image *img, BYTE *dest, COLORREF transmask);
GLIB_API void* PalImageToTrueColor(Image *img, RGBQUAD *pal, COLORREF transmask);
GLIB_API void PalImageToTrueColor(Image *img, RGBQUAD *pal, BYTE *dest, COLORREF transmask);
