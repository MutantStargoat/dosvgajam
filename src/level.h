#ifndef LEVEL_H_
#define LEVEL_H_

#include "szint.h"
#include "util.h"
#include "tiles.h"

#define MAX_LAYERS	4

#define TILE_XSZ	32
#define TILE_YSZ	16

#define CELL_XSZ	(TILE_XSZ << 1)
#define CELL_YSZ	(TILE_YSZ << 1)

struct rect {
	int x, y, w, h;
};

enum { DIR_N, DIR_W, DIR_S, DIR_E };
enum { DIR8_N, DIR8_NW, DIR8_W, DIR8_SW, DIR8_S, DIR8_SE, DIR8_E, DIR8_NE };

enum {
	CELL_OPEN	= 0x0001,
	CELL_WALK	= 0x0003,	/* implies open */

	CELL_EXIT_N	= 0x0100,
	CELL_EXIT_W	= 0x0200,
	CELL_EXIT_S	= 0x0400,
	CELL_EXIT_E	= 0x0800,

	CELL_EXITS	= CELL_EXIT_N | CELL_EXIT_W | CELL_EXIT_S | CELL_EXIT_E
};

struct mob;

struct level_cell {
	int cx, cy;				/* cell coordinates */
	int x, y;				/* place for computed pixel coordinates */
	int height;				/* maximum height of tiles/sprites in cell, for vis */
	unsigned int flags;

	struct mob *mobs;		/* linked list */

	struct level_cell *dirty_next;
};

struct level {
	struct tileset *tset;		/* not owned by the level */

	int tmap_size, tmap_shift;
	struct tileimg **tmap[MAX_LAYERS];
	int num_layers;

	int size, shift;
	struct level_cell *cells;

	int startx, starty;		/* start cell where player initially spawns */

	struct mob **mobs;		/* dynarr */
};

extern struct tileset tileset;
extern int adjlut[CELL_XSZ * CELL_YSZ * 2];

int create_level(struct level *lvl, int sz, int nlayers);
void destroy_level(struct level *lvl);

int load_level(struct level *lvl, const char *fname);

/* get the cell cx,cy */
struct level_cell *get_level_cell(struct level *lvl, int cx, int cy);
/* get the cell at virtual screen coords sx, sy */
struct level_cell *get_level_cell_vscr(struct level *lvl, int sx, int sy);

struct tileimg *get_cell_tile(struct level *lvl, struct level_cell *cell, int n, int layer);

void calc_cell_height(struct level *lvl, struct level_cell *cell);

const char *strcellflags(unsigned int flags);

int scrvec_to_dir8(int dx, int dy);
int gridvec_to_dir8(int32_t dx, int32_t dy);

/* implicit in these conversions is the tile size: 64x32 */
static INLINE void vscr_to_grid(int sx, int sy, int32_t *gridx, int32_t *gridy)
{
	sy += CELL_YSZ / 2;
	sx <<= 3;
	sy <<= 4;
	*gridx = (sy + sx) >> 1;
	*gridy = (sy - sx) >> 1;
}

static INLINE void grid_to_vscr(int32_t gridx, int32_t gridy, int *sx, int *sy)
{
	*sx = (gridx - gridy) >> 3;
	*sy = (gridx + gridy) >> 4;
}

/* conversion between virtual screen and cell indices */
static INLINE void vscr_to_cell(int sx, int sy, int *col, int *row)
{
	int32_t gx, gy;
	vscr_to_grid(sx, sy, &gx, &gy);
	*col = gx >> 8;
	*row = gy >> 8;
}

static INLINE void cell_to_vscr(int cx, int cy, int *sx, int *sy)
{
	grid_to_vscr((cx) << 8, (cy) << 8, sx, sy);
}

static INLINE void grid_to_cell(int32_t gx, int32_t gy, int *cx, int *cy)
{
	*cx = (gx + 0x80) >> 8;
	*cy = (gy + 0x80) >> 8;
}

#endif	/* LEVEL_H_ */
