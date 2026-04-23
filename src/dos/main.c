#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include "app.h"
#include "vga.h"
#include "options.h"
#include "logger.h"
#include "keyb.h"
#include "timer.h"
#include "mouse.h"

static int quit, use_mouse;

static void upd_mouse(void);

int main(int argc, char **argv)
{
	load_options("game.cfg");
	init_logger(opt.logfile);

	kb_init();
	init_timer(100);

	use_mouse = have_mouse();

	if(app_init() == -1) {
		return 1;
	}

	printf("cur screen: %s\n", cur_scr->name);

	for(;;) {
		int key;
		while((key = kb_getkey()) != -1) {
			app_keyboard(key, 1);
			if(quit) goto end;
		}

		if(use_mouse) {
			upd_mouse();
		}

		app_display();
	}

end:
	save_options("game.cfg");
	app_shutdown();
	kb_shutdown();
	stop_logger();
	print_tail(opt.logfile);
	return 0;
}

void app_quit(void)
{
	quit = 1;
}

void app_abort(void)
{
	vga_cleanup();
	kb_shutdown();
	stop_logger();
	print_tail(opt.logfile);
	abort();
}

int app_keydown(int key)
{
	return kb_isdown(key);
}

static void upd_mouse(void)
{
	static int dx, dy, prev_mx, prev_my, mx = 160, my = 120;
	static int bn, prev_bn, bndiff;

	read_mouse_rel(&dx, &dy);
	if((dx | dy)) {
		mx += dx;
		my += dy;
		if(mx < 0) mx = 0;
		if(mx >= 320) mx = 319;
		if(my < 0) my = 0;
		if(my >= 240) my = 239;

		if(mx != prev_mx || my != prev_my) {
			app_motion(mx, my);
			prev_mx = mx;
			prev_my = my;
		}
	}

	bn = read_mouse_bn();
	bndiff = bn ^ prev_bn;

	if(bndiff) {
		prev_bn = bn;

		if(bndiff & 1) {
			app_mouse(0, bn & 1, mx, my);
		}
		if(bndiff & 2) {
			app_mouse(1, bn & 2, mx, my);
		}
		if(bndiff & 4) {
			app_mouse(2, bn & 4, mx, my);
		}
	}
}
