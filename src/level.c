#include <stdio.h>
#include <assert.h>
#include "level.h"
#include "tiles.h"
#include "util.h"

struct tileset tileset;

int create_level(struct level *lvl, int sz, int nlayers)
{
	int i;

	lvl->tset = 0;

	assert(nlayers <= MAX_LAYERS);

	lvl->num_layers = nlayers;
	lvl->size = sz;
	lvl->shift = calc_shift(sz);

	if(!(lvl->cells = calloc(sz * sz, sizeof *lvl->cells))) {
		fprintf(stderr, "create_level: failed to allocate %dx%d cells\n", sz, sz);
		return -1;
	}

	sz <<= 1;
	lvl->tmap_size = sz;
	lvl->tmap_shift = calc_shift(sz);

	for(i=0; i<nlayers; i++) {
		if(!(lvl->tmap[i] = calloc(sz * sz, sizeof *lvl->tmap[i]))) {
			fprintf(stderr, "create_level: failed to allocate %dx%d tilemap layer\n", sz, sz);
			while(--i >= 0) free(lvl->tmap[i]);
			free(lvl->cells);
			return -1;
		}
	}
	return 0;
}

void destroy_level(struct level *lvl)
{
	int i;

	if(!lvl) return;

	free(lvl->cells);
	for(i=0; i<lvl->num_layers; i++) {
		free(lvl->tmap[i]);
	}
}

/* NOTE load_level is defined in tiledfmt.c */

struct level_cell *get_level_cell(struct level *lvl, int cx, int cy)
{
	if(!BOUNDCHK(cx, lvl->size) || !BOUNDCHK(cy, lvl->size)) {
		return 0;
	}
	return lvl->cells + (cy << lvl->shift) + cx;
}

struct level_cell *get_level_cell_vscr(struct level *lvl, int sx, int sy)
{
	int cx, cy;
	vscr_to_cell(sx, sy, &cx, &cy);
	return get_level_cell(lvl, cx, cy);
}

struct tileimg *get_cell_tile(struct level *lvl, struct level_cell *cell, int n, int layer)
{
	int tx, ty;
	struct tileimg **mapptr;

	tx = cell->cx << 1;
	ty = cell->cy << 1;
	mapptr = lvl->tmap[layer] + (ty << lvl->tmap_shift) + tx;

	switch(n) {
	case 0:
		return mapptr[0];
	case 1:
		return mapptr[lvl->tmap_size];
	case 2:
		return mapptr[1];
	case 3:
		return mapptr[lvl->tmap_size + 1];
	default:
		break;
	}
	return 0;
}

void calc_cell_height(struct level *lvl, struct level_cell *cell)
{
	static const int tile_yoffs[] = {0, -TILE_YSZ/2, -TILE_YSZ/2, -TILE_YSZ};
	int i, j, tile_height;
	struct tileimg *tile;

	cell->height = 0;
	for(i=0; i<lvl->num_layers; i++) {
		for(j=0; j<4; j++) {
			if((tile = get_cell_tile(lvl, cell, j, i))) {
				tile_height = tile->height + tile_yoffs[j] + tile->yorg;
				if(tile_height > cell->height) {
					cell->height = tile_height;
				}
			}
		}
	}
}

const char *strcellflags(unsigned int flags)
{
	static char str[8];

	sprintf(str, "%c%c/%c%c%c%c", flags & CELL_OPEN ? 'o' : 'x', flags & CELL_WALK ? 'w' : 'n',
			flags & CELL_EXIT_N ? 'n' : '-', flags & CELL_EXIT_W ? 'w' : '-',
			flags & CELL_EXIT_S ? 's' : '-', flags & CELL_EXIT_E ? 'e' : '-');
	return str;
}
