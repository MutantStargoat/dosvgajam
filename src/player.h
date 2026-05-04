#ifndef PLAYER_H_
#define PLAYER_H_

#include "level.h"

struct mob {
	int32_t x, y;
	int dir;
	int hp;
	struct level *lvl;
	struct level_cell *cell;
};

/* returns non-zero if the player moved */
int mob_move(struct mob *mob, int dx, int dy);

#endif	/* PLAYER_H_ */
