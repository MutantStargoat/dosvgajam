#include "app.h"
#include "vga.h"
#include "g3d/g3d.h"
#include "imago2.h"


static struct g3d_vertex cube_varr[] = {
/*       x         y         z        w     u  v cidx lit       */
	{-0x10000, -0x10000,  0x10000, 0x10000, 0, 0, 6, 15},
	{ 0x10000, -0x10000,  0x10000, 0x10000, 0, 0, 1, 15},
	{ 0x10000, -0x10000, -0x10000, 0x10000, 0, 0, 2, 15},
	{-0x10000, -0x10000, -0x10000, 0x10000, 0, 0, 3, 15},
	{-0x10000,  0x10000,  0x10000, 0x10000, 0, 0, 4, 15},
	{ 0x10000,  0x10000,  0x10000, 0x10000, 0, 0, 0, 0},
	{ 0x10000,  0x10000, -0x10000, 0x10000, 0, 0, 0, 0},
	{-0x10000,  0x10000, -0x10000, 0x10000, 0, 0, 5, 15}
};
static uint16_t cube_faces[] = {
	0, 1, 5, 4,
	1, 2, 6, 5,
	2, 3, 7, 6,
	7, 3, 0, 4,
	4, 5, 6, 7,
	3, 2, 1, 0
};

static struct img_pixmap bgimg;


static int scrgame_init(void)
{
	img_init(&bgimg);
	if(img_load(&bgimg, "data/bgimg2.png") == -1) {
		fprintf(stderr, "failed to load bgimage\n");
		return -1;
	}
	if(img_color_offset(&bgimg, 16) == -1) {
		fprintf(stderr, "failed to offset bgimage colormap\n");
		return -1;
	}
	return 0;
}

static void scrgame_destroy(void)
{
	img_destroy(&bgimg);
}

static int scrgame_start(void)
{
	int i;
	int32_t proj[16];
	struct img_colormap *cmap;

	mat_perspective(proj, 50, (4 << 16) / 3, 0x8000, 0x100000);
	g3d_projection(proj);

	cmap = img_colormap(&bgimg);
	for(i=0; i<cmap->ncolors; i++) {
		vga_setpal(i == 0 ? 16 : -1, cmap->color[i].r, cmap->color[i].g, cmap->color[i].b);
	}
	vga_setpal(255, 255, 255, 255);

	return 0;
}

static void scrgame_stop(void)
{
}

static void scrgame_display(void)
{
	int32_t xform[16];
	unsigned int anim = time_msec >> 3;

	/*vga_clearfb(0);*/
	vga_blitfb(vga_backbuf, bgimg.pixels);

	mat_trans(xform, 0, 0, -0x70000);
	mat_mul_rotx(xform, anim);
	mat_mul_roty(xform, anim);
	g3d_modelview(xform);

	g3d_fbpixels = vga_backbuf;

	g3d_draw_indexed(G3D_QUADS, cube_varr, cube_faces, 24);

	vga_pgflip(1);
}

static void scrgame_keyb(int key, int press)
{
}

static void scrgame_mouse(int bn, int press, int x, int y)
{
}

static void scrgame_motion(int x, int y)
{
}


struct app_screen scr_game = {
	"game",
	scrgame_init, scrgame_destroy,
	scrgame_start, scrgame_stop,
	scrgame_display,
	scrgame_keyb,
	scrgame_mouse, scrgame_motion
};
