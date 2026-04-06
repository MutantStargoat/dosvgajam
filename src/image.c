#include <stdio.h>
#include <stdlib.h>
#include "image.h"
#include "imago2.h"

struct image *load_image(const char *fname)
{
	int i, x, y;
	unsigned int planesz, pscansz;
	struct img_pixmap pixmap;
	struct img_colormap *cmap;
	struct image *img;
	uint8_t *src, *dest;

	img_init(&pixmap);
	if(img_load(&pixmap, fname) == -1) {
		fprintf(stderr, "load_image: failed to load %s\n", fname);
		return 0;
	}
	if(!(cmap = img_colormap(&pixmap))) {
		fprintf(stderr, "load_image: %s isn't an indexed color image\n", fname);
		img_destroy(&pixmap);
		return 0;
	}

	if(!(img = calloc(1, sizeof *img))) {
		fprintf(stderr, "load_image: failed to allocate image structure\n");
		img_destroy(&pixmap);
		return 0;
	}

	img->width = pixmap.width;
	img->height = pixmap.height;
	img->pixels = pixmap.pixels;

	img->ncolors = cmap->ncolors;
	for(i=0; i<cmap->ncolors; i++) {
		img->cmap[i].r = cmap->color[i].r;
		img->cmap[i].g = cmap->color[i].g;
		img->cmap[i].b = cmap->color[i].b;
	}

	/* separate pixel planes for faster blits */
	pscansz = img->width >> 2;
	planesz = pscansz * img->height;

	if(!(img->plane[0] = malloc(img->width * img->height))) {
		fprintf(stderr, "load_image: failed to allocate pixel plane blocks\n");
	}

	for(i=0; i<4; i++) {
		if(i) img->plane[i] = img->plane[i - 1] + planesz;

		src = img->pixels + i;
		dest = img->plane[i];

		for(y=0; y<img->height; y++) {
			for(x=0; x<pscansz; x++) {
				*dest++ = *src;
				src += 4;
			}
		}
	}

	return img;
}

void free_image(struct image *img)
{
	struct img_pixmap pixmap;

	if(!img) return;

	img_init(&pixmap);
	pixmap.pixels = img->pixels;
	img_destroy(&pixmap);
	free(img->plane[0]);
}
