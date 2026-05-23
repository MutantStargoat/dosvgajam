#include <stdio.h>
#include <stdlib.h>
#include "mob.h"
#include "level.h"
#include "util.h"

struct mob *create_mob(void)
{
	struct mob *mob;

	mob = calloc_nf(1, sizeof *mob);
	mob->dir = rand() & 7;
	mob->state = MOB_IDLE;
	mob->hp = 100;
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
	mob->spr.cur = mob->spr.anim + st;
	mob->spr.frm = 0;
	mob->spr.nfrm = mob->spr.cur->seq[mob->dir]->ntiles;
}

static INLINE void ray_step(struct level *lvl, int32_t x, int32_t y, int32_t dx,
		int32_t dy, int32_t hslope, int32_t vslope,	int32_t *nx, int32_t *ny)
{
	int32_t hxx, hyy, vxx, vyy;

	/* convert to origin at upper-left, rather than center of cells, to make
	 * stepping simpler
	 */
	x += 128;
	y += 128;

	/* find next boundaries when stepping horizontally or vertically */
	hxx = (dx > 0 ? x + 256 : x) & ~0xff;
	/*hslope = (dy << 8) / dx;*/
	hyy = (hslope * (hxx - x)) >> 8;

	vyy = (dy > 0 ? y + 256 : y) & ~0xff;
	/*vslope = (dx << 8) / dy;*/
	vxx = (vslope * (vyy - y)) >> 8;

	if(abs(vxx) > abs(hyy)) {
		/* hxx,hyy closer */
		*nx = hxx - 128;
		*ny = hyy - 128;
	} else {
		/* vxx,vyy closer */
		*nx = vxx - 128;
		*ny = vyy - 128;
	}
}

void mob_beam(struct mob *mob, int32_t tx, int32_t ty)
{
	int i, end_cx, end_cy;
	int32_t hslope, vslope, dx, dy, nx, ny;

	mob->beam.x0 = mob->x;
	mob->beam.y0 = mob->y;

	dx = tx - mob->x;
	dy = ty - mob->y;

	hslope = dx ? (dy << 8) / dx : 256;
	vslope = dy ? (dx << 8) / dy : 256;

	ray_step(mob->lvl, mob->x, mob->y, dx, dy, hslope, vslope, &nx, &ny);

	mob->beam.x1 = nx;
	mob->beam.y1 = ny;
	mob->beam.nseg = 1;
}
