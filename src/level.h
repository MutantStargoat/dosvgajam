#ifndef LEVEL_H_
#define LEVEL_H_

#include "szint.h"
#include "util.h"

#define NUM_LAYERS	2

#define TILE_XSZ	32
#define TILE_YSZ	16

#define CELL_XSZ	(TILE_XSZ << 1)
#define CELL_YSZ	(TILE_YSZ << 1)

struct rect {
	unsigned int x, y, w, h;
};

struct tilemap_layer {
	unsigned int *tiles;
};

struct tilemap {
	int sz, shift;
	struct tilemap_layer layer[NUM_LAYERS];
};

enum {
	CELL_EXIT_N	= 0x0100,
	CELL_EXIT_W	= 0x0200,
	CELL_EXIT_S	= 0x0400,
	CELL_EXIT_E	= 0x0800
};

struct level_cell {
	int cx, cy;				/* cell coordinates */
	unsigned int flags;

	struct level_cell *dirty_next;
};

struct level {
	struct tilesheet *tsheet;
	struct tilemap tmap;

	int size, shift;
	struct level_cell *cells;
};

int create_level(struct level *lvl, int sz);
void destroy_level(struct level *lvl);

/* get hte cell cx,cy */
struct level_cell *get_level_cell(struct level *lvl, int cx, int cy);
/* get the cell at virtual screen coords sx, sy */
struct level_cell *get_level_cell_vscr(struct level *lvl, int sx, int sy);

struct tileimg *get_cell_tile(struct level *lvl, struct level_cell *cell, int n, int layer);

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
