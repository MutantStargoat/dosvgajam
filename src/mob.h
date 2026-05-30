#ifndef MOB_H_
#define MOB_H_

#include "level.h"
#include "sprite.h"

enum {
	MOB_INVALID,

	MOB_IDLE,
	MOB_WALK,
	MOB_FIRE,

	NUM_MOB_STATES
};

struct beam;

struct beamseg {
	struct beam *beam;
	struct level_cell *cell;
	int32_t x0, y0, x1, y1;
	int color;

	struct beamseg *next;	/* for level cell linked list */
};

#define MAX_BEAM_SEG	16
struct beam {
	int x0, y0, x1, y1;
	struct beamseg seg[MAX_BEAM_SEG];
	int nseg;
};

struct mob {
	int32_t x, y, rad;
	int state, dir;
	struct sprite spr;
	int hp, dmg;	/* hit points, and damage accumulator since last update */
	int32_t hitx, hity;	/* position of last hit */
	struct level *lvl;
	struct level_cell *cell;
	struct beam beam;

	struct mob *next;
};

void init_mob(struct mob *mob);

struct mob *create_mob(void);
void free_mob(struct mob *mob);

/* returns non-zero if the player moved */
int mob_move(struct mob *mob, int dx, int dy);

void mob_lookat(struct mob *mob, int32_t x, int32_t y);

void mob_state(struct mob *mob, int st);

/* shoot a beam towards a target position, check against level cells, and break
 * it into beam segments, one per cell to be able to draw it correctly
 */
void mob_beam(struct mob *mob, int32_t tx, int32_t ty, int dmg);

int32_t mob_rayhit(struct mob *mob, int32_t ox, int32_t oy, int32_t dx, int32_t dy,
		int32_t *hitx, int32_t *hity);

#endif	/* MOB_H_ */
