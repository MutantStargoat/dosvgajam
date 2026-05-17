#ifndef PLAYER_H_
#define PLAYER_H_

#include "level.h"

enum {
	MOB_IDLE,
	MOB_WALK,
	MOB_FIRE
};

#define NUM_WALK_FRAMES	8

struct mob {
	int32_t x, y;
	int state, dir, anmfrm;
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

#endif	/* PLAYER_H_ */
