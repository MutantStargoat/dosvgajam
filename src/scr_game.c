#include <stdio.h>
#include <stdlib.h>
#include "app.h"
#include "vga.h"
#include "tiles.h"
#include "level.h"
#include "rend.h"
#include "keyb.h"

#define XSCROLL_MAX		CELL_XSZ
#define YSCROLL_MAX		240

#define COL_SIZE		(CELL_XSZ >> 1)
#define ROW_SIZE		(CELL_YSZ >> 1)

static struct level lvl;
static int xscroll, yscroll;

struct tileimg *seltile, *cursor;


static int scrgame_init(void)
{
	if(load_level(&lvl, "data/dbglevel.tmj") == -1) {
		return -1;
	}

	xscroll = yscroll = 0;

	/* define a cell selection tile */
	seltile = tiles_define(&tileset, 64, 480, CELL_XSZ, CELL_YSZ);
	/* define the mouse cursor sprite */
	cursor = tiles_define(&tileset, 128, 480, 16, 24);

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

	vga_setpitch(VGA_PITCH);

	return 0;
}

static void scrgame_stop(void)
{
	vga_setpitch(80);
}

#define SCROLL_SPEED	1024
static void update(void)
{
	static long prev_upd;
	long dt;

	dt = time_msec - prev_upd;
	if(dt < 16) return;

	prev_upd = time_msec;

	if(kb_isdown(KEY_UP)) {
		yscroll -= SCROLL_SPEED * dt;
	}
	if(kb_isdown(KEY_DOWN)) {
		yscroll += SCROLL_SPEED * dt;
	}
	if(kb_isdown(KEY_LEFT)) {
		xscroll -= SCROLL_SPEED * dt;
	}
	if(kb_isdown(KEY_RIGHT)) {
		xscroll += SCROLL_SPEED * dt;
	}
}

static void scrgame_display(void)
{
	int cx, cx0, cy, cy0, sx, sy, x, row;
	struct level_cell *cell;

	update();

	vscr_to_cell(xscroll, yscroll, &cx, &cy);
	cell_to_vscr(cx, cy, &sx, &sy);		/* top corner of first cell */
	sx -= CELL_XSZ / 2;					/* top-left bound */

	row = 0;
	while(sy < yscroll + FB_HEIGHT) {
		x = (row & 1) ? sx - CELL_XSZ / 2 : sx;
		cx0 = cx;
		cy0 = cy;
		while(x < xscroll + FB_WIDTH) {
			if((cell = get_level_cell(&lvl, cx, cy))) {
				draw_level_cell(&lvl, cell, 0, x, sy);
			}

			x += CELL_XSZ;

			/* check if we reach the edge of the map diamond */
			if(++cx >= lvl.size) break;
			if(--cy < 0) break;
		}

		sy += CELL_YSZ / 2;

		if(row++ & 1) {
			cx = cx0 + 1;
			cy = cy0;
		} else {
			cx = cx0;
			cy = cy0 + 1;
		}
	}

	/* mouseover highlight */
	/*
	vscr_to_cell(mouse_x, mouse_y, &cx, &cy);
	printf("mouse %d,%d -> cell %d,%d\n", mouse_x, mouse_y, cx, cy);
	if(BOUNDCHK(cx, lvl.size) && BOUNDCHK(cy, lvl.size)) {
		cell_to_vscr(cx, cy, &sx, &sy);
		sx -= CELL_XSZ / 2;
		tiles_blit_key(seltile, sx, sy);
		dirty_rect(sx, sy, CELL_XSZ, CELL_YSZ);
	}
	*/

	tiles_blit_key(cursor, mouse_x, mouse_y);

	vga_pgflip(1);
}

static void scrgame_keyb(int key, int press)
{
	if(!press) return;

	switch(key) {
	default:
		break;
	}
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
