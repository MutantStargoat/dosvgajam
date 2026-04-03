#ifndef FAKEVGA_H_
#define FAKEVGA_H_

#include "vga.h"

struct cmap_color {
	uint8_t r, g, b;
};

extern int win_width, win_height;
extern float win_aspect;

extern struct cmap_color fake_cmap[256];

#endif	/* FAKEVGA_H_ */
