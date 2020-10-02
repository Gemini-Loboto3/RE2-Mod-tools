#include <stdafx.h>
#include <stdint.h>
#include "Bitmap.h"

#define getByte(value, n) (value >> (n*8) & 0xFF)
 
static __inline uint32_t getpixel(CBitmap *image, unsigned int x, unsigned int y)
{
	return image->getPixel(x, y);
}

static __inline float lerp(float s, float e, float t)
{
	return s+(e-s)*t;
}

static __inline float blerp(float c00, float c10, float c01, float c11, float tx, float ty)
{
	return lerp(lerp(c00, c10, tx), lerp(c01, c11, tx), ty);
}

static __inline void putpixel(CBitmap *image, unsigned int x, unsigned int y, uint32_t color)
{
	image->setPixel(x, y, color & 0xfff8f8f8);
}

void Bilinear_scale(CBitmap *src, CBitmap *dst, float scalex, float scaley)
{
	int newWidth = (int)(src->w*scalex);
	int newHeight= (int)(src->h*scaley);
	int x, y;

	dst->Create(newWidth, newHeight);

	for(x= 0, y=0; y < newHeight; x++)
	{
		if(x > newWidth) { x = 0; y++; }
		float gx = x / (float)(newWidth) * (src->w-1);
		float gy = y / (float)(newHeight) * (src->h-1);
		int gxi = (int)gx;
		int gyi = (int)gy;
		uint32_t result=0;
		uint32_t c00 = getpixel(src, gxi, gyi);
		uint32_t c10 = getpixel(src, gxi+1, gyi);
		uint32_t c01 = getpixel(src, gxi, gyi+1);
		uint32_t c11 = getpixel(src, gxi+1, gyi+1);
		uint8_t i;
		for(i = 0; i < 3; i++)
			result |= (uint8_t)blerp(getByte(c00, i), getByte(c10, i), getByte(c01, i), getByte(c11, i), gx - gxi, gy -gyi) << (8*i);
		putpixel(dst,x, y, result);
	}
}