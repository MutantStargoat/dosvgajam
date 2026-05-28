#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "mob.h"
#include "level.h"
#include "util.h"

void init_mob(struct mob *mob)
{
	memset(mob, 0, sizeof *mob);
	mob->dir = rand() & 7;
	mob->state = MOB_INVALID;
	mob->hp = 100;
}

struct mob *create_mob(void)
{
	struct mob *mob;

	mob = malloc_nf(sizeof *mob);
	init_mob(mob);
	return mob;
}

void free_mob(struct mob *mob)
{
	free(mob);
}

int mob_move(struct mob *mob, int dx, int dy)
{
	int cx, cy, ncx, ncy;
	int32_t nx, ny;

	if(!(dx | dy)) {
		mob_state(mob, MOB_IDLE);
		return 0;
	}

	nx = mob->x + dx + dy;
	ny = mob->y - dx + dy;
	grid_to_cell(nx, ny, &ncx, &ncy);

	cx = mob->cell->cx;
	cy = mob->cell->cy;

	if(ncx != cx || ncy != cy) {
		if(ncx < cx) {
			if(!(mob->cell->flags & CELL_EXIT_W)) return 0;
		} else if(ncx > cx) {
			if(!(mob->cell->flags & CELL_EXIT_E)) return 0;
		}
		if(ncy < cy) {
			if(!(mob->cell->flags & CELL_EXIT_N)) return 0;
		} else if(ncy > cy) {
			if(!(mob->cell->flags & CELL_EXIT_S)) return 0;
		}

		if(!(mob->cell = get_level_cell(mob->lvl, ncx, ncy))) {
			return 0;
		}
	}

	mob->x = nx;
	mob->y = ny;
	mob->dir = scrvec_to_dir8(dx, dy);

	if(mob->state != MOB_WALK) {
		mob_state(mob, MOB_WALK);
	} else {
		mob->spr.frm++;
	}
	return 1;
}

void mob_lookat(struct mob *mob, int32_t x, int32_t y)
{
	int dx = x - mob->x;
	int dy = y - mob->y;
	mob->dir = gridvec_to_dir8(dx, dy);
}

void mob_state(struct mob *mob, int st)
{
	if(mob->state == st) return;

	mob->state = st;

	if(mob->spr.anim[st].seq[0]) {
		mob->spr.cur = mob->spr.anim + st;
		mob->spr.nfrm = mob->spr.cur->seq[mob->dir]->ntiles;
	} else {
		mob->spr.cur = 0;
		mob->spr.nfrm = 0;
	}
	mob->spr.frm = 0;
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
	x1 = (dx > 0 ? x + 256 : x) & ~0xff;
	/*hslope = (dy << 8) / dx;*/
	y1 = y + ((hslope * (x1 - x)) >> 8);
	offs = y1 - (y & ~0xff);

	if(offs < 256 && offs >= 0) {
		*nx = x1 - 128;
		*ny = y1 - 128;
		return dx > 0 ? DIR_E : DIR_W;
	}

	y1 = (dy > 0 ? y + 256 : y) & ~0xff;
	/*vslope = (dx << 8) / dy;*/
	x1 = x + ((vslope * (y1 - y)) >> 8);

	*nx = x1 - 128;
	*ny = y1 - 128;
	return dy > 0 ? DIR_S : DIR_N;
}

void mob_beam(struct mob *mob, int32_t tx, int32_t ty)
{
	int i, exit_dir;
	int32_t hslope, vslope, dx, dy, x, y, nx, ny;
	struct level *lvl = mob->lvl;
	struct level_cell *cell = mob->cell;
	struct beamseg *seg;

	/* first delete the previous beam if it's still active */
	for(i=0; i<mob->beam.nseg; i++) {
		seg = mob->beam.seg + i;
		cell_remove_beamseg(seg->cell, seg);
	}
	mob->beam.nseg = 0;

	x = mob->beam.x0 = mob->x;
	y = mob->beam.y0 = mob->y;

	dx = tx - mob->x;
	dy = ty - mob->y;

	hslope = dx ? (dy << 8) / dx : 256;
	vslope = dy ? (dx << 8) / dy : 256;

	for(i=0; i<MAX_BEAM_SEG; i++) {
		exit_dir = ray_step(lvl, x, y, dx, dy, hslope, vslope, &nx, &ny);

		seg = mob->beam.seg + i;
		seg->x0 = x;
		seg->y0 = y;
		seg->x1 = nx;
		seg->y1 = ny;
		seg->beam = &mob->beam;

		assert(cell->beamsegs == 0);
		cell_add_beamseg(cell, seg);
		mob->beam.nseg++;

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

	mob->beam.x1 = nx;
	mob->beam.y1 = ny;
}
