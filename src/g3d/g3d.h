#ifndef GFX3D_H_
#define GFX3D_H_

#include "szint.h"
#include "xmath.h"
#include "app.h"

struct g3d_vertex {
	int32_t x, y, z, w;
	int32_t u, v;
	int32_t cidx, lit;
};

enum {
	G3D_POINTS		= 1,
	G3D_LINES		= 2,
	G3D_TRIANGLES	= 3,
	G3D_QUADS		= 4
};

extern unsigned char *g3d_fbpixels;
extern int g3d_width, g3d_height;
extern unsigned int g3d_curcidx;

extern int g3d_bbox[4];


int g3d_init(void);
void g3d_shutdown(void);

void g3d_framebuffer(int width, int height, void *fb);

void g3d_modelview(const int32_t *m);
void g3d_projection(const int32_t *m);

#define g3d_xform(v, m)		vec4_xform((int32_t*)(v), (m))
#define g3d_xform3(v, m)	vec3_xform((int32_t*)(v), (m))

void g3d_color(int cidx);

void g3d_draw(int prim, struct g3d_vertex *varr, int vcount);
void g3d_draw_indexed(int prim, struct g3d_vertex *varr, unsigned short *idxarr,
		int idxcount);
void g3d_draw_prim(int prim, struct g3d_vertex *varr, unsigned short *idxarr);

void g3d_polyfill(struct g3d_vertex *verts);

#endif	/* GFX3D_H_ */
