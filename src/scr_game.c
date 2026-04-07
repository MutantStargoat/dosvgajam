#include <stdio.h>
#include "app.h"
#include "vga.h"
#include "tiles.h"
#include "level.h"

static struct tilesheet tiles;

#define MAX_DIRTY	16
static struct rect vport;
static struct rect dirty[MAX_DIRTY];
static int ndirty;

static int scrgame_init(void)
{
	if(tiles_load(&tiles, "data/tiles.png") == -1) {
		fprintf(stderr, "failed to load tiles\n");
		return -1;
	}
	tiles_define(&tiles, 0, 192, TILE_XSZ, TILE_YSZ);
	tiles_define(&tiles, 32, 192, TILE_XSZ, TILE_YSZ);
	tiles_define(&tiles, 64, 192, TILE_XSZ, TILE_YSZ);
	return 0;
}

static void scrgame_destroy(void)
{
	tiles_destroy(&tiles);
}

static int scrgame_start(void)
{
	int i;

	vga_setpal(0, 0, 0, 0);
	for(i=1; i<tiles.ncolors; i++) {
		vga_setpal(-1, tiles.cmap[i].r, tiles.cmap[i].g, tiles.cmap[i].b);
	}

	vport.x = vport.y = 50;
	vport.w = 200;
	vport.h = 150;

	dirty[0] = vport;
	ndirty = 1;

	return 0;
}

static void scrgame_stop(void)
{
}

struct level lvl;

static void scrgame_display(void)
{
	int i, cx, cy, sx, sy;
	struct level_cell *cell;

	/* invalidate cells in the dirty rects */
	for(i=0; i<ndirty; i++) {
		vscr_to_cell(dirty[i].x, dirty[i].y, &cx, &cy);
		if((cell = get_level_cell(&lvl, cx, cy))) {
			draw_level_cell(&lvl, cell, 0);
		}
	}
	ndirty = 0;

	tiles_blit_key(tiles.tiles, 20, 20);
	tiles_blit_key(tiles.tiles + 1, 36, 28);
	tiles_blit_key(tiles.tiles + 2, 20, 36);
	ndirty = 0;
}

static void scrgame_keyb(int key, int press)
{
}

static void scrgame_mouse(int bn, int press, int x, int y)
{
}

static void scrgame_motion(int x, int y)
{
}


struct app_screen scr_game = {
	"game",
	scrgame_init, scrgame_destroy,
	scrgame_start, scrgame_stop,
	scrgame_display,
	scrgame_keyb,
	scrgame_mouse, scrgame_motion
};
