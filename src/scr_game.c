#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "app.h"
#include "vga.h"
#include "tiles.h"
#include "level.h"
#include "rend.h"
#include "mob.h"
#include "options.h"
#include "dynarr.h"
#include "psys.h"

#define BEAM_DMG	32
#define BEAM_COL	253
#define RED_COL		(BEAM_COL - 1)
#define BEAM_HEIGHT	16
#define BEAM_DUR	100
#define GUN_DUR		600

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

struct mob player;
int xscroll, yscroll;

static int vsync;
static int prev_mx, prev_my;

static struct level lvl;

struct tileimg *seltile, *cursors[2], *balltile;
static int mouse_mode;
static long last_fps_upd, nframes;

#define FONT_OFFS	32
static struct tileimg *font[96];
static int text_color = 0xff;

static int dbg_hide_walls;

static struct au_sample *sfx_laser;

static int game_state, mobs_rem;


#ifdef DRAW_FULL
#define MAX_VIS_CELLS	8192
#else
#define MAX_VIS_CELLS	256
#endif
static struct level_cell *viscells[MAX_VIS_CELLS];
static unsigned int num_vis;

static void draw_bitplane(int bpl);
static void scrollto(int32_t x, int32_t y);
static void gprintf(int x, int y, const char *fmt, ...);


static int scrgame_init(void)
{
	int i, x, y;
	struct sprite sprmob;

	if(load_level(&lvl, "data/levels/testlvl.tmj") == -1) {
		return -1;
	}

	tile_inval = tiles_define(&tileset, 464, 176, 16, 16);
	tile_inval->xorg = tile_inval->yorg = 8;

	/* define a cell selection tile */
	seltile = tiles_define(&tileset, 384, 160, CELL_XSZ, CELL_YSZ);
	seltile->xorg = CELL_XSZ / 2;
	seltile->yorg = CELL_YSZ / 2;

	/* define the mouse cursor sprite */
	cursors[0] = tiles_define(&tileset, 448, 160, 10, 16);
	/* another mouse cursor sprite */
	cursors[1] = tiles_define(&tileset, 448, 176, 16, 16);
	cursors[1]->xorg = cursors[1]->yorg = 7;

	balltile = tiles_define(&tileset, 448 + 16, 160, 16, 16);
	balltile->xorg = balltile->yorg = 8;

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

	init_mob(&player);
	player.rad = 64;
	define_spranim(&tileset, player.spr.anim + MOB_IDLE, 1, 0, 256, 32, 32);
	define_spranim(&tileset, player.spr.anim + MOB_WALK, 8, 32, 256, 32, 32);
	define_spranim(&tileset, player.spr.anim + MOB_FIRE, 1, 288, 256, 32, 32);
	define_spranim(&tileset, player.spr.anim + MOB_DEAD, 1, 320, 256, 32, 32);
	spr_origin(&player.spr, 16, 28);

	memset(&sprmob, 0, sizeof sprmob);
	define_spranim(&tileset, sprmob.anim + MOB_IDLE, 1, 448, 256, 32, 32);
	define_spranim(&tileset, sprmob.anim + MOB_FIRE, 1, 448 + 32, 256, 32, 32);
	define_spranim_onedir(&tileset, sprmob.anim + MOB_DEAD, 4, 384, 512, 32, 32);
	spr_origin(&sprmob, 16, 28);

	for(i=0; i<dynarr_size(lvl.mobs); i++) {
		lvl.mobs[i]->spr = sprmob;
	}

#ifndef NO_SOUND
	if(!(sfx_laser = au_load_sample("data/sfx/laser4.wav"))) {
		return -1;
	}
#endif
	return 0;
}

