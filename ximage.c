#include "ximage.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "nvbdflib.h"

#include <iconv.h>

ximage_textsize nowsiz;
ximage *textout;
int textout_color;

BDF_FONT *bdf_asc[3];
BDF_FONT *bdf_jis[3];

iconv_t ic;

void x_memcpy(void *dst, void *src, int size)
{
	while(size) if((((unsigned int *)src)[--size] & 0x80000000) && (((unsigned int *)dst)[size] = ((unsigned int *)src)[size]));
}

void x_memset(void *dst, unsigned int data, unsigned int size)
{
	while(size) ((unsigned int *)dst)[--size] = data;
}

void x_bdfdot(int x, int y, int c)
{
	textout->pixels[y * textout->p + x] = c * textout_color;
}

void ximage_monoalpha(unsigned int *p, int sx, int sy)
{
	int x, y;
	int bayerpattern[] = {
		0,  8,  2, 10,
		12,  4, 14,  6,
		3, 11,  1,  9,
		15,  7, 13,  5
	};
	
	for(y = 0; y < sy; y++) {
		for(x = 0; x < sx; x++) {
			int bayer = bayerpattern[(y&3) * 4 + (x&3)] * 16 + 8;
			int alpha = p[y * sx + x] >> 24;
			//int alpha = 0x80;
			alpha = alpha > bayer ? 0xff : 0x00;
			p[y * sx + x] = (p[y * sx + x] & 0xffffff) | (alpha << 24);
		}
	}
}

void ximage_swaprgb(unsigned int *p, int sx, int sy)
{
	int x, y;
	
	for(y = 0; y < sy; y++) {
		for(x = 0; x < sx; x++) {
			int c = p[y * sx + x];
			int a = (c >> 24) & 0xff;
			int r = (c >> 16) & 0xff;
			int g = (c >>  8) & 0xff;
			int b = (c >>  0) & 0xff;
			
			p[y * sx + x] = (a << 24) | (b << 16) | (g << 8) | r;
		}
	}
}

void ximage_init(void)
{
	bdf_asc[0] = bdfReadPath("resource/shnm6x12r.bdf");
	bdf_asc[1] = bdfReadPath("resource/shnm7x14r.bdf");
	bdf_asc[2] = bdfReadPath("resource/shnm8x16r.bdf");
	
	bdf_jis[0] = bdfReadPath("resource/shnmk12.bdf");
	bdf_jis[1] = bdfReadPath("resource/shnmk14.bdf");
	bdf_jis[2] = bdfReadPath("resource/shnmk16.bdf");
	
	bdfSetDrawingFunction(x_bdfdot);
	
	ic = iconv_open("SHIFT-JIS", "UTF-8");
}

ximage *ximage_load(char *f)
{
	ximage *obj = (ximage *)malloc(sizeof(ximage));
	int dummy;
	
	obj->pixels = (unsigned int *)stbi_load(f,&obj->w,&obj->h,&dummy,4);
	
	ximage_swaprgb(obj->pixels,obj->w,obj->h);
	
	ximage_monoalpha(obj->pixels,obj->w,obj->h);
	
	obj->p = obj->w;
	
	return obj;
}

ximage *ximage_create(int w, int h, int p, void *pixels)
{
	ximage *obj = (ximage *)malloc(sizeof(ximage));
	
	obj->pixels = (uint32_t *)pixels;
	
	obj->w = w;
	obj->h = h;
	obj->p = p;
	
	return obj;
}

void ximage_bitblt(ximage *dst, ximage *src, int x, int y)
{
	int i;
	int ox=0,oy=0;
	int ex=0,ey=0;
	
	if(x < 0) ox = (0 - x);
	if(y < 0) oy = (0 - y);
	
	if(dst->w < (x+src->w)) ex = ((x+src->w) - dst->w) - ox;
	if(dst->h < (y+src->h)) ey = ((y+src->h) - dst->h) - oy;
	
	for(i = y + oy; i < y + src->h - oy - ey; i++) {
		x_memcpy(dst->pixels + (i * dst->p + (x+ox)), src->pixels + (((i-y)-oy) * src->p + (ox)), src->w - ox - ex);
	}
}

