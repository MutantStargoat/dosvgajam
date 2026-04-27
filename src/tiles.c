#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tiles.h"
#include "app.h"
#include "image.h"
#include "util.h"
#include "dynarr.h"
#include "vga.h"
#include "level.h"


#define RLE_OP_SKIP			0
#define RLE_OP_COPY			0x80
#define RLE_OP_BITS			0x80
#define RLE_COUNT_BITS		0x7f

static void conv_rle(struct tileimg *tile, int bpl);

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
		dynarr_free(ts->tiles[i]->rle);
		free(ts->tiles[i]);
	}
	dynarr_free(ts->tiles);
	free(ts->plane[0]);
}

struct tileimg *tiles_define(struct tileset *ts, int x, int y, int w, int h)
{
	struct tileimg *tile = calloc_nf(1, sizeof *tile);

	dynarr_push_nf(ts->tiles, &tile);

	tile->sheet = ts;
	tile->x = x;
	tile->y = y;
	tile->width = w;
	tile->height = h;
	tile->imgptr = ts->pixels + y * ts->pitch + x;

	tile->planeptr[0] = ts->plane[0] + y * ts->pscansz + (x >> 2);
	tile->planeptr[1] = tile->planeptr[0] + ts->psize;
	tile->planeptr[2] = tile->planeptr[1] + ts->psize;
	tile->planeptr[3] = tile->planeptr[2] + ts->psize;

#ifdef VGA_LFB
	conv_rle(tile, -1);
#else
	conv_rle(tile, 0);
	conv_rle(tile, 1);
	conv_rle(tile, 2);
	conv_rle(tile, 3);
#endif

	return tile;
}

#ifdef VGA_LFB
void tiles_blit_key(struct tileimg *tile, int x, int y)
{
	int i, j;
	uint8_t *src, *dst, ckey = tile->sheet->ckey;

	x -= tile->xorg;
	y -= tile->yorg;

	if(x < -32) return;
	if(x >= FB_WIDTH) return;
	if(y < -16) return;
	if(y >= FB_HEIGHT) return;

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

void tiles_blit_rle(struct tileimg *tile, int x, int y)
{
	unsigned int rle, op, count;
	uint8_t *rleptr, *end, *dst, *dstrow;

	x -= tile->xorg;
	y -= tile->yorg;

	if(x < -32) return;
	if(x >= FB_WIDTH) return;
	if(y < -16) return;
	if(y >= FB_HEIGHT) return;

	rleptr = tile->rle;
	end = rleptr + dynarr_size(tile->rle);

	dst = dstrow = vga_backbuf + y * VGA_PITCH + x;

	while(rleptr < end) {
		rle = (unsigned int)*rleptr++;
		op = rle & RLE_OP_BITS;
		count = rle & RLE_COUNT_BITS;

		if(op & RLE_OP_COPY) {
			memcpy(dst, rleptr, count);
			dst += count;
			rleptr += count;
		} else {
			/* skip */
			if(count) {
				dst += count;
			} else {
				/* skip 0 means next scanline */
				dstrow += VGA_PITCH;
				dst = dstrow;
			}
		}
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

	x -= tile->xorg;
	y -= tile->yorg;

	if(x < -32) return;
	if(x >= FB_WIDTH) return;
	if(y < -16) return;
	if(y >= FB_HEIGHT) return;

	offs = y * VGA_PITCH + (x >> 2);
	mask = 1 << (x & 3);

	for(k=0; k<4; k++) {
		vga_planemask(mask);
		src = tile->planeptr[k];
		dst = vga_backbuf + offs;
		for(i=0; i<tile->height; i++) {
			for(j=0; j<bpwidth; j++) {
				uint8_t pixel = src[j];
				if(pixel != ckey) {
					dst[j] = pixel;
				}
			}
			dst += VGA_PITCH;
			src += srcpitch;
		}

		mask = (mask << 1) & 0xf;
		if(!mask) {
			mask = 1;
			offs++;
		}
	}
}

void tiles_blit_rle(struct tileimg *tile, int x, int y)
{
	int k, offs;
	unsigned int rle, op, count, mask;
	uint8_t *rleptr, *end, *dst, *dstrow;

	x -= tile->xorg;
	y -= tile->yorg;

	if(x < -32) return;
	if(x >= FB_WIDTH) return;
	if(y < -16) return;
	if(y >= FB_HEIGHT) return;

	offs = y * VGA_PITCH + (x >> 2);
	mask = 1 << (x & 3);

	for(k=0; k<4; k++) {
		rleptr = tile->prle[k];
		end = rleptr + dynarr_size(rleptr);

		vga_planemask(mask);
		dst = dstrow = vga_backbuf + offs;

		while(rleptr < end) {
			rle = (unsigned int)*rleptr++;
			op = rle & RLE_OP_BITS;
			count = rle & RLE_COUNT_BITS;

			if(op & RLE_OP_COPY) {
				memcpy(dst, rleptr, count);
				dst += count;
				rleptr += count;
			} else {
				/* skip */
				if(count) {
					dst += count;
				} else {
					/* skip 0 means next scanline */
					dstrow += VGA_PITCH;
					dst = dstrow;
				}
			}
		}

		mask = (mask << 1) & 0xf;
		if(!mask) {
			mask = 1;
			offs++;
		}
	}
}
#endif


#define ADD_RLE_SKIP(skip) \
	do { \
		uint8_t foo = RLE_OP_SKIP | ((skip) & RLE_COUNT_BITS); \
		dynarr_push_nf(rlebuf, &foo); \
	} while(0)

#define ADD_RLE_SPAN(count, ptr) \
	do { \
		int i; \
		uint8_t *p = ptr; \
		uint8_t foo = RLE_OP_COPY | ((count) & RLE_COUNT_BITS); \
		dynarr_push_nf(rlebuf, &foo); \
		for(i=0; i<count; i++) { \
			dynarr_push_nf(rlebuf, p++); \
		} \
	} while(0)

static void conv_rle(struct tileimg *tile, int bpl)
{
	int i, j, scanlen, count, skip;
	unsigned int pitch;
	uint8_t *rlebuf, *srcrow, *sptr, ckey;

	rlebuf = dynarr_alloc_nf(0, 1);

	ckey = tile->sheet->ckey;

	if(bpl < 0) {
		pitch = tile->sheet->pitch;
		srcrow = tile->imgptr;
		scanlen = tile->width;
	} else {
		pitch = tile->sheet->pscansz;
		srcrow = tile->planeptr[bpl];
		scanlen = tile->width / 4;
	}

	for(i=0; i<tile->height; i++) {
		count = skip = 0;
		sptr = srcrow;
		for(j=0; j<scanlen; j++) {
			if(*sptr == ckey) {
				/* transparent pixel */
				if(count) {
					/* we had non-transparent up to now, add a span */
					ADD_RLE_SPAN(count, sptr - count);
					count = 0;
				}

				skip++;

			} else {
				/* non-transparent pixel */
				if(skip) {
					/* we had transparent up to now, add a skip */
					ADD_RLE_SKIP(skip);
					skip = 0;
				}

				count++;
			}
			sptr++;
		}

		/* end of scanline, add any residuals */
		if(count) {
			ADD_RLE_SPAN(count, sptr - count);
			count = 0;
		}
		ADD_RLE_SKIP(0);	/* skip with 0 count means skip to next scanline */
		skip = 0;

		srcrow += pitch;
	}

	if(bpl < 0) {
		tile->rle = rlebuf;
	} else {
		tile->prle[bpl] = rlebuf;
	}
}
