#ifndef IMAGE_H_
#define IMAGE_H_

#include "szint.h"

struct imgcolor {
	uint8_t r, g, b;
};

struct image {
	int width, height;
	uint8_t *pixels, *plane[4];
	struct imgcolor cmap[256];
	int ncolors;
};

struct image *load_image(const char *fname);
void free_image(struct image *img);

#endif	/* IMAGE_H_ */
