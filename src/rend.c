#include <stdio.h>
#include "rend.h"
#include "app.h"
#include "level.h"
#include "tiles.h"
#include "vga.h"

void draw_level_cell(struct level *lvl, struct level_cell *cell, int layer, int destx, int desty)
{
	static const int offs[][2] = {
		{-TILE_XSZ / 2, 0},
		{-TILE_XSZ, TILE_YSZ / 2},
		{0, TILE_YSZ / 2},
		{-TILE_XSZ / 2, TILE_YSZ}
	};
	int i, x, y;
	struct tileimg *tile;

	for(i=0; i<4; i++) {
		if((tile = get_cell_tile(lvl, cell, i, layer))) {
			x = destx + offs[i][0];
			y = desty - tile->height + offs[i][1];
			tiles_blit_rle(tile, x, y, cur_bpl);
		}
	}
}


enum {
	IN		= 0,
	LEFT	= 1,
	RIGHT	= 2,
	TOP		= 4,
	BOTTOM	= 8
};

static int outcode(int x, int y, int xmin, int ymin, int xmax, int ymax)
{
	int code = 0;

	if(x < xmin) {
		code |= LEFT;
	} else if(x > xmax) {
		code |= RIGHT;
	}
	if(y < ymin) {
		code |= TOP;
	} else if(y > ymax) {
		code |= BOTTOM;
	}
	return code;
}

#define LERP(a, b, t)	((a) + (((b) - (a)) * (t) >> 8))

int clip_line(int *x0, int *y0, int *x1, int *y1, int xmin, int ymin, int xmax, int ymax)
{
	int oc_out, oc0, oc1;
	int32_t fx0, fy0, fx1, fy1, fxmin, fymin, fxmax, fymax, a, b;

	oc0 = outcode(*x0, *y0, xmin, ymin, xmax, ymax);
	oc1 = outcode(*x1, *y1, xmin, ymin, xmax, ymax);

	if(!(oc0 | oc1)) return 1;	/* both points are inside */

	fx0 = *x0 << 8;
	fy0 = *y0 << 8;
	fx1 = *x1 << 8;
	fy1 = *y1 << 8;
	fxmin = xmin << 8;
	fymin = ymin << 8;
	fxmax = xmax << 8;
	fymax = ymax << 8;

	for(;;) {
		long x, y, t;

		if(oc0 & oc1) return 0;		/* both have points with the same outbit, not visible */
		if(!(oc0 | oc1)) break;		/* both points are inside */

		oc_out = oc0 ? oc0 : oc1;

		if(oc_out & TOP) {
			t = ((fymin - fy0) << 8) / (fy1 - fy0);
			x = LERP(fx0, fx1, t);
			y = fymin;
		} else if(oc_out & BOTTOM) {
			t = ((fymax - fy0) << 8) / (fy1 - fy0);
			x = LERP(fx0, fx1, t);
			y = fymax;
		} else if(oc_out & LEFT) {
			t = ((fxmin - fx0) << 8) / (fx1 - fx0);
			x = fxmin;
			y = LERP(fy0, fy1, t);
		} else /*if(oc_out & RIGHT)*/ {
			t = ((fxmax - fx0) << 8) / (fx1 - fx0);
			x = fxmax;
			y = LERP(fy0, fy1, t);
		}

		if(oc_out == oc0) {
			fx0 = x;
			fy0 = y;
			oc0 = outcode(fx0 >> 8, fy0 >> 8, xmin, ymin, xmax, ymax);
		} else {
			fx1 = x;
			fy1 = y;
			oc1 = outcode(fx1 >> 8, fy1 >> 8, xmin, ymin, xmax, ymax);
		}
	}

	*x0 = fx0 >> 8;
	*y0 = fy0 >> 8;
	*x1 = fx1 >> 8;
	*y1 = fy1 >> 8;
	return 1;
}


void draw_line(int x0, int y0, int x1, int y1, uint8_t color)
{
	int i, dx, dy, dx2, dy2, xinc, yinc, err;
	uint8_t *vmem;

#ifdef VGA_LFB
	if(cur_bpl) return;
#endif

	vmem = vga_backbuf + y0 * SCANLEN + x0;
	dx = x1 - x0;
	dy = y1 - y0;

	if(dx >= 0) {
		xinc = 1;
	} else {
		xinc = -1;
		dx = -dx;
	}
	if(dy >= 0) {
		yinc = SCANLEN;
	} else {
		yinc = -SCANLEN;
		dy = -dy;
	}
	dx2 = dx << 1;
	dy2 = dy << 1;

	if(dx > dy) {
		/* x-major */
		err = dy2 - dx;
		for(i=0; i<=dx; i++) {
			*vmem = color;
			if(err >= 0) {
				err -= dx2;
				vmem += yinc;
			}
			err += dy2;
			vmem += xinc;
		}
	} else {
		/* y-major */
		err = dx2 - dy;
		for(i=0; i<=dy; i++) {
			*vmem = color;
			if(err >= 0) {
				err -= dy2;
				vmem += xinc;
			}
			err += dx2;
			vmem += yinc;
		}
	}
}
