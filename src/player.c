#include <stdio.h>
#include "player.h"
#include "util.h"

int mob_move(struct mob *mob, int dx, int dy)
{
	int cx, cy, ncx, ncy;
	int32_t nx, ny;

	if(!(dx | dy)) return 0;

	nx = mob->x + dx + dy;
	ny = mob->y - dx + dy;
	ncx = nx >> 8;
	ncy = ny >> 8;

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
	return 1;
}
