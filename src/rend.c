#include <stdio.h>
#include "rend.h"
#include "level.h"
#include "tiles.h"

void draw_level_cell(struct level *lvl, struct level_cell *cell, int layer, int destx, int desty, int bpl)
{
	static const int offs[][2] = {
		{-TILE_XSZ / 2, -TILE_YSZ},
		{-TILE_XSZ, -TILE_YSZ / 2},
		{0, -TILE_YSZ / 2},
		{-TILE_XSZ / 2, 0}
	};
	int i, x, y;
	struct tileimg *tile;

	for(i=0; i<4; i++) {
		if((tile = get_cell_tile(lvl, cell, i, layer))) {
			x = destx + offs[i][0];
			y = desty + offs[i][1];
			tiles_blit_rle(tile, x, y, bpl);
		}
	}
}
