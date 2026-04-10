#include "rend.h"
#include "level.h"
#include "tiles.h"

void draw_level_cell(struct level *lvl, struct level_cell *cell, int layer)
{
	static const int offs[][2] = {
		{TILE_XSZ >> 1, 0},
		{0, TILE_YSZ >> 1},
		{TILE_XSZ, TILE_YSZ >> 1},
		{TILE_XSZ >> 1, TILE_YSZ}
	};
	int i, sx, sy;
	struct tileimg *tile;

	cell_to_vscr(cell->cx, cell->cy, &sx, &sy);

	for(i=0; i<4; i++) {
		if((tile = get_cell_tile(lvl, cell, i, layer))) {
			tiles_blit_key(tile, sx + offs[i][0], sy + offs[i][1]);
		}
	}
}
