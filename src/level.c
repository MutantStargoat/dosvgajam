#include "level.h"
#include "tiles.h"

int create_level(struct level *lvl, int sz);
void destroy_level(struct level *lvl);

/* get hte cell cx,cy */
struct level_cell *get_level_cell(struct level *lvl, int cx, int cy);
/* get the cell at virtual screen coords sx, sy */
struct level_cell *get_level_cell_vscr(struct level *lvl, int sx, int sy);

struct tileimg *get_cell_tile(struct level *lvl, struct level_cell *cell, int n, int layer)
{
	int tx, ty;
	unsigned int *mapptr, idx;

	tx = cell->cx << 1;
	ty = cell->cy << 1;
	mapptr = lvl->tmap.layer[layer].tiles + (ty << lvl->tmap.shift) + tx;

	switch(n) {
	case 0:
		idx = *mapptr;
		break;
	case 1:
		idx = mapptr[lvl->tmap.sz];
		break;
	case 2:
		idx = mapptr[1];
		break;
	case 3:
		idx = mapptr[lvl->tmap.sz + 1];
		break;
	default:
		return 0;
	}

	return idx ? lvl->tsheet->tiles + idx : 0
}
