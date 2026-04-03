#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "vga.h"
#include "app.h"
#include "g3d/g3d.h"
#include "timer.h"
#include "options.h"

int mouse_x, mouse_y;
unsigned int mouse_bnstate;
unsigned int modkeys;

long time_msec;

struct app_screen *cur_scr;

/* available screens */
#define MAX_SCREENS	8
static struct app_screen *screens[MAX_SCREENS];
static int num_screens;


int app_init(void)
{
	int i;
	char *start_scr_name;

	if(vga_setmodex() == -1) {
		return -1;
	}

	g3d_init();
	g3d_framebuffer(FB_WIDTH, FB_HEIGHT, 0);

	/* initialize screens */
	screens[num_screens++] = &scr_menu;
	screens[num_screens++] = &scr_game;

	start_scr_name = getenv("START_SCREEN");

	for(i=0; i<num_screens; i++) {
		if(screens[i]->init() == -1) {
			return -1;
		}
	}

	time_msec = get_msec();

	for(i=0; i<num_screens; i++) {
		if(screens[i]->name && start_scr_name && strcmp(screens[i]->name, start_scr_name) == 0) {
			app_chscr(screens[i]);
			break;
		}
	}
	if(!cur_scr) {
		app_chscr(&scr_game);
	}

	return 0;
}

void app_shutdown(void)
{
	int i;

	vga_cleanup();

	for(i=0; i<num_screens; i++) {
		if(screens[i]->destroy) {
			screens[i]->destroy();
		}
	}
}

void app_display(void)
{
	time_msec = get_msec();

	cur_scr->display();
}

void app_keyboard(int key, int press)
{
	if(press) {
		switch(key) {
		case 27:
			app_quit();
			break;
		}
	}

	if(cur_scr && cur_scr->keyboard) {
		cur_scr->keyboard(key, press);
	}
}

void app_mouse(int bn, int st, int x, int y)
{
	mouse_x = x;
	mouse_y = y;

	if(st) {
		mouse_bnstate |= 1 << bn;
	} else {
		mouse_bnstate &= ~(1 << bn);
	}

	if(cur_scr && cur_scr->mouse) {
		cur_scr->mouse(bn, st, x, y);
	}
}

void app_motion(int x, int y)
{
	if(cur_scr && cur_scr->motion) {
		cur_scr->motion(x, y);
	}
	mouse_x = x;
	mouse_y = y;
}

void app_chscr(struct app_screen *scr)
{
	struct app_screen *prev = cur_scr;

	if(!scr) return;

	if(scr->start && scr->start() == -1) {
		return;
	}

	if(prev && prev->stop) {
		prev->stop();
	}
	cur_scr = scr;
}
