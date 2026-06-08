#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "psys.h"
#include "vga.h"
#include "app.h"

struct color {
	int r, g, b;
};
static struct color ramp_gun[] = {{255, 222, 166}, {192, 52, 38}, {67, 32, 32}};
static struct color ramp_flame[] = {
	{255, 245, 10}, {255, 180, 15}, {255, 64, 20}, {160, 40, 10}, {64, 8, 8}
};
static struct color ramp_blast[] = {{192, 192, 255}, {100, 100, 220}, {92, 92, 160}};

#define CIDX_GUN0		200
#define CIDX_GUN_LEN	(sizeof ramp_gun / sizeof *ramp_gun)
#define CIDX_FLAME0		(CIDX_GUN0 + CIDX_GUN_LEN)
#define CIDX_FLAME_LEN	(sizeof ramp_flame / sizeof *ramp_flame)
#define CIDX_BLAST0		(CIDX_FLAME0 + CIDX_FLAME_LEN)
#define CIDX_BLAST_LEN	(sizeof ramp_blast / sizeof *ramp_blast)

#define MAX_PARTICLES	256
static struct particle particles[MAX_PARTICLES];
static struct particle *ppool;	/* free particle list (points into particles[]) */

struct psys psys_blasthit, psys_gunhit, psys_flame;

void psys_init_prefabs(void)
{
	int i;

	for(i=0; i<CIDX_GUN_LEN; i++) {
		vga_setpal(i ? -1 : i + CIDX_GUN0, ramp_gun[i].r, ramp_gun[i].g, ramp_gun[i].b);
	}
	for(i=0; i<CIDX_FLAME_LEN; i++) {
		vga_setpal(-1, ramp_flame[i].r, ramp_flame[i].g, ramp_flame[i].b);
	}
	for(i=0; i<CIDX_BLAST_LEN; i++) {
		vga_setpal(-1, ramp_blast[i].r, ramp_blast[i].g, ramp_blast[i].b);
	}

	psys_init(&psys_gunhit);
	psys_gunhit.emlife = 400;
	psys_gunhit.rad = 0x300;
	psys_gunhit.grav = -0x500;
	psys_gunhit.colramp[0] = CIDX_GUN0;
	psys_gunhit.colramp[1] = CIDX_GUN0 + CIDX_GUN_LEN;
	psys_gunhit.spawn_rate = 0x1000;

	psys_init(&psys_flame);
	psys_flame.rad = 0x200;
	psys_flame.grav = 0x800;
	psys_flame.colramp[0] = CIDX_FLAME0;
	psys_flame.colramp[1] = CIDX_FLAME0 + CIDX_FLAME_LEN;


	psys_init(&psys_blasthit);
	psys_blasthit.emlife = 400;
	psys_blasthit.rad = 0x300;
	psys_blasthit.grav = -0x800;
	psys_blasthit.colramp[0] = CIDX_BLAST0;
	psys_blasthit.colramp[1] = CIDX_BLAST0 + CIDX_BLAST_LEN;
	psys_blasthit.spawn_rate = 0x1800;

}


void psys_init(struct psys *ps)
{
	memset(ps, 0, sizeof *ps);
	ps->x = 160;
	ps->y = 120;
	ps->rad = 0x200;
	ps->grav = -0x80;
	ps->emlife = -1;
	ps->plife = 2000;
	ps->colramp[0] = ps->colramp[1] = 0xff;
	ps->spawn_rate = 0xa00;
}

void psys_destroy(struct psys *ps)
{
	struct particle *tmp, *p = ps->plist;
	while(p) {
		tmp = p;
		p = p->next;
		pfree(tmp);
	}
}

