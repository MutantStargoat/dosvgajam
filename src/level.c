#include <stdio.h>
#include <assert.h>
#include "level.h"
#include "tiles.h"
#include "util.h"

struct tileset tileset;
int adjlut[CELL_XSZ * CELL_YSZ * 2];

static void gen_masks(void);

int create_level(struct level *lvl, int sz, int nlayers)
{
	int i;

	if(!adjlut[0]) gen_masks();

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

static void gen_masks(void)
{
	FILE *fp;
	int i, j, len, offs;
	int *uptr, *uptr2, *lptr, *lptr2;

	lptr = adjlut + CELL_YSZ / 2 * (CELL_XSZ * 2);
	uptr = lptr - CELL_XSZ * 2;

	for(i=0; i<CELL_YSZ/2; i++) {
		len = i * 2;
		offs = (CELL_XSZ - len) * 2;

		for(j=0; j<len; j++) {
			lptr2 = lptr + offs;
			uptr2 = uptr + offs;
			/* bottom left */
			lptr[j * 2] = -1;
			lptr[j * 2 + 1] = 1;
			/* bottom right */
			lptr2[j * 2] = 0;
			lptr2[j * 2 + 1] = 1;
			/* upper left */
			uptr[j * 2] = -1;
			uptr[j * 2 + 1] = -1;
			/* upper right */
			uptr2[j * 2] = 0;
			uptr2[j * 2 + 1] = -1;
		}

		lptr += CELL_XSZ * 2;
		uptr -= CELL_XSZ * 2;
	}

	if((fp = fopen("cellmask.ppm", "wb"))) {
		fprintf(fp, "P6\n%d %d\n255\n", CELL_XSZ, CELL_YSZ);

		lptr = adjlut;
		for(i=0; i<CELL_XSZ * CELL_YSZ; i++) {
			int r = lptr[0] ? 255 : 0;
			int g = lptr[0] || lptr[1] <= 0 ? 0 : 255;
			int b = lptr[1] < 0 ? 255 : 0;
			fputc(r, fp);
			fputc(g, fp);
			fputc(b, fp);
			lptr += 2;
		}

		fclose(fp);
	}
}
