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

#ifndef NO_SOUND
#include "audio.h"

static struct au_music *mus;
#endif

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

#define NUM_WALK_FRAMES	8

static struct mob player;

struct tileimg *mob_idle[8], *mob_fire[8];


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
	int i, j, x, y, dir;
	static int spr_row_dir[] = {
		DIR8_W, DIR8_E, DIR8_S, DIR8_N, DIR8_NW, DIR8_NE, DIR8_SW, DIR8_SE
	};

	if(load_level(&lvl, "data/levels/testlvl.tmj") == -1) {
		return -1;
	}

	/* define a cell selection tile */
	seltile = tiles_define(&tileset, 384, 160, CELL_XSZ, CELL_YSZ);
	seltile->xorg = CELL_XSZ / 2;
	seltile->yorg = CELL_YSZ / 2;

	/* define the mouse cursor sprite */
	cursors[0] = tiles_define(&tileset, 448, 160, 10, 16);
	/* another mouse cursor sprite */
	cursors[1] = tiles_define(&tileset, 448, 176, 16, 16);
	cursors[1]->xorg = cursors[1]->yorg = 7;

	x = 256;
	y = 200;
	for(i=0; i<sizeof font / sizeof *font; i++) {
		font[i] = tiles_define(&tileset, x, y, 8, 8);
		x += 8;
		if(x >= tileset.width) {
			x = 256;
			y += 8;
		}
	}

	define_spranim(&tileset, player.spr.anim + MOB_WALK, 8, 32, 256, 32, 32);
	spr_origin(&player.spr, 16, 28);

	define_spranim(&tileset, player.spr.anim + MOB_IDLE, 1, 0, 256, 32, 32);

	for(i=0; i<8; i++) {
		y = 256 + i * 32;
		dir = spr_row_dir[i];

		mob_idle[dir] = tiles_define(&tileset, 448, y, 32, 32);
		mob_idle[dir]->xorg = 16;
		mob_idle[dir]->yorg = 28;

		mob_fire[dir] = tiles_define(&tileset, 448+32, y, 32, 32);
		mob_fire[dir]->xorg = 16;
		mob_fire[dir]->yorg = 28;
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

#ifndef NO_SOUND
	if(!(mus = au_load_music("data/music/test.mus"))) {
		fprintf(stderr, "failed to load music\n");
		return -1;
	}
#endif

	vga_setpal(0, 0, 0, 0);
	for(i=1; i<tileset.ncolors; i++) {
		vga_setpal(-1, tileset.cmap[i].r, tileset.cmap[i].g, tileset.cmap[i].b);
	}
	vga_setpal(0xff, 0xff, 0xff, 0xff);

	vga_setpitch(VGA_PITCH);

	xscroll = yscroll = 0;
	mouse_mode = 1;
	num_vis = 0;

	player.x = lvl.startx;
	player.y = lvl.starty;
	player.dir = DIR8_S;
	player.lvl = &lvl;
	player.cell = get_level_cell(&lvl, lvl.startx >> 8, lvl.starty >> 8);
	player.hp = 256;

	scrollto(lvl.startx, lvl.starty);

#ifndef NO_SOUND
	if(mus) {
		au_play_music(mus);
	}
#endif

	vsync = opt.vsync;
	nframes = 0;
	last_fps_upd = time_msec;
	return 0;
}

