#include <stdio.h>
#include <stdlib.h>
#include "app.h"
#include "vga.h"
#include "tiles.h"
#include "level.h"
#include "rend.h"

#define XSCROLL_MAX		CELL_XSZ
#define YSCROLL_MAX		240

#define COL_SIZE		(CELL_XSZ >> 1)
#define ROW_SIZE		(CELL_YSZ >> 1)

static struct level lvl;
static int xscroll, yscroll;

struct tileimg *seltile, *cursor;

static void test(void)
{
	int pos[][2] = {{0, 0}, {32, 16}, {64, 64}, {90, 44}, {-22, -12}, {24, -8}};
	int exp1[][2] = {{0, 0}, {0, 1}, {1, 4}, {1, 3}, {-1, -1}, {0, -1}};
	int cell[][2] = {{0, 0}, {1, 0}, {1, 1}, {0, 1}, {2, 3}};
	int exp2[][2] = {{0, 0}, {64, 0}, {96, 16}, {32, 16}, {160, 48}};
	int i, x, y, row, col;

	printf("TEST vscr_to_cell\n");
	for(i=0; i<sizeof pos / sizeof *pos; i++) {
		vscr_to_cell(pos[i][0], pos[i][1], &col, &row);
		printf("%3d,%3d -> %3d,%3d  (%d,%d expected)\n", pos[i][0], pos[i][1],
				col, row, exp1[i][0], exp1[i][1]);
	}

	printf("TEST cell_to_vscr\n");
	for(i=0; i<sizeof cell / sizeof *cell; i++) {
		cell_to_vscr(cell[i][0], cell[i][1], &x, &y);
		printf("%3d,%3d -> %3d,%3d  (%d,%d expected)\n", cell[i][0], cell[i][1],
				x, y, exp2[i][0], exp2[i][1]);
	}
}

static int scrgame_init(void)
{
	if(load_level(&lvl, "data/dbglevel.tmj") == -1) {
		return -1;
	}

	xscroll = yscroll = 0;

	/* define a cell selection tile */
	seltile = tiles_define(&tileset, 64, 480, CELL_XSZ, CELL_YSZ);
	/* define the mouse cursor sprite */
	cursor = tiles_define(&tileset, 128, 480, 10, 16);

	test();
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
	static int xacc, yacc;
	long dt;

	dt = time_msec - prev_upd;
	if(dt < 16) return;

	prev_upd = time_msec;

	if(app_keydown(KEY_UP)) {
		yacc -= SCROLL_SPEED * dt;
	}
	if(app_keydown(KEY_DOWN)) {
		yacc += SCROLL_SPEED * dt;
	}
	if(app_keydown(KEY_LEFT)) {
		xacc -= SCROLL_SPEED * dt;
	}
	if(app_keydown(KEY_RIGHT)) {
		xacc += SCROLL_SPEED * dt;
	}

	xscroll += xacc >> 16;
	yscroll += yacc >> 16;
	xacc -= xacc & (int)0xffff0000;
	yacc -= yacc & (int)0xffff0000;
}

#define XCELLS		((320 + TILE_XSZ + CELL_XSZ - 1) / CELL_XSZ - 1)
#define YCELLS		(2 * (240 + TILE_YSZ + CELL_YSZ - 1) / CELL_YSZ - 1)

static void scrgame_display(void)
{
	int i, j, x, y, cx0, cy0, cx, cy, xoffs, yoffs;
	struct level_cell *cell;

	update();

	vga_clearfb(0);

	/* find cell at the top-left corner of the screen */
	vscr_to_cell(-xscroll, -yscroll, &cx0, &cy0);

	xoffs = xscroll & (CELL_XSZ - 1);
	yoffs = yscroll & (CELL_YSZ - 1);

	cy = cy0;
	y = yoffs;
	for(i=0; i<YCELLS; i++) {
		cx = cx0;
		x = cy & 1 ? xoffs + CELL_XSZ / 2 : xoffs;
		for(j=0; j<XCELLS; j++) {
			if((cell = get_level_cell(&lvl, cx0 + j, cy0 + i))) {
				draw_level_cell(&lvl, cell, 0, x, y);
			}
			x += CELL_XSZ;
			cx++;
		}
		y += CELL_YSZ / 2;
		cy++;
	}

	/* mouseover highlight */
	vscr_to_cell(mouse_x - xscroll, mouse_y - yscroll, &cx, &cy);
	if(BOUNDCHK(cx, lvl.size) && BOUNDCHK(cy, lvl.size)) {
		cell_to_vscr(cx, cy, &x, &y);
		x += xscroll;
		y += yscroll;
		tiles_blit_key(seltile, x - CELL_XSZ / 2, y - CELL_YSZ / 2);
	}

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
	int cx, cy;

	if(!press) return;

	if(bn == 0) {
		vscr_to_cell(mouse_x - xscroll, mouse_y - yscroll, &cx, &cy);
		printf("mouse %d,%d -> cell %d,%d    (scroll: %d,%d)\n", x, y, cx, cy,
				xscroll, yscroll);
	}
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
