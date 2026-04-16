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

/* fractional row/col scroll accumulators. advance when they go over |COL_SIZE|
 * or |ROW_SIZE| to paint more tiles outside the edges
 */
static int scroll_dx, scroll_dy;

static struct rect vport, vpcells;
static struct rect *dirty;
static int num_dirty, max_dirty;

static void scroll(int dx, int dy);
static void dirty_rect(int x, int y, int w, int h);
static INLINE void clear_dirty(void);


static int scrgame_init(void)
{
	if(load_level(&lvl, "data/dbglevel.tmj") == -1) {
		return -1;
	}

	max_dirty = 16;
	num_dirty = 0;
	dirty = malloc_nf(16 * sizeof *dirty);

	return 0;
}

static void scrgame_destroy(void)
{
	destroy_level(&lvl);
	free(dirty);
}

static int scrgame_start(void)
{
	int i;

	vga_setpal(0, 0, 0, 0);
	for(i=1; i<tileset.ncolors; i++) {
		vga_setpal(-1, tileset.cmap[i].r, tileset.cmap[i].g, tileset.cmap[i].b);
	}
	vga_setpal(0xff, 0xff, 0xff, 0xff);

	vga_setpitch((320 + CELL_XSZ) / 4);
	vga_scroll(CELL_XSZ / 2, CELL_YSZ / 2);

	scroll_dx = scroll_dy = 0;

	vport.x = vport.y = 0;
	vport.w = 320;
	vport.h = 240;
	dirty_rect(vport.x, vport.y, vport.w, vport.h);

	vpcells.x = vpcells.y = 0;
	vpcells.w = (320 + CELL_XSZ - 1) / CELL_XSZ;
	vpcells.h = (240 + CELL_YSZ - 1) / CELL_YSZ;

	return 0;
}

static void scrgame_stop(void)
{
	vga_setpitch(320 / 4);
}

#define SCROLL_SPEED	1024
static void update(void)
{
	static long prev_upd;
	static int32_t scrx, scry;
	long dt;

	dt = time_msec - prev_upd;
	if(dt < 16) return;

	prev_upd = time_msec;

	if(kb_isdown(KEY_UP)) {
		scry -= SCROLL_SPEED * dt;
	}
	if(kb_isdown(KEY_DOWN)) {
		scry += SCROLL_SPEED * dt;
	}
	if(kb_isdown(KEY_LEFT)) {
		scrx -= SCROLL_SPEED * dt;
	}
	if(kb_isdown(KEY_RIGHT)) {
		scrx += SCROLL_SPEED * dt;
	}

	if(abs(scrx) > 65535 || abs(scry) > 65535) {
		int xoffs = scrx >> 16;
		int yoffs = scry >> 16;
		int sx = vga_xscroll + xoffs;
		int sy = vga_yscroll + yoffs;

		if(sx < 0) sx = 0;
		if(sx >= XSCROLL_MAX) sx = XSCROLL_MAX - 1;
		if(sy < 0) sy = 0;
		if(sy >= YSCROLL_MAX) sy = YSCROLL_MAX - 1;
		vga_scroll(sx, sy);

		scrx = scry = 0;
	}
}

static void scrgame_display(void)
{
	int i, cx, cx0, cy, cy0, sx, sy, x, row;
	struct level_cell *cell;

	update();

	/* invalidate cells in the dirty rects */
	for(i=0; i<num_dirty; i++) {
		vscr_to_cell(dirty[i].x, dirty[i].y, &cx, &cy);
		cell_to_vscr(cx, cy, &sx, &sy);		/* top corner of first cell */
		sx -= CELL_XSZ / 2;					/* top-left bound */

		row = 0;
		while(sy < dirty[i].y + dirty[i].h) {
			x = (row & 1) ? sx - CELL_XSZ / 2 : sx;
			cx0 = cx;
			cy0 = cy;
			while(x < dirty[i].x + dirty[i].w) {
				if((cell = get_level_cell(&lvl, cx, cy))) {
					draw_level_cell(&lvl, cell, 0);
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
	}

	clear_dirty();

#ifdef VGA_LFB
	vga_pgflip(1);
#endif
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


static void scroll(int dx, int dy)
{
	int new_blocks;
	/* TODO: dx */

	/* NOTES:
	 * - accumulate deltas
	 * - act when delta > row size (CELL_YSZ/2)
	 */

	scroll_dx += dx;
	scroll_dy += dy;

	if(scroll_dy >= ROW_SIZE) {
		new_blocks = scroll_dy / ROW_SIZE;
		dirty_rect(vport.x, vport.y + vport.h, vport.w, new_blocks * ROW_SIZE);
		scroll_dy = 0;
	}

	vport.x += dx;
	vport.y += dy;

	vga_scroll(dx, dy);
}

static void dirty_rect(int x, int y, int w, int h)
{
	struct rect *dptr;

	if(num_dirty >= max_dirty) {
		max_dirty <<= 1;
		dirty = realloc_nf(dirty, max_dirty * sizeof *dirty);
	}

	dptr = dirty + num_dirty++;
	dptr->x = x;
	dptr->y = y;
	dptr->w = w;
	dptr->h = h;
}

static INLINE void clear_dirty(void)
{
	num_dirty = 0;
}
