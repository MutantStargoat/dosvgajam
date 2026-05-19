#ifndef SPRITE_H_
#define SPRITE_H_

#include "tiles.h"

#define MAX_SPR_ANIMS	16
#define NUM_DIRS		8

struct spranim {
	struct tileseq *seq[NUM_DIRS];
};

struct sprite {
	int frm, nfrm;
	struct spranim *cur;
	struct spranim anim[MAX_SPR_ANIMS];
};

/* defines a sprite animation N frames * 8 directions starting from the top-left
 * spritesheet position x, y, and continuing left for frames, and down for dirs
 * in order: W,E,S,N,NW,NE,SW,SE
 */
int define_spranim(struct tileset *ts, struct spranim *sa, int nfrm, int x, int y,
		int w, int h);

void spr_origin(struct sprite *spr, int x, int y);

#endif	/* SPRITE_H_ */