int psys_update(struct psys *ps, long dt)
{
	int32_t t;
	long life;
	struct particle *p, dummy, *pprev;

	/* emitter lifetime */
	if(ps->emlife > 0) {
		ps->emlife -= dt;
		if(ps->emlife <= 0) {
			ps->emlife = 0;
		}
	}

	/* update existing particles, remove dead ones */
	dummy.next = ps->plist;
	pprev = &dummy;
	while((p = pprev->next)) {
		p->life -= dt;
		if(p->life <= 0) {
			/* dead, remove it and go on */
			pprev->next = p->next;
			pfree(p);
			ps->npart--;
			continue;
		}

		life = ps->plife - p->life;
		t = (life << 8) / ps->plife;
		p->color = ps->colramp[0] + (((ps->colramp[1] - ps->colramp[0]) * t) >> 8);

		p->x = p->x + ((p->vx * dt) >> 10);	/* div 1024 instead of 1000... close enough */
		p->y = p->y + ((p->vy * dt) >> 10);
		/* the >> 9 here instead of >> 10, is a hack to add some damping */
		p->vx = p->vx - ((p->vx * dt) >> 9);
		p->vy = p->vy - (((p->vy + ps->grav) * dt) >> 9);

		pprev = pprev->next;
	}
	ps->plist = dummy.next;

	/* spawn new particles */
	if(ps->emlife == 0) return 1;

	ps->spawn_acc += (ps->spawn_rate * dt) >> 10;
	while(ps->spawn_acc >= 0x100) {
		ps->spawn_acc -= 0x100;
		if(!(p = palloc())) continue;

		p->x = ps->x << 8;
		p->y = ps->y << 8;
		if(ps->rad) {
			p->x += (rand() % (ps->rad << 1) - ps->rad);
			p->y += (rand() % (ps->rad << 1) - ps->rad);
		}
		p->vx = ps->dirx;
		p->vy = ps->diry;
		p->life = ps->plife;
		p->color = ps->colramp[0];
		p->next = ps->plist;
		ps->plist = p;
		ps->npart++;
	}
	return 1;
}

void psys_draw(struct psys *ps)
{
	int x, y;
	struct particle *p;

#ifdef VGA_LFB
	if(cur_bpl) return;
#endif

	p = ps->plist;
	while(p) {
		x = (p->x >> 8) - xscroll;
		y = (p->y >> 8) - yscroll;

		if(BOUNDCHK(x, FB_WIDTH) && BOUNDCHK(y, FB_HEIGHT)) {
#ifdef VGA_LFB
			vga_backbuf[y * SCANLEN + x] = p->color;
#else
			if((x & 3) == cur_bpl) {
				vga_backbuf[y * SCANLEN + (x >> 2)] = p->color;
			}
#endif
		}
		p = p->next;
	}
}


void psys_add_emitter(struct psys **pslist, struct psys *ps)
{
	ps->next = *pslist;
	*pslist = ps;
}

void psys_upd_emitters(struct psys **pslist, long dt)
{
	struct psys *ps, *psprev, dummy;

	dummy.next = *pslist;
	psprev = &dummy;
	while((ps = psprev->next)) {
		psys_update(ps, dt);

		if(!ps->emlife && !ps->npart) {
			psprev->next = ps->next;
			free(ps);
			continue;
		}

		psprev = psprev->next;
	}
	*pslist = dummy.next;
}


struct particle *palloc(void)
{
	int i;
	struct particle *p;
	static int ppool_initialized, logged_plimit;

	if(!ppool_initialized) {
		/* first call, initialize particles */
		for(i=0; i<MAX_PARTICLES-1; i++) {
			particles[i].next = particles + (i + 1);
		}
		ppool = particles;
		ppool_initialized = 1;
	}

	if(!ppool) {
		if(!logged_plimit) {
			fprintf(stderr, "palloc: exceeded particle limit (%d)\n", MAX_PARTICLES);
			logged_plimit = 1;
		}
		return 0;
	}

	p = ppool;
	ppool = ppool->next;
	p->next = 0;
	return p;
}

void pfree(struct particle *p)
{
	p->next = ppool;
	ppool = p;
}
