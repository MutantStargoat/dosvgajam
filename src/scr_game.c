#include <stdio.h>
#include "app.h"
#include "vga.h"
#include "tiles.h"
#include "level.h"
#include "rend.h"

static struct level lvl;

#define MAX_DIRTY	16
static struct rect vport;
static struct rect dirty[MAX_DIRTY];
static int ndirty;

static int scrgame_init(void)
{
	if(load_level(&lvl, "data/dbglevel.tmj") == -1) {
		return -1;
	}

	return 0;
}

static void scrgame_destroy(void)
{
	destroy_level(&lvl);
}

static int scrgame_start(void)
{
	int i;

	vga_setpal(0, 0, 0, 0);
	for(i=1; i<tileset.ncolors; i++) {
		vga_setpal(-1, tileset.cmap[i].r, tileset.cmap[i].g, tileset.cmap[i].b);
	}
	vga_setpal(0xff, 0xff, 0xff, 0xff);

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

static void scrgame_display(void)
{
	int i, cx, cy, sx, sy, x, y;
	struct level_cell *cell;

	/* invalidate cells in the dirty rects */
	for(i=0; i<ndirty; i++) {
		vscr_to_cell(dirty[i].x, dirty[i].y, &cx, &cy);
		cell_to_vscr(cx, cy, &sx, &sy);		/* top-left corner of first cell */

		y = sy;
		while(y < dirty[i].y + dirty[i].h) {
			int start_cx = cx;
			int start_cy = cy;
			x = sx;
			while(x < dirty[i].x + dirty[i].w) {
				if((cell = get_level_cell(&lvl, cx, cy))) {
					draw_level_cell(&lvl, cell, 0);
				}
				if((cell = get_level_cell(&lvl, cx, cy - 1))) {
					draw_level_cell(&lvl, cell, 0);
				}

				x += CELL_XSZ;

				/* check if we reach the edge of the map diamond */
				if(++cx >= lvl.size) break;
				if(--cy < 0) break;
			}

			y += CELL_YSZ;
			cx = start_cx + 1;
			cy = start_cy + 1;
		}
	}

	vga_rect_outline(VGA_VMEM, dirty[i].x, dirty[i].y, dirty[i].w, dirty[i].h, 0xff);
	ndirty = 0;

#ifdef VGA_LFB
	vga_pgflip(1);
#endif
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
