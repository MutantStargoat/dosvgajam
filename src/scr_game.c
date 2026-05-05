#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "app.h"
#include "vga.h"
#include "tiles.h"
#include "level.h"
#include "rend.h"
#include "player.h"
#include "options.h"

/* define to consider all cells visible always */
#undef DRAW_FULL

#define XSCROLL_MAX		CELL_XSZ
#define YSCROLL_MAX		240

#define COL_SIZE		(CELL_XSZ >> 1)
#define ROW_SIZE		(CELL_YSZ >> 1)

static int vsync;
static int prev_mx, prev_my;

static struct level lvl;
static int xscroll, yscroll;

struct tileimg *seltile, *cursors[2];
static int mouse_mode;
static long last_fps_upd, nframes;

#define FONT_OFFS	32
static struct tileimg *font[96];
static int text_color = 0xff;

static struct tileimg *hero[8];
static int dir;

static struct mob player;


#ifdef DRAW_FULL
#define MAX_VIS_CELLS	8192
#else
#define MAX_VIS_CELLS	256
#endif
static struct level_cell *viscells[MAX_VIS_CELLS];
static unsigned int num_vis;

static void draw_bitplane(int bpl);
static void scrollto(int32_t x, int32_t y);
static void gprintf(int x, int y, int bpl, const char *fmt, ...);


static int scrgame_init(void)
{
	int i, x, y;

	if(load_level(&lvl, "data/testlvl.tmj") == -1) {
		return -1;
	}

	/* define a cell selection tile */
	seltile = tiles_define(&tileset, 64, 480, CELL_XSZ, CELL_YSZ);
	seltile->xorg = CELL_XSZ / 2;
	seltile->yorg = CELL_YSZ / 2;

	/* define the mouse cursor sprite */
	cursors[0] = tiles_define(&tileset, 128, 480, 10, 16);
	/* another mouse cursor sprite */
	cursors[1] = tiles_define(&tileset, 128, 496, 16, 16);
	cursors[1]->xorg = cursors[1]->yorg = 7;

	x = 256;
	y = 488;
	for(i=0; i<sizeof font / sizeof *font; i++) {
		font[i] = tiles_define(&tileset, x, y, 8, 8);
		x += 8;
		if(x >= tileset.width) {
			x = 256;
			y += 8;
		}
	}

	for(i=0; i<8; i++) {
		hero[i] = tiles_define(&tileset, i * 32, 256, 32, 32);
		hero[i]->xorg = 16;
		hero[i]->yorg = 24;
	}

	return 0;
}

static void scrgame_destroy(void)
{
	destroy_level(&lvl);
}

static int scrgame_start(void)
{
	int i, x, y;

	vga_setpal(0, 0, 0, 0);
	for(i=1; i<tileset.ncolors; i++) {
		vga_setpal(-1, tileset.cmap[i].r, tileset.cmap[i].g, tileset.cmap[i].b);
	}
	vga_setpal(0xff, 0xff, 0xff, 0xff);

	vga_setpitch(VGA_PITCH);

	xscroll = yscroll = 0;
	mouse_mode = 0;
	num_vis = 0;

	player.x = lvl.startx;
	player.y = lvl.starty;
	player.dir = DIR8_S;
	player.lvl = &lvl;
	player.cell = get_level_cell(&lvl, lvl.startx >> 8, lvl.starty >> 8);
	player.hp = 256;

	scrollto(lvl.startx, lvl.starty);

	vsync = opt.vsync;
	nframes = 0;
	last_fps_upd = time_msec;
	return 0;
}

static void scrgame_stop(void)
{
	vga_setpitch(80);
}

#define SCROLL_SPEED	1
static void update(void)
{
	static long prev_upd;
	long dt;
	int i, j, x, y, dx, dy;
	struct level_cell *cell;

	dt = time_msec - prev_upd;
	if(dt < 16) return;

	prev_upd = time_msec;

	dx = dy = 0;
	if(app_keydown(KEY_UP)) {
		dy -= SCROLL_SPEED * dt >> 2;
	}
	if(app_keydown(KEY_DOWN)) {
		dy += SCROLL_SPEED * dt >> 2;
	}
	if(app_keydown(KEY_LEFT)) {
		dx -= SCROLL_SPEED * dt >> 2;
	}
	if(app_keydown(KEY_RIGHT)) {
		dx += SCROLL_SPEED * dt >> 2;
	}

	if(mob_move(&player, dx, dy)) {
		scrollto(player.x, player.y);
	}

	/* compute the list of visible cells */
	num_vis = 0;
	cell = lvl.cells;
	for(i=0; i<lvl.size; i++) {
		for(j=0; j<lvl.size; j++) {
			cell_to_vscr(j, i, &x, &y);

			x -= xscroll;
			y -= yscroll;

#ifndef DRAW_FULL
			if(x >= -CELL_XSZ && x < FB_WIDTH + TILE_XSZ && y >= -CELL_YSZ &&
					y - cell->height < FB_HEIGHT) {
#endif
				viscells[num_vis++] = cell;
				cell->x = x;
				cell->y = y;
#ifndef DRAW_FULL
			}
#endif

			cell++;
		}
	}
}

