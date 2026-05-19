#ifndef TILESHEET_H_
#define TILESHEET_H_

#include "szint.h"
#include "image.h"

struct tileimg;

struct tileset {
	char *name;
	int width, height;
	unsigned int pitch, pscansz, psize;
	int xshift;
	uint8_t *pixels, *plane[4];
	struct imgcolor cmap[256];
	unsigned int ncolors;
	uint8_t ckey;

	struct tileimg **tiles;	/* dynarr */
};

struct tileimg {
	struct tileset *sheet;
	int id;
	int x, y, width, height;
	int xorg, yorg;
	uint8_t *imgptr;
	uint8_t *planeptr[4];
	unsigned char *rle, *prle[4];
};

struct tileseq {
	int interv;			/* animation interval in milliseconds */
	int ntiles;
	struct tileimg **tile;
};

int tiles_load(struct tileset *ts, const char *fname);
void tiles_destroy(struct tileset *ts);

int dump_tile(struct tileimg *tile, const char *fname);

struct tileimg *tiles_define(struct tileset *ts, int x, int y, int w, int h);

void tiles_blit_key(struct tileimg *tile, int x, int y, int bpl);
void tiles_blit_rle(struct tileimg *tile, int x, int y, int bpl);
void tiles_fill_rle(struct tileimg *tile, int x, int y, int cidx, int bpl);

/* tile sequences */
struct tileseq *tileseq_define(struct tileset *ts, int n, int x, int y, int w, int h,
		int dx, int dy);
void free_tileseq(struct tileseq *tseq);

void tileseq_origin(struct tileseq *seq, int x, int y);

#endif	/* TILESHEET_H_ */
