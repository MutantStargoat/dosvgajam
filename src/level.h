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

#define CELL_EXIT(dir)		(CELL_EXIT_N << (dir))

struct mob;
struct psys;

struct level_cell {
	int cx, cy;				/* cell coordinates */
	int x, y;				/* place for computed pixel coordinates */
	int height;				/* maximum height of tiles/sprites in cell, for vis */
	unsigned int flags;

	struct mob *mobs;			/* linked list */
	struct beamseg *beamsegs;	/* linked list */
	struct psys *psys;			/* linked list */
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

void cell_add_beamseg(struct level_cell *cell, struct beamseg *bs);
int cell_remove_beamseg(struct level_cell *cell, struct beamseg *bs);

void cell_add_psys(struct level_cell *cell, struct psys *ps);
int cell_remove_psys(struct level_cell *cell, struct psys *ps);

struct mob *raycast(struct level *lvl, int32_t x, int32_t y, int32_t dx, int32_t dy, struct mob *ignmob);

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

static INLINE int ray_step(struct level *lvl, int32_t x, int32_t y, int32_t dx,
		int32_t dy, int32_t hslope, int32_t vslope,	int32_t *nx, int32_t *ny)
{
	int32_t x1, y1, offs;

	/* convert to origin at upper-left, rather than center of cells, to make
	 * stepping simpler
	 */
	x += 128;
	y += 128;

	/* find next boundaries when stepping horizontally or vertically */
	x1 = (dx > 0 ? x + 256 : x - 1) & ~0xff;
	/*hslope = (dy << 8) / dx;*/
	y1 = y + ((hslope * (x1 - x)) >> 8);
	offs = y1 - (y & ~0xff);

	if(offs < 256 && offs >= 0) {
		*nx = x1 - 128;
		*ny = y1 - 128;
		return dx > 0 ? DIR_E : DIR_W;
	}

	y1 = (dy > 0 ? y + 256 : y - 1) & ~0xff;
	/*vslope = (dx << 8) / dy;*/
	x1 = x + ((vslope * (y1 - y)) >> 8);

	*nx = x1 - 128;
	*ny = y1 - 128;
	return dy > 0 ? DIR_S : DIR_N;
}


#endif	/* LEVEL_H_ */
