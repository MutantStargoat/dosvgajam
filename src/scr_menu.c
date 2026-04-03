#include "app.h"
#include "vga.h"

static int scrmenu_init(void)
{
	return 0;
}

static void scrmenu_destroy(void)
{
}

static int scrmenu_start(void)
{
	return 0;
}

static void scrmenu_stop(void)
{
}

static void scrmenu_display(void)
{
}

static void scrmenu_keyb(int key, int press)
{
}

static void scrmenu_mouse(int bn, int press, int x, int y)
{
}

static void scrmenu_motion(int x, int y)
{
}


struct app_screen scr_menu = {
	"menu",
	scrmenu_init, scrmenu_destroy,
	scrmenu_start, scrmenu_stop,
	scrmenu_display,
	scrmenu_keyb,
	scrmenu_mouse, scrmenu_motion
};
