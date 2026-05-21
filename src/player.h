#ifndef MOB_H_
#define MOB_H_

#include "level.h"
#include "sprite.h"

enum {
	MOB_IDLE,
	MOB_WALK,
	MOB_FIRE,

	NUM_MOB_STATES
};

struct mob {
	int32_t x, y;
	int state, dir;
	struct sprite spr;
	int hp;
	struct level *lvl;
	struct level_cell *cell;

	struct mob *next;
};

struct mob *create_mob(void);
void free_mob(struct mob *mob);

/* returns non-zero if the player moved */
int mob_move(struct mob *mob, int dx, int dy);

void mob_lookat(struct mob *mob, int32_t x, int32_t y);

void mob_state(struct mob *mob, int st);

#endif	/* MOB_H_ */
