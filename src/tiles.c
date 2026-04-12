#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tiles.h"
#include "image.h"
#include "util.h"
#include "dynarr.h"
#include "vga.h"


int tiles_load(struct tileset *ts, const char *fname)
{
	struct image *img;
	char *path;

	ts->name = strdup_nf(fname);

	if(!strstr(fname, "data/")) {
		path = alloca(strlen(fname) + 6);
		sprintf(path, "data/%s", fname);
	} else {
		path = (char*)fname;
	}

	if(!(img = load_image(path))) {
		return -1;
	}
	ts->width = img->width;
	ts->height = img->height;
	ts->pitch = img->width;
	if(next_pow2(ts->pitch - 1) == ts->pitch) {
		ts->xshift = calc_shift(ts->pitch);
	} else {
		ts->xshift = 0;
	}
	ts->pixels = img->pixels;
	ts->plane[0] = img->plane[0];
	ts->plane[1] = img->plane[1];
	ts->plane[2] = img->plane[2];
	ts->plane[3] = img->plane[3];
	ts->pscansz = ts->pitch >> 2;
	ts->psize = ts->pscansz * img->height;
	memcpy(ts->cmap, img->cmap, sizeof ts->cmap);
	ts->ncolors = img->ncolors;
	ts->ckey = 0;

	ts->tiles = dynarr_alloc_nf(0, sizeof *ts->tiles);
	return 0;
}

void tiles_destroy(struct tileset *ts)
{
	int i;

	free(ts->name);
	free(ts->pixels);

	for(i=0; i<dynarr_size(ts->tiles); i++) {
		free(ts->tiles[i]);
	}
	dynarr_free(ts->tiles);
	free(ts->plane[0]);
}

struct tileimg *tiles_define(struct tileset *ts, int x, int y, int w, int h)
{
	struct tileimg *tile = malloc_nf(sizeof *tile);

	dynarr_push_nf(ts->tiles, &tile);

	tile->sheet = ts;
	tile->x = x;
	tile->y = y;
	tile->width = w;
	tile->height = h;
	tile->imgptr = ts->pixels + y * ts->pitch + x;
	tile->rle = 0;	/* TODO */

	tile->planeptr[0] = ts->plane[0] + y * ts->pscansz + (x >> 2);
	tile->planeptr[1] = tile->planeptr[0] + ts->psize;
	tile->planeptr[2] = tile->planeptr[1] + ts->psize;
	tile->planeptr[3] = tile->planeptr[2] + ts->psize;

	return tile;
}

#ifdef VGA_LFB
void tiles_blit_key(struct tileimg *tile, int x, int y)
{
	int i, j;
	uint8_t *src, *dst, ckey = tile->sheet->ckey;

	src = tile->imgptr;
	dst = vga_backbuf + y * VGA_PITCH + x;

	for(i=0; i<tile->height; i++) {
		for(j=0; j<tile->width; j++) {
			uint8_t pixel = src[j];
			if(pixel != ckey) {
				dst[j] = pixel;
			}
		}
		dst += VGA_PITCH;
		src += tile->sheet->pitch;
	}
}

#else

void tiles_blit_key(struct tileimg *tile, int x, int y)
{
	int i, j, k, offs;
	unsigned int mask;
	unsigned int bpwidth = tile->width >> 2;
	unsigned int srcpitch = tile->sheet->pscansz;
	uint8_t *src, *dst, ckey = tile->sheet->ckey;
	/* TODO RLE blit */

	offs = y * vga_pitch + (x >> 2);
	mask = 1 << (x & 3);

	for(k=0; k<4; k++) {
		vga_planemask(mask);
		src = tile->planeptr[k];
		dst = VGA_VMEM + offs;
		for(i=0; i<tile->height; i++) {
			for(j=0; j<bpwidth; j++) {
				uint8_t pixel = src[j];
				if(pixel != ckey) {
					dst[j] = pixel;
				}
			}
			dst += vga_pitch;
			src += srcpitch;
		}

		mask = (mask << 1) & 0xf;
		if(!mask) {
			mask = 1;
			offs++;
		}
	}
}
#endif