void ximage_boxfill(ximage *dst, int x0, int y0, int x1, int y1, int c)
{
	int i;
	int ox=0,oy=0;
	int ex=0,ey=0;
	
	if(x0 < 0) ox = (0 - x0);
	if(y0 < 0) oy = (0 - y0);
	
	if(dst->w < x1) ex = (x1 - dst->w) - ox;
	if(dst->h < y1) ey = (y1 - dst->h) - oy;
	
	for(i = y0 + oy; i < y1 - oy - ey; i++) {
		x_memset(dst->pixels + (i * dst->p + (x0+ox)), c | 0xff000000, (x1 - x0) - ox - ex);
	}
}

void ximage_boxnfill(ximage *dst, int x0, int y0, int x1, int y1, int c, int t)
{
	ximage_boxfill(dst,x0+0,y0+0,x1+0,y0+t,c);
	ximage_boxfill(dst,x0+0,y1-t,x1+0,y1+0,c);
	
	ximage_boxfill(dst,x0+0,y0+0,x0+t,y1+0,c);
	ximage_boxfill(dst,x1-t,y0+0,x1+0,y1+0,c);
}


void ximage_lineto(ximage *dst, int x0, int y0, int x1, int y1, int c)
{
	int dx = abs(x1-x0), sx = x0<x1 ? 1 : -1;
	int dy = abs(y1-y0), sy = y0<y1 ? 1 : -1; 
	int err = (dx>dy ? dx : -dy)/2, e2;
 
	while(1) {
		if(x0 >= 0 && y0 >= 0 && x0 < dst->w && y0 < dst->h)((int *)dst->pixels)[y0 * dst->p + x0] = c;
		if (x0==x1 && y0==y1) break;
		e2 = err;
		if (e2 >-dx) { err -= dy; x0 += sx; }
		if (e2 < dy) { err += dx; y0 += sy; }
	}
}

void ximage_delete(ximage *dst)
{
	free(dst);
}

void ximage_textout(ximage *dst, int x, int y, int c, int mode, char *sz)
{
	int flst[] = { 12, 14, 16 };
	int ox,oy;
	int od = flst[(int)nowsiz];
	
	bdfSetDrawingAreaSize(dst->w,dst->h);
	
	switch(mode) {
	case 0:
		ox=0,oy=0;
		break;
	case 1:
		ox=(strlen(sz)*od/2),oy=od;
		break;
	case 2:
		ox=(strlen(sz)*od),oy=0;
		break;
	}
	
	int byte1 = 0;
	
	size_t wren = 2048;
	size_t rren = strlen(sz);
	
	char s1[2048+1];
	char s2[2048+1];
	
	char *si1 = s1;
	char *si2 = s2;
	
	strcpy(s1,sz);
	
	iconv(ic,&si1,&rren,&si2,&wren);
	
	unsigned char *s = (unsigned char *)s2;
	
	textout_color = c;
	textout = dst;
	
	BDF_FONT *asc = bdf_asc[(int)nowsiz];
	BDF_FONT *jis = bdf_jis[(int)nowsiz];

	for(;*s!=0;s++) {
		if(byte1 == 0) {
			if ((0x81 <= *s && *s <= 0x9f) || (0xe0 <= *s && *s <= 0xfc)) {
				byte1 = *s;
			} else {
				bdfPrintCharacter(asc, x, y, *s);
			}
		} else {
			int k,t;
			if (0x81 <= byte1 && byte1 <= 0x9f) {
				k = (byte1 - 0x81) * 2;
			} else {
				k = (byte1 - 0xe0) * 2 + 62;
			}
			if (0x40 <= *s && *s <= 0x7e) {
				t = *s - 0x40;
			} else if (0x80 <= *s && *s <= 0x9e) {
				t = *s - 0x80 + 63;
			} else {
				t = *s - 0x9f;
				k++;
			}
			bdfPrintCharacter(jis, x - od/2, y, (k+1+32)*256+(t+32+1));
			byte1 = 0;
		}
		x += od/2;
	}
}

void ximage_textoutformat(ximage *dst, int x, int y, int c, int mode, char *fmt, ...)
{
	char buf[1024];
	
	va_list ap;
	va_start(ap, fmt);
	
	vsnprintf(buf, sizeof(buf), fmt, ap);
	
	va_end(ap);
	
	ximage_textout(dst,x,y,c,mode,buf);
}

void ximage_fsize(ximage_textsize size)
{
	nowsiz = size;
}
