#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include "app.h"
#include "vga.h"
#include "options.h"
#include "logger.h"
#include "timer.h"

static int quit;

int main(int argc, char **argv)
{
	load_options("game.cfg");
	init_logger(opt.logfile);

	init_timer(100);

	if(app_init() == -1) {
		return 1;
	}

	printf("cur screen: %s\n", cur_scr->name);

	for(;;) {
		while(kbhit()) {
			if(getch() == 27) {
				quit = 1;
			}
			if(quit) goto end;
		}

		app_display();
	}

end:
	save_options("game.cfg");
	app_shutdown();
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
	stop_logger();
	print_tail(opt.logfile);
	abort();
}
