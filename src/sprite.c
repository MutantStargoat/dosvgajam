#include "sprite.h"
#include "level.h"
#include "app.h"

int define_spranim(struct tileset *ts, struct spranim *sa, int nfrm, int x, int y,
		int w, int h)
{
	int i;
	static int diridx[] = {
		DIR8_W, DIR8_E, DIR8_S, DIR8_N, DIR8_NW, DIR8_NE, DIR8_SW, DIR8_SE
	};

	for(i=0; i<NUM_DIRS; i++) {
		if(!(sa->seq[diridx[i]] = tileseq_define(ts, nfrm, x, y, w, h, w, 0))) {
			return -1;
		}
		y += h;
	}
	return 0;
}

int define_spranim_onedir(struct tileset *ts, struct spranim *sa, int nfrm, int x,
		int y, int w, int h)
{
	int i;

	if(!(sa->seq[0] = tileseq_define(ts, nfrm, x, y, w, h, w, 0))) {
		return -1;
	}
	for(i=1; i<NUM_DIRS; i++) {
		sa->seq[i] = sa->seq[i - 1];
	}
	return 0;
}

void spr_origin(struct sprite *spr, int x, int y)
{
	int i, j;

	for(i=0; i<MAX_SPR_ANIMS; i++) {
		for(j=0; j<NUM_DIRS; j++) {
			if(spr->anim[i].seq[j]) {
				tileseq_origin(spr->anim[i].seq[j], x, y);
			}
		}
	}
}

void spr_draw(struct sprite *spr, int x, int y, int dir)
{
	struct tileimg *tile;

	if(!spr->cur) {
		tile = tile_inval;
	} else {
		tile = spr->cur->seq[dir]->tile[(spr->frm >> 1) % spr->nfrm];
	}

	tiles_blit_rle(tile, x, y, cur_bpl);
}