static void scrgame_destroy(void)
{
	destroy_level(&lvl);
#ifndef NO_SOUND
	au_stop_sample(sfx_laser);
	au_free_sample(sfx_laser);
#endif
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
	vga_setpal(RED_COL, 255, 0, 0);

	vga_setpal(BEAM_COL, 92, 92, 160);
	vga_setpal(BEAM_COL+1, 192, 192, 255);

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
	mob_state(&player, MOB_IDLE);

	scrollto(lvl.startx, lvl.starty);

	/* go through all the mobs, and call mob_state to initialize their current
	 * animation frames */
	for(i=0; i<dynarr_size(lvl.mobs); i++) {
		mob_state(lvl.mobs[i], MOB_IDLE);
	}

	game_state = 0;
	mobs_rem = dynarr_size(lvl.mobs);

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
	int i, j, x, y, dx, dy, cx, cy;
	int32_t gx, gy, dirx, diry;
	struct level_cell *cell;
	struct mob *mob;

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

	player.hp -= player.dmg;
	player.dmg = 0;
	if(player.hp <= 0) {
		player.hp = 0;
		mob_state(&player, MOB_DEAD);
	}

	if(player.state == MOB_DEAD) {
		game_state = -1;
		goto skip_player;
	}

	if(player.state == MOB_FIRE) {
		if(player.state_t == 0) {
			vscr_to_grid(mouse_x + xscroll, mouse_y + yscroll + BEAM_HEIGHT, &gx, &gy);
			gx -= 128;
			gy -= 128;
			mob_lookat(&player, gx, gy);
			mob_beam(&player, gx, gy, BEAM_DMG);

#ifndef NO_SOUND
			au_play_sample(sfx_laser);
#endif
			/* poor man's normalize */
			if(player.beam.nseg > 0) {
				gx = player.beam.x1;
				gy = player.beam.y1;
				dirx = player.x - gx;
				diry = player.y - gy;

				if(dirx > 0) {
					dirx = 0x800;
				} else if(dirx < 0) {
					dirx = -0x800;
				}
				if(diry > 0) {
					diry = 0x800;
				} else if(diry < 0) {
					diry = -0x800;
				}

				grid_to_cell(gx, gy, &cx, &cy);
				if((cell = get_level_cell(&lvl, cx, cy))) {
					cell_spawn_psys(cell, gx, gy, dirx, diry, &psys_blasthit);
				}
			}

		} else if(player.state_t >= BEAM_DUR) {

			/* fire duration ended, change state and remove beam */
			mob_state(&player, MOB_IDLE);
			mob_beamstop(&player);
		}

	} else {
		if(mob_move(&player, dx, dy)) {
			scrollto(player.x, player.y);
		}
	}
skip_player:

	player.state_t += dt;

	/* apply damage to mobs */
	for(i=0; i<dynarr_size(lvl.mobs); i++) {
		mob = lvl.mobs[i];
		if(mob->state == MOB_DEAD) continue;

		mob->hp -= mob->dmg;
		mob->dmg = 0;
		if(mob->hp <= 0) {
			mob->hp = 0;
			mob_state(mob, MOB_DEAD);

			if(--mobs_rem <= 0) {
				game_state = 1;		/* victory */
			}
		}
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

				/* update mobs in this cell */
				mob = cell->mobs;
				while(mob) {
					if(mob->update) {
						mob->update(mob, dt);
					}

					if(mob->state == MOB_FIRE) {
						if(mob->state_t == 0) {
							/* TODO play sound */
						} else if(mob->state_t >= GUN_DUR) {
							mob_state(mob, MOB_IDLE);
						}
					}
					mob = mob->next;
				}

				/* update particles in this cell */
				psys_upd_emitters(&cell->psys, dt);

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

	vga_clearfb(0);

	for(i=0; i<4; i++) {
		vga_planemask(1 << i);
		draw_bitplane(i);
	}

	vga_pgflip(vsync);
}

static void draw_bitplane(int bpl)
{
	int i, j, x, y, x1, y1, player_cx, player_cy;
	int32_t mouse_gx, mouse_gy;
	struct level_cell *cell;
	struct beamseg *bseg;
	struct mob *mob;
	struct psys *ps;

	cur_bpl = bpl;

	player_cx = player.cell->cx;
	player_cy = player.cell->cy;

	for(i=0; i<lvl.num_layers; i++) {
		for(j=0; j<num_vis; j++) {
			cell = viscells[j];

			if((i & 1) == 0 || !dbg_hide_walls) {
				/* TODO dither wall layer if tile bounds overlap player sprite */
				draw_level_cell(&lvl, cell, i, cell->x, cell->y);
			}

			if(i == 1) {
				bseg = cell->beamsegs;
				while(bseg) {
					grid_to_vscr(bseg->x0, bseg->y0, &x, &y);
					grid_to_vscr(bseg->x1, bseg->y1, &x1, &y1);
					x -= xscroll;
					y -= yscroll + BEAM_HEIGHT;
					x1 -= xscroll;
					y1 -= yscroll + BEAM_HEIGHT;
					if(clip_line(&x, &y, &x1, &y1, 1, 1, FB_WIDTH - 2, FB_HEIGHT - 2)) {
						draw_line(x, y, x1, y1, BEAM_COL + 1);

						if(abs(x1 - x) > abs(y1 - y)) {
							draw_line(x, y - 1, x1, y1 - 1, BEAM_COL);
							draw_line(x, y + 1, x1, y1 + 1, BEAM_COL);
						} else {
							draw_line(x - 1, y, x1 - 1, y1, BEAM_COL);
							draw_line(x + 1, y, x1 + 1, y1, BEAM_COL);
						}
					}

					bseg = bseg->next;
				}

				/* draw mobs */
				mob = cell->mobs;
				while(mob) {
					grid_to_vscr(mob->x, mob->y, &x, &y);
					spr_draw(&mob->spr, x - xscroll, y - yscroll, mob->dir);
					mob = mob->next;
				}

				if(cell->cx == player_cx && cell->cy == player_cy) {
					grid_to_vscr(player.x, player.y, &x, &y);
					if(showdbg) {
						tiles_blit_rle(seltile, cell->x, cell->y, bpl);
					}

					spr_draw(&player.spr, x - xscroll, y - yscroll, player.dir);
				}

				/* draw particles */
				ps = cell->psys;
				while(ps) {
					psys_draw(ps);
					ps = ps->next;
				}
			}
		}
	}

	/* draw UI */
#ifdef VGA_LFB
	if(cur_bpl == 0)
#endif
	{
		vga_rect_outline(vga_backbuf, 23, 1, 68, 16, 255);
		vga_fillrect(vga_backbuf, 25, 3, player.hp >> 2, 12, RED_COL);
		vga_planemask(1 << bpl);
	}
	gprintf(5, 5, "HP %3d/256", player.hp);

	vscr_to_grid(mouse_x + xscroll, mouse_y + yscroll, &mouse_gx, &mouse_gy);
	mouse_gx -= 128;
	mouse_gy -= 128;

	tiles_blit_rle(cursors[mouse_mode], mouse_x, mouse_y, bpl);

	if(showdbg) {
		gprintf(160, 0, fps_text);
		gprintf(260, 0, "vis:%d", num_vis);
		gprintf(160, 10, "cell:%d,%d %s", player_cx, player_cy, strcellflags(player.cell->flags));
	}

	if(game_state) {
		text_color = 0;
		gprintf(120, 160, game_state > 0 ? " Victory! " : "Game Over!");
		text_color = game_state > 0 ? 0xff : RED_COL;
		gprintf(119, 159, game_state > 0 ? " Victory! " : "Game Over!");
		text_color = 0xff;
		gprintf(98, 180, "Hit ESC to exit");
	}
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

	case KEY_F2:
		dbg_hide_walls ^= 1;
		break;

	default:
		break;
	}
}

static void scrgame_mouse(int bn, int press, int x, int y)
{
	prev_mx = x;
	prev_my = y;

	if(bn == 0 && press) {
		mob_state(&player, MOB_FIRE);
	}
}

static void scrgame_motion(int x, int y)
{
	int dx, dy;
	int32_t gx, gy;

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


static void gprintf(int x, int y, const char *fmt, ...)
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
			tiles_fill_rle(font[c - 32], x, y, text_color, cur_bpl);
		}
		x += 8;
	}
}
