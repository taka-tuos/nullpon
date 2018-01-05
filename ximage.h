#include <stdlib.h>
#include <string.h>

#ifndef __XIMAGE__
#define __XIMAGE__

typedef struct {
	unsigned int *pixels;
	int w,h,p;
} ximage;

typedef enum {
	FSIZ_12,
	FSIZ_14,
	FSIZ_16
} ximage_textsize;

void ximage_init(void);
ximage *ximage_create(int w, int h, int p, void *pixels);
ximage *ximage_load(char *f);
void ximage_bitblt(ximage *dst, ximage *src, int x, int y);
void ximage_boxfill(ximage *dst, int x0, int y0, int x1, int y1, int c);
void ximage_lineto(ximage *dst, int x0, int y0, int x1, int y1, int c);
void ximage_boxnfill(ximage *dst, int x0, int y0, int x1, int y1, int c, int t);
void ximage_delete(ximage *dst);
void ximage_textout(ximage *dst, int x, int y, int c, int mode, char *sz);
void ximage_textoutformat(ximage *dst, int x, int y, int c, int mode, char *fmt, ...);
void ximage_fsize(ximage_textsize size);

#endif
