#ifndef OPTIONS_H_
#define OPTIONS_H_

enum {
	SCALER_NEAREST,
	SCALER_LINEAR,

	NUM_SCALERS
};

struct options {
	int xres, yres;
	int vsync;
	int fullscreen;
	int scale, scaler;
	int vol_master, vol_mus, vol_sfx;
	int music;

	int mouse_speed;

	const char *logfile;
};

extern struct options opt;

int load_options(const char *fname);
int save_options(const char *fname);

#endif	/* OPTIONS_H_ */
