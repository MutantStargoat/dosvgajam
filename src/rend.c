#include <stdio.h>
#include "rend.h"
#include "level.h"
#include "tiles.h"

void draw_level_cell(struct level *lvl, struct level_cell *cell, int layer)
{
	static const int offs[][2] = {
		{-TILE_XSZ/2, 0},
		{-TILE_XSZ, TILE_YSZ/2},
		{0, TILE_YSZ/2},
		{-TILE_XSZ/2, TILE_YSZ}
	};
	int i, sx, sy, x, y;
	struct tileimg *tile;

	cell_to_vscr(cell->cx, cell->cy, &sx, &sy);

	for(i=0; i<4; i++) {
		if((tile = get_cell_tile(lvl, cell, i, layer))) {
			x = sx + offs[i][0];
			y = sy + offs[i][1];
			if(x >= 0 && y >= 0) {
				tiles_blit_key(tile, x, y);
			}
		}
	}
}
