#ifndef TILESHEET_H_
#define TILESHEET_H_

#include "szint.h"
#include "image.h"

struct tileimg;

struct tilesheet {
	int width, height;
	unsigned int pitch, pscansz, psize;
	int xshift;
	uint8_t *pixels, *plane[4];
	struct imgcolor cmap[256];
	unsigned int ncolors;
	uint8_t ckey;

	struct tileimg *tiles;	/* dynarr */
};

struct tileimg {
	struct tilesheet *sheet;
	int x, y, width, height;
	uint8_t *imgptr;
	uint8_t *planeptr[4];
	unsigned char *rle;
};

int tiles_load(struct tilesheet *ts, const char *fname);
void tiles_destroy(struct tilesheet *ts);

void tiles_define(struct tilesheet *ts, int x, int y, int w, int h);

void tiles_blit_key(struct tileimg *tile, int x, int y);

#endif	/* TILESHEET_H_ */