static void scrgame_stop(void)
{
#ifndef NO_SOUND
	if(mus) {
		au_stop_music(mus);
		au_free_music(mus);
	}
#endif
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
	if(app_keydown(KEY_UP) || app_keydown('w')) {
		dy -= SCROLL_SPEED * dt >> 2;
	}
	if(app_keydown(KEY_DOWN) || app_keydown('s')) {
		dy += SCROLL_SPEED * dt >> 2;
	}
	if(app_keydown(KEY_LEFT) || app_keydown('a')) {
		dx -= SCROLL_SPEED * dt >> 2;
	}
	if(app_keydown(KEY_RIGHT) || app_keydown('d')) {
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
		sprintf(fps_text, "fps:%ld.%ld", fps / 10, fps % 10);
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
	int i, j, x, y, mouse_cx, mouse_cy, mouse_gx, mouse_gy, player_cx, player_cy;
	struct level_cell *cell;
	struct tileimg *tile;
	struct tileseq *seq;

	player_cx = player.cell->cx;
	player_cy = player.cell->cy;

	for(i=0; i<lvl.num_layers; i++) {
		for(j=0; j<num_vis; j++) {
			cell = viscells[j];

			/* TODO dither wall layer if tile bounds overlap player sprite */
			draw_level_cell(&lvl, cell, i, cell->x, cell->y, bpl);

			/* draw mobs */
			if(i == 1) {
				struct mob *mob = cell->mobs;
				while(mob) {
					grid_to_vscr(mob->x, mob->y, &x, &y);
					switch(mob->state) {
					case MOB_IDLE:
						tile = mob_idle[mob->dir];
						break;
					case MOB_FIRE:
						tile = mob_fire[mob->dir];
						break;
					default:
						tile = 0;
					}

					if(tile) tiles_blit_rle(tile, x - xscroll, y - yscroll, bpl);
					mob = mob->next;
				}

				if(cell->cx == player_cx && cell->cy == player_cy) {
					grid_to_vscr(player.x, player.y, &x, &y);
					tiles_blit_rle(seltile, cell->x, cell->y, bpl);

					seq = player.spr.anim[MOB_WALK].seq[player.dir];
					tile = seq->tile[player.spr.frm >> 1];		/* XXX remove the >> 1 */

					tiles_blit_rle(tile, x - xscroll, y - yscroll, bpl);
					/*tiles_blit_rle(cursors[1], x - xscroll, y - yscroll, bpl);*/
				}
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
	vscr_to_grid(mouse_x + xscroll, mouse_y + yscroll, &mouse_gx, &mouse_gy);
	mouse_gx -= 128;
	mouse_gy -= 128;

	tiles_blit_rle(cursors[mouse_mode], mouse_x, mouse_y, bpl);

	gprintf(0, 0, bpl, fps_text);
	/*gprintf(0, 8, bpl, "vsync: %s", vsync ? "on" : "off");*/
	gprintf(90, 0, bpl, "vis:%d", num_vis);
	gprintf(160, 0, bpl, "cell:%d,%d %s", player_cx, player_cy, strcellflags(player.cell->flags));
	/*gprintf(0, 8, bpl, "player: %s,%s\n", fixpstr(player.x, 8), fixpstr(player.y, 8));
	gprintf(180, 8, bpl, "mouse: %s,%s\n", fixpstr(mouse_gx, 8), fixpstr(mouse_gy, 8));*/
}

static void scrgame_keyb(int key, int press)
{
	if(!press) return;

	switch(key) {
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
	int cx, cy, gx, gy;

	prev_mx = x;
	prev_my = y;

	if(!press) return;

	if(bn == 0) {
		vscr_to_cell(mouse_x + xscroll, mouse_y + yscroll, &cx, &cy);

		vscr_to_grid(mouse_x + xscroll, mouse_y + yscroll, &gx, &gy);
		gx -= 128;
		gy -= 128;
		mob_lookat(&player, gx, gy);
	}
}

static void scrgame_motion(int x, int y)
{
	int dx, dy, gx, gy;

	dx = mouse_x - prev_mx;
	dy = mouse_y - prev_my;
	prev_mx = mouse_x;
	prev_my = mouse_y;

	if(mouse_bnstate & 1) {
		vscr_to_grid(x + xscroll, y + yscroll, &gx, &gy);
		gx -= 128;
		gy -= 128;
		mob_lookat(&player, gx, gy);
	}

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
