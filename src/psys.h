#ifndef PSYS_H_
#define PSYS_H_

#include "szint.h"

struct particle {
	int32_t x, y;
	int32_t vx, vy;
	long life;
	int color;

	struct particle *next;
};

struct psys {
	int x, y;
	int32_t rad;
	int32_t dirx, diry;
	int32_t grav;
	long emlife, plife;		/* milliseconds */
	int colramp[2];
	int32_t spawn_rate, spawn_acc;

	int npart;
	struct particle *plist;

	struct psys *next;
};

/* prefabs */
extern struct psys psys_blasthit, psys_gunhit, psys_flame;

void psys_init_prefabs(void);

void psys_init(struct psys *ps);
void psys_destroy(struct psys *ps);
int psys_update(struct psys *ps, long dt);
void psys_draw(struct psys *ps);

void psys_add_emitter(struct psys **pslist, struct psys *ps);
void psys_upd_emitters(struct psys **pslist, long dt);

struct particle *palloc(void);
void pfree(struct particle *p);


#endif	/* PSYS_H_ */
