#include <stdio.h>
#include <stdlib.h>
#include "player.h"
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
		mob->state = MOB_IDLE;
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
		if(++mob->spr.frm >= mob->spr.nfrm * 2) {		// XXX remove the *2
			mob->spr.frm = 0;
		}
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
	mob->state = st;
	mob->spr.cur = mob->spr.anim + st;
	mob->spr.frm = 0;
	mob->spr.nfrm = mob->spr.cur->seq[mob->dir]->ntiles;
}
