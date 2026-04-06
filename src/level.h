#ifndef LEVEL_H_
#define LEVEL_H_

#include "szint.h"
#include "util.h"

#define NUM_LAYERS	2

#define TILE_XSZ	32
#define TILE_YSZ	16

#define CELL_XSZ	(TILE_XSZ << 1)
#define CELL_YSZ	(TILE_YSZ << 1)

struct tilemap_layer {
	unsigned short *tiles;
};

struct tilemap {
	int sz, shift;
	struct tilemap_layer layer[NUM_LAYERS];
};

enum {
	CELL_EXIT_N	= 0x100,
	CELL_EXIT_W	= 0x200,
	CELL_EXIT_S	= 0x400,
	CELL_EXIT_E	= 0x800
};

struct level_cell {
	unsigned short flags;
};

struct level {
	struct tilemap tilemap;
	struct level_cell *cells;
};

/* conversion between virtual screen and grid coordinates (24.8 fixed point) */
static INLINE void vscr_to_grid(int sx, int sy, int32_t *gridx, int32_t *gridy)
{
	sx <<= 2;
	sy <<= 3;
	*gridx = (sy + sx) >> 1;
	*gridy = (sy - sx) >> 1;
}

static INLINE void grid_to_vscr(int32_t gridx, int32_t gridy, int *sx, int *sy)
{
	*sx = (gridx - gridy) >> 2;
	*sy = (gridx + gridy) >> 3;
}

/* conversion between virtual screen and cell indices */
static INLINE void vscr_to_cell(int sx, int sy, int *cx, int *cy)
{
	int32_t gx, gy;
	vscr_to_grid(sx, sy, &gx, &gy);
	*cx = gx >> 8;
	*cy = gy >> 8;
}

static INLINE void cell_to_vscr(int cx, int cy, int *sx, int *sy)
{
	grid_to_vscr(cx << 8, cy << 8, sx, sy);
}

#endif	/* LEVEL_H_ */
