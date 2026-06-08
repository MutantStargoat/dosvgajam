#include <stdio.h>
#include <limits.h>
#include <assert.h>
#include "level.h"
#include "tiles.h"
#include "util.h"
#include "dynarr.h"
#include "mob.h"
#include "app.h"
#include "psys.h"

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

	lvl->mobs = dynarr_alloc_nf(0, sizeof *lvl->mobs);
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

	for(i=0; i<dynarr_size(lvl->mobs); i++) {
		free_mob(lvl->mobs[i]);
	}
	dynarr_free(lvl->mobs);
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

int scrvec_to_dir8(int dx, int dy)
{
	unsigned int mask;
	static const int dirlut[] = {-1, DIR8_E, DIR8_W, -1, DIR8_S, DIR8_SE,
		DIR8_SW, -1, DIR8_N, DIR8_NE, DIR8_NW, -1, -1, -1, -1, -1};

	/* bits NSWE */
	mask = ((dy >> 28) & 8) | ((dx >> 30) & 2) | ((dy > 0) << 2) | (dx > 0);
	return dirlut[mask];
}

int gridvec_to_dir8(int32_t dx, int32_t dy)
{
	static int diag[] = {DIR8_S, DIR8_W, DIR8_E, DIR8_N};
	unsigned int flip = 0;

	if(dx < 0) {
		dx = -dx;
		flip = 1;
	}
	if(dy < 0) {
		dy = -dy;
		flip |= 2;
	}

	/*if((dy << 7) < dx * 53) { */
	if((dy << 7) < (dx << 6) - (dx << 3)) {
		/* slope less than 0.4142 -> 22.5 deg */
		return flip & 1 ? DIR8_NW : DIR8_SE;
	}
	/*if((dx << 7) < dy * 53) { */
	if((dx << 7) < (dy << 6) - (dy << 3)) {
		/* slope above 2.4142 -> between 67.5 and 90 deg */
		return flip & 2 ? DIR8_NE : DIR8_SW;
	}
	/* slope between 0.4142 and 2.4142 -> between 22.5 and 67.5 deg */
	return diag[flip];
}

void cell_add_beamseg(struct level_cell *cell, struct beamseg *bs)
{
	bs->cell = cell;
	bs->next = cell->beamsegs;
	cell->beamsegs = bs;
}

int cell_remove_beamseg(struct level_cell *cell, struct beamseg *bs)
{
	int found = 0;
	struct beamseg dummy, *prev;

	dummy.next = cell->beamsegs;
	prev = &dummy;
	while(prev->next) {
		if(prev->next == bs) {
			prev->next = bs->next;
			found = 1;
			break;
		}
		prev = prev->next;
	}
	cell->beamsegs = dummy.next;

	bs->next = 0;
	bs->cell = 0;
	return found;
}

void cell_add_psys(struct level_cell *cell, struct psys *ps)
{
	ps->next = cell->psys;
	cell->psys = ps;
}

int cell_remove_psys(struct level_cell *cell, struct psys *ps)
{
	struct psys dummy, *prev;

	dummy.next = cell->psys;
	prev = &dummy;
	while(prev->next) {
		if(prev->next == ps) {
			prev->next = ps->next;
			ps->next = 0;
			cell->psys = dummy.next;
			return 1;
		}
		prev = prev->next;
	}
	return 0;
}

void cell_spawn_psys(struct level_cell *cell, int32_t x, int32_t y, int32_t dx,
		int32_t dy, struct psys *templ)
{
	struct psys *ps;

	ps = malloc_nf(sizeof *ps);
	*ps = *templ;
	grid_to_vscr(x, y, &ps->x, &ps->y);
	ps->y -= 16;

	ps->dirx = dx;
	ps->diry = dy;

	psys_add_emitter(&cell->psys, ps);
}

struct mob *raycast(struct level *lvl, int32_t x, int32_t y, int32_t dx, int32_t dy, struct mob *ignmob)
{
	int i, cx, cy, exit_dir;
	int32_t hslope, vslope, nx, ny, rdx, rdy, hitx, hity;
	struct level_cell *cell;
	struct mob *cellmob, *hitmob;
	int32_t hit_dist, t;

	grid_to_cell(x, y, &cx, &cy);
	if(!(cell = get_level_cell(lvl, cx, cy))) {
		return 0;
	}

	hslope = dx ? (dy << 8) / dx : 256;
	vslope = dy ? (dx << 8) / dy : 256;

	for(i=0; i<MAX_BEAM_SEG; i++) {
		exit_dir = ray_step(lvl, x, y, dx, dy, hslope, vslope, &nx, &ny);

		rdx = nx - x;
		rdy = ny - y;

		hitmob = 0;
		hit_dist = INT_MAX;

		if(ignmob != &player) {
			if((t = mob_rayhit(&player, x, y, rdx, rdy, &hitx, &hity)) >= 0) {
				hitmob = &player;
				hit_dist = t;
			}
		}

		cellmob = cell->mobs;
		while(cellmob) {
			if(cellmob != ignmob) {
				if((t = mob_rayhit(cellmob, x, y, rdx, rdy, &hitx, &hity)) >= 0 && t < hit_dist) {
					hitmob = cellmob;
					hit_dist = t;
				}
			}
			cellmob = cellmob->next;
		}

		if(hitmob) {
			return hitmob;
		}

		if(!(cell->flags & CELL_EXIT(exit_dir))) {
			break;
		}

		switch(exit_dir) {
		case DIR_E:
			if(cell->cx >= lvl->size - 1) goto break_loop;
			cell++;
			break;
		case DIR_W:
			if(cell->cx <= 0) goto break_loop;
			cell--;
			break;
		case DIR_S:
			if(cell->cy >= lvl->size - 1) goto break_loop;
			cell += lvl->size;
			break;
		case DIR_N:
			if(cell->cy <= 0) goto break_loop;
			cell -= lvl->size;
			break;
		}

		x = nx;
		y = ny;
	}
break_loop:
	return 0;
}