static char fps_text[32];

static void scrgame_display(void)
{
	int i;
	long fps, elapsed;

	nframes++;
	if((elapsed = time_msec - last_fps_upd) >= 1500) {
		fps = 10000 * nframes / elapsed;
		sprintf(fps_text, "fps: %ld.%ld", fps / 10, fps % 10);
		last_fps_upd = time_msec;
		nframes = 0;
	}

	update();

#ifdef VGA_LFB
	vga_clearfb(0);
#endif

	for(i=0; i<4; i++) {
		vga_planemask(1 << i);
		draw_bitplane(i);
	}

	vga_pgflip(vsync);
}

static void draw_bitplane(int bpl)
{
	int i, j, x, y, mouse_cx, mouse_cy, player_cx, player_cy;
	struct level_cell *cell;

	player_cx = player.cell->cx;
	player_cy = player.cell->cy;

	for(i=0; i<lvl.num_layers; i++) {
		for(j=0; j<num_vis; j++) {
			cell = viscells[j];
			draw_level_cell(&lvl, cell, i, cell->x, cell->y, bpl);

			if(i == 1 && cell->cx == player_cx && cell->cy == player_cy) {
				grid_to_vscr(player.x, player.y, &x, &y);
				tiles_blit_rle(seltile, cell->x, cell->y, bpl);
				tiles_blit_rle(hero[0], x - xscroll, y - yscroll, bpl);
				/*tiles_blit_rle(cursors[1], x - xscroll, y - yscroll, bpl);*/
			}
		}
	}

	/* mouseover highlight */
	/*vscr_to_cell(mouse_x + xscroll, mouse_y + yscroll, &mouse_cx, &mouse_cy);
	if(BOUNDCHK(mouse_cx, lvl.size) && BOUNDCHK(mouse_cy, lvl.size)) {
		cell_to_vscr(mouse_cx, mouse_cy, &x, &y);
		x -= xscroll;
		y -= yscroll;
		tiles_blit_rle(seltile, x, y, bpl);
	}
	*/

	tiles_blit_rle(cursors[mouse_mode], mouse_x, mouse_y, bpl);

	gprintf(0, 0, bpl, fps_text);
	gprintf(0, 8, bpl, "vsync: %s", vsync ? "on" : "off");
	gprintf(120, 0, bpl, "vis: %d", num_vis);
	gprintf(120, 8, bpl, "cell: %d,%d %s", player_cx, player_cy, strcellflags(player.cell->flags));
}

static void scrgame_keyb(int key, int press)
{
	if(!press) return;

	switch(key) {
	case 'd':
		xscroll++;
		break;
	case 'a':
		xscroll--;
		break;
	case 's':
		yscroll++;
		break;
	case 'w':
		yscroll--;
		break;
	case '\t':
		mouse_mode ^= 1;
		break;

	case 'v':
		vsync ^= 1;
		break;

	default:
		break;
	}
}

static void scrgame_mouse(int bn, int press, int x, int y)
{
	int cx, cy;

	prev_mx = x;
	prev_my = y;

	if(!press) return;

	if(bn == 0) {
		vscr_to_cell(mouse_x + xscroll, mouse_y + yscroll, &cx, &cy);
		printf("mouse %d,%d -> cell %d,%d    (scroll: %d,%d)\n", x, y, cx, cy,
				xscroll, yscroll);
	}
}

static void scrgame_motion(int x, int y)
{
	int dx, dy;

	dx = mouse_x - prev_mx;
	dy = mouse_y - prev_my;
	prev_mx = mouse_x;
	prev_my = mouse_y;

	if(mouse_bnstate & 4) {
		xscroll -= dx;
		yscroll -= dy;
	}
}


struct app_screen scr_game = {
	"game",
	scrgame_init, scrgame_destroy,
	scrgame_start, scrgame_stop,
	scrgame_display,
	scrgame_keyb,
	scrgame_mouse, scrgame_motion
};


static void scrollto(int32_t gridx, int32_t gridy)
{
	int sx, sy;
	grid_to_vscr(gridx, gridy, &sx, &sy);
	xscroll = sx - (FB_WIDTH >> 1);
	yscroll = sy - (FB_HEIGHT >> 1);
}


static void gprintf(int x, int y, int bpl, const char *fmt, ...)
{
	static char buf[1024];
	va_list ap;
	char *s = buf;
	int c;

	va_start(ap, fmt);
	vsprintf(buf, fmt, ap);
	va_end(ap);

	while((c = *s++)) {
		if(c >= FONT_OFFS && c < 128) {
			tiles_fill_rle(font[c - 32], x, y, text_color, bpl);
		}
		x += 8;
	}
}
