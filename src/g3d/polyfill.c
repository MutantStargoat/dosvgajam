#include <stdio.h>
#include <string.h>
#include "g3d.h"
#include "vga.h"
#include "app.h"
#include "util.h"

#define PIXELCLIP

#ifdef VGA_LFB
#define PITCH	FB_WIDTH
#else
#define PITCH	(FB_WIDTH >> 2)
#endif

static void filltop(struct g3d_vertex *v0, struct g3d_vertex *v1, struct g3d_vertex *v2);
static void fillbot(struct g3d_vertex *v0, struct g3d_vertex *v1, struct g3d_vertex *v2);
static void fillspan(unsigned char *dest, int x, int len);


void g3d_polyfill(struct g3d_vertex *verts)
{
	int i, topidx, botidx, mididx;
	int32_t dx, dy, dym;
	int32_t v01x, v01y, v02x, v02y;
	struct g3d_vertex vtmp;

	/* calculate winding */
	v01x = verts[1].x - verts[0].x;
	v01y = verts[1].y - verts[0].y;
	v02x = verts[2].x - verts[0].x;
	v02y = verts[2].y - verts[0].y;
	if(!((v01x * v02y - v02x * v01y) & 0x80000000)) {
		return;
	}

	topidx = botidx = 0;
	for(i=1; i<3; i++) {
		if(verts[i].y < verts[topidx].y) {
			topidx = i;
		}
		if(verts[i].y > verts[botidx].y) {
			botidx = i;
		}
	}
	mididx = topidx + 1; if(mididx > 2) mididx = 0;
	if(mididx == botidx) {
		if(++mididx > 2) mididx = 0;
	}

	dy = verts[botidx].y - verts[topidx].y;
	if(dy == 0) return;

	dx = verts[botidx].x - verts[topidx].x;
	dym = verts[mididx].y - verts[topidx].y;
	vtmp.x = muldiv(dx, dym, dy) + verts[topidx].x;	/* dx * dym / dy + vtop.x */
	vtmp.y = verts[mididx].y;

	if(verts[topidx].y != verts[mididx].y) {
		filltop(verts + topidx, verts + mididx, &vtmp);
	}
	if(verts[mididx].y != verts[botidx].y) {
		fillbot(verts + mididx, &vtmp, verts + botidx);
	}
}

static void filltop(struct g3d_vertex *v0, struct g3d_vertex *v1, struct g3d_vertex *v2)
{
	struct g3d_vertex *vtmp;
	int x, line, lasty, len;
	int32_t xl, xr, dxl, dxr, slopel, sloper, dy;
	int32_t y0, y1, yoffs;
	unsigned char *fbptr;

	if(v1->x > v2->x) {
		vtmp = v1;
		v1 = v2;
		v2 = vtmp;
	}

	dy = v1->y - v0->y;
	dxl = v1->x - v0->x;
	dxr = v2->x - v0->x;
	slopel = (dxl << 8) / dy;
	sloper = (dxr << 8) / dy;

	y0 = (v0->y + 0x100) & 0xffffff00;	/* start from the next scanline */
	yoffs = y0 - v0->y;					/* offset of the next scanline */
	xl = v0->x + ((yoffs * slopel) >> 8);
	xr = v0->x + ((yoffs * sloper) >> 8);

	line = y0 >> 8;
	lasty = v1->y >> 8;
#ifdef PIXELCLIP
	if(lasty >= FB_HEIGHT) lasty = FB_HEIGHT - 1;
#endif
	x = xl >> 8;

	fbptr = g3d_fbpixels + line * PITCH;

	while(line <= lasty) {
		if(line >= 0) {
			len = ((xr + 0x100) >> 8) - (xl >> 8);
#ifdef PIXELCLIP
			if(line >= 0) {
				int start;
				if(x < 0) {
					len += x;
					start = 0;
				} else {
					start = x;
				}
				if(start + len > FB_WIDTH) len = FB_WIDTH - start;
				if(len > 0) fillspan(fbptr, start, len);
			}
#else
			if(len > 0) fillspan(fbptr, x, len);
#endif
		}

		xl += slopel;
		xr += sloper;
		x = xl >> 8;
		fbptr += PITCH;
		line++;
	}
}

static void fillbot(struct g3d_vertex *v0, struct g3d_vertex *v1, struct g3d_vertex *v2)
{
	struct g3d_vertex *vtmp;
	int x, line, lasty, len;
	int32_t xl, xr, dxl, dxr, slopel, sloper, dy;
	int32_t y0, y1, yoffs;
	unsigned char *fbptr;

	if(v0->x > v1->x) {
		vtmp = v0;
		v0 = v1;
		v1 = vtmp;
	}

	dy = v2->y - v0->y;
	dxl = v2->x - v0->x;
	dxr = v2->x - v1->x;
	slopel = (dxl << 8) / dy;
	sloper = (dxr << 8) / dy;

	y0 = (v0->y + 0x100) & 0xffffff00;	/* start from the next scanline */
	yoffs = y0 - v0->y;					/* offset of the next scanline */
	xl = v0->x + ((yoffs * slopel) >> 8);
	xr = v1->x + ((yoffs * sloper) >> 8);

	line = y0 >> 8;
	lasty = v2->y >> 8;
#ifdef PIXELCLIP
	if(lasty >= FB_HEIGHT) lasty = FB_HEIGHT - 1;
#endif
	x = xl >> 8;

	fbptr = g3d_fbpixels + line * PITCH;

	while(line <= lasty) {
		if(line >= 0) {
			len = ((xr + 0x100) >> 8) - (xl >> 8);
#ifdef PIXELCLIP
			if(line >= 0) {
				int start;
				if(x < 0) {
					len += x;
					start = 0;
				} else {
					start = x;
				}
				if(start + len > FB_WIDTH) len = FB_WIDTH - start;
				if(len > 0) fillspan(fbptr, start, len);
			}
#else
			if(len > 0) fillspan(fbptr, x, len);
#endif
		}

		xl += slopel;
		xr += sloper;
		x = xl >> 8;
		fbptr += PITCH;
		line++;
	}
}

#ifndef VGA_LFB
static void fillspan(unsigned char *dest, int x, int len)
{
	unsigned int mask = 0xf;
	int align;

	dest += x >> 2;

	if(len < 4) mask >>= 4 - len;

	/* handle the start of the span. The x offset alignment affects:
	 * 1. which bitplane to start from, adjust the plane mask accordingly.
	 * 2. how many pixels we write, adjust remaining length accordingly.
	 */
	align = x & 3;
	vga_planemask(mask << align);
	*dest++ = g3d_curcidx;
	len -= 4 - align;

	/* the middle part of the span is all written 4 pixels at a time by
	 * enabling all 4 bit planes.
	 */
	if(len >= 4) {
		vga_planemask(0xf);
		while(len >= 4) {
			*dest++ = g3d_curcidx;
			len -= 4;
		}
	}

	/* handle any leftovers at the end */
	if(len) {
		mask = 0xf >> (4 - len);
		vga_planemask(mask);
		*dest = g3d_curcidx;
	}
}
#else
static void fillspan(unsigned char *dest, int x, int len)
{
	memset(dest + x, g3d_curcidx, len);
}
#endif
