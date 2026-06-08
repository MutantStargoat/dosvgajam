#include <stdio.h>
#include "ai.h"
#include "app.h"
#include "util.h"
#include "psys.h"

#define GUARD_DMG	16
#define GUARD_FIRE_DELAY	800

static void spawn_bullet_psys(struct level_cell *cell, int32_t x, int32_t y, int32_t dx, int32_t dy);

void ai_guard(struct mob *mob, int32_t dt)
{
	int32_t tx, ty, dx, dy;
	struct mob *hitmob;

	mob->state_t += dt;
	if(mob->spr.frm < mob->spr.nfrm) {
		mob->spr.frm++;
	}

	if(mob->state == MOB_DEAD) return;

	if(player.state == MOB_DEAD) return;

	if(mob->fire_cooldown) {
		mob->fire_cooldown -= dt;
		if(mob->fire_cooldown < 0) mob->fire_cooldown = 0;
	}

	tx = player.x;
	ty = player.y;

	dx = tx - mob->x;
	dy = ty - mob->y;

	if((hitmob = raycast(mob->lvl, mob->x, mob->y, dx, dy, mob)) == &player) {
		mob_lookat(mob, tx, ty);

		if(!mob->fire_cooldown) {
			mob_state(mob, MOB_FIRE);
			player.dmg += GUARD_DMG;
			mob->fire_cooldown = GUARD_FIRE_DELAY;

			spawn_bullet_psys(player.cell, player.x, player.y, -dx, -dy);
		}
	}
}

static void spawn_bullet_psys(struct level_cell *cell, int32_t x, int32_t y, int32_t dx, int32_t dy)
{
	struct psys *ps;

	ps = malloc_nf(sizeof *ps);
	*ps = psys_gunhit;
	ps->x = x;
	ps->y = y;

	/* poor man's normalize */
	if(dx > 0) {
		dx = 0x100;
	} else if(dx < 0) {
		dx = -0x100;
	}
	if(dy > 0) {
		dy = 0x100;
	} else if(dy < 0) {
		dy = -0x100;
	}
	ps->dirx = dx;
	ps->diry = dy;

	psys_add_emitter(&cell->psys, ps);
}
