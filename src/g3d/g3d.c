#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "g3d.h"
#include "vga.h"
#include "util.h"
#include "xmath.h"

struct rect {
	int x0, y0, x1, y1;
};

static int32_t mvmat[16];
static int32_t pmat[16];
static int32_t mvpmat[16];
static int mvp_recalc;
static int vp[4];

unsigned char *g3d_fbpixels;
int g3d_width, g3d_height;
unsigned int g3d_curcidx;


int g3d_init(void)
{
	memset(mvmat, 0, sizeof mvmat);
	memset(pmat, 0, sizeof pmat);
	mvmat[0] = mvmat[5] = mvmat[10] = mvmat[15] = 0x10000;
	pmat[0] = pmat[5] = pmat[10] = pmat[15] = 0x10000;
	mvp_recalc = 1;
	g3d_curcidx = 0xff;
	return 0;
}

void g3d_shutdown(void)
{
}

void g3d_framebuffer(int width, int height, void *fb)
{
	g3d_fbpixels = fb ? fb : vga_backbuf;
	g3d_width = width;
	g3d_height = height;

	vp[0] = vp[1] = 0;
	vp[2] = width;
	vp[3] = height;
}

void g3d_modelview(const int32_t *m)
{
	memcpy(mvmat, m, sizeof mvmat);
	mvp_recalc = 1;
}

void g3d_projection(const int32_t *m)
{
	memcpy(pmat, m, sizeof pmat);
	mvp_recalc = 1;
}

void g3d_color(int cidx)
{
	g3d_curcidx = cidx;
}

static INLINE void update_matrix(void)
{
	if(!mvp_recalc) return;

	memcpy(mvpmat, pmat, sizeof mvpmat);
	mat_mult(mvpmat, mvmat);

	mvp_recalc = 0;
}

void g3d_draw(int prim, struct g3d_vertex *varr, int vcount)
{
	int i;

	for(i=0; i<vcount; i+=prim) {
		g3d_draw_prim(prim, varr, 0);
		varr += prim;
	}
}

void g3d_draw_indexed(int prim, struct g3d_vertex *varr, unsigned short *idxarr,
		int idxcount)
{
	int i;

	for(i=0; i<idxcount; i+=prim) {
		g3d_draw_prim(prim, varr, idxarr);
		idxarr += prim;
	}
}

void g3d_draw_prim(int prim, struct g3d_vertex *varr, unsigned short *idxarr)
{
	int i, vcount, x, y;
	struct g3d_vertex v[4];

	update_matrix();

	vcount = prim;

	for(i=0; i<vcount; i++) {
		v[i] = idxarr ? varr[idxarr[i]] : varr[i];

		/* transform to view space */
		g3d_xform3(v + i, mvmat);

		/* transform to homogeneous clip space */
		g3d_xform(v + i, pmat);

		/* TODO clip */

		/* perspective division */
		if(v[i].w != 0) {
			v[i].x = (v[i].x << 4) / (v[i].w >> 4);
			v[i].y = (v[i].y << 4) / (v[i].w >> 4);
		} else {
			v[i].x >>= 8;
			v[i].y >>= 8;
		}

		/* viewport transform */
		v[i].x = (v[i].x + 0x80) * vp[2] + (vp[0] << 8);
		v[i].y = (0x80 - v[i].y) * vp[3] + (vp[1] << 8);
	}

	g3d_curcidx = v[0].cidx;

	switch(prim) {
	case G3D_POINTS:
		x = v[0].x >> 8;
		y = v[0].y >> 8;
		if(x >= 0 && y >= 0 && x < FB_WIDTH && y < FB_HEIGHT) {
			g3d_fbpixels[y * FB_WIDTH + x] = g3d_curcidx;
		}
		break;

	case G3D_LINES:
		break;	/* TODO */

	case G3D_TRIANGLES:
		g3d_polyfill(v);
		break;

	case G3D_QUADS:
		g3d_polyfill(v);
		v[1] = v[0];
		g3d_polyfill(v + 1);
		break;
	}
}
