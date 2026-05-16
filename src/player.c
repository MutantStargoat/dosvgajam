#include <stdio.h>
#include "player.h"
#include "level.h"
#include "util.h"

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
	mob->state = MOB_WALK;
	mob->anmfrm++;
	return 1;
}

void mob_lookat(struct mob *mob, int32_t x, int32_t y)
{
	int dx = x - mob->x;
	int dy = y - mob->y;
	mob->dir = gridvec_to_dir8(dx, dy);
}
