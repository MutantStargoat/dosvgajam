#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "miniglut.h"
#include "fakevga.h"
#include "app.h"
#include "options.h"

#define ASPECT		((float)FB_WIDTH / (float)FB_HEIGHT)

uint8_t *vga_backbuf;
unsigned int vga_pitch;

struct cmap_color fake_cmap[256] = {
	{0, 0, 0}, {0, 0, 0xaa}, {0, 0xaa, 0}, {0, 0xaa, 0xaa},
	{0xaa, 0, 0}, {0xaa, 0, 0xaa}, {0xaa, 0xaa, 0}, {0xaa, 0xaa, 0xaa},
	{0x55, 0x55, 0x55}, {0x55, 0x55, 0xff}, {0x55, 0xff, 0x55}, {0x55, 0xff, 0xff},
	{0xff, 0x55, 0x55}, {0xff, 0x55, 0xff}, {0xff, 0xff, 0x55}, {0xff, 0xff, 0xff}
};


#ifndef NO_GLTEX
static int tex_xsz, tex_ysz;
#else
static int xoffs, yoffs;
#endif

#if defined(__unix__) || defined(unix)
#include <GL/glx.h>
extern Display *miniglut_dpy;
extern Window miniglut_win;

typedef void (*swapint_ext_t)(Display*, Window, int);
typedef void (*swapint_mesa_t)(int);

static swapint_ext_t glx_swap_interval_ext;
static swapint_mesa_t glx_swap_interval_mesa;

#endif
#ifdef _WIN32
#include <windows.h>
static PROC wgl_swap_interval_ext;
#endif


static int create_glfb(void);
static void set_vsync(int vsync);


int vga_setmodex(void)
{
	if(!(vga_backbuf = calloc(1, SCANLEN * TOTAL_LINES))) {
		return -1;
	}
	vga_backbuf += GUARD_OFFS;

	if(create_glfb() == -1) {
		return -1;
	}

#if (defined(__unix__) || defined(unix)) && GLX_VERSION_1_4
	glx_swap_interval_ext = (swapint_ext_t)glXGetProcAddress((unsigned char*)"glXSwapIntervalEXT");
	glx_swap_interval_mesa = (swapint_mesa_t)glXGetProcAddress((unsigned char*)"glXSwapIntervalMESA");
#endif
#ifdef _WIN32
	wgl_swap_interval_ext = wglGetProcAddress("wglSwapIntervalEXT");
#endif
	return 0;
}

void vga_cleanup(void)
{
	free(vga_backbuf - GUARD_OFFS);
}

void vga_setpitch(unsigned int pitch)
{
	/* ignore pitch */
	vga_pitch = SCANLEN;
}

void vga_setpal(int16_t idx, uint8_t r, uint8_t g, uint8_t b)
{
	static int prev_idx;

	if(idx == -1) {
		idx = ++prev_idx;
	} else {
		assert(*(uint16_t*)&idx < 256);
		prev_idx = idx;
	}

	fake_cmap[idx].r = r;
	fake_cmap[idx].g = g;
	fake_cmap[idx].b = b;
}

void vga_clearfb(unsigned int color)
{
	memset(vga_backbuf, color, SCANLEN * FB_HEIGHT);
}

void vga_blitfb(uint8_t *vmem, const uint8_t *img)
{
	int i;
	for(i=0; i<FB_HEIGHT; i++) {
		memcpy(vmem, img, FB_WIDTH);
		vmem += SCANLEN;
		img += FB_WIDTH;
	}
}

void vga_vline(uint8_t *vmem, int x, int y, int len, uint8_t color)
{
	vmem += y * SCANLEN + x;

	while(len-- > 0) {
		*vmem = color;
		vmem += SCANLEN;
	}
}

void vga_hline(uint8_t *vmem, int x, int y, int len, uint8_t color)
{
	vmem += y * SCANLEN + x;

	while(len-- > 0) {
		*vmem++ = color;
	}
}

void vga_rect_outline(uint8_t *vmem, int x, int y, int w, int h, uint8_t color)
{
	vga_vline(vmem, x, y, h, color);
	vga_vline(vmem, x + w - 1, y, h, color);
	vga_hline(vmem, x + 1, y, w - 2, color);
	vga_hline(vmem, x + 1, y + h - 1, w - 2, color);
}

static uint32_t *convbuf;
static int convbuf_size;

static int create_glfb(void)
{
#ifndef NO_GLTEX
	tex_xsz = FB_WIDTH;
	tex_ysz = FB_HEIGHT;
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex_xsz, tex_ysz, 0, GL_RGBA,
			GL_UNSIGNED_BYTE, 0);
	if(opt.scaler == SCALER_LINEAR) {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	} else {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	}

	glMatrixMode(GL_TEXTURE);
	glLoadIdentity();
	glScalef((float)FB_WIDTH / tex_xsz, (float)FB_HEIGHT / tex_ysz, 1);
#endif

	if(FB_WIDTH * FB_HEIGHT > convbuf_size) {
		convbuf_size = FB_WIDTH * FB_HEIGHT;
		free(convbuf);
		if(!(convbuf = malloc(convbuf_size * sizeof *convbuf))) {
			return -1;
		}
	}
	return 0;
}

#define PACK_RGB32(r, g, b) \
	((r) | ((uint32_t)g << 8) | ((uint32_t)b << 16))

void vga_pgflip(int wait_vblank)
{
	unsigned int i, j;
	uint32_t *dptr = convbuf;
	uint8_t *sptr = vga_backbuf;
	static int prev_vsync = -1;

	if(wait_vblank != prev_vsync) {
		set_vsync(wait_vblank);
		prev_vsync = wait_vblank;
	}

	sptr = vga_backbuf + (FB_HEIGHT - 1) * SCANLEN;
	for(i=0; i<FB_HEIGHT; i++) {
		for(j=0; j<FB_WIDTH; j++) {
			struct cmap_color *col = fake_cmap + sptr[j];
			*dptr++ = PACK_RGB32(col->r, col->g, col->b);
		}
		sptr -= SCANLEN;
	}

#ifdef NO_GLTEX
	if(xoffs | yoffs) {
		glClear(GL_COLOR_BUFFER_BIT);
	}
	glDrawPixels(FB_WIDTH, FB_HEIGHT, GL_RGBA, GL_UNSIGNED_BYTE, convbuf);
#else
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, FB_WIDTH, FB_HEIGHT, GL_RGBA,
			GL_UNSIGNED_BYTE, convbuf);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	if(win_aspect >= ASPECT) {
		glScalef(ASPECT / win_aspect, 1, 1);
	} else {
		glScalef(1, win_aspect / ASPECT, 1);
	}

	glClear(GL_COLOR_BUFFER_BIT);

	glBegin(GL_QUADS);
	glTexCoord2f(0, 0);
	glVertex2f(-1, -1);
	glTexCoord2f(1, 0);
	glVertex2f(1, -1);
	glTexCoord2f(1, 1);
	glVertex2f(1, 1);
	glTexCoord2f(0, 1);
	glVertex2f(-1, 1);
	glEnd();
#endif

	glutSwapBuffers();
	assert(glGetError() == GL_NO_ERROR);
}


#if defined(__unix__) || defined(unix)
static void set_vsync(int vsync)
{
	if(glx_swap_interval_ext) {
		glx_swap_interval_ext(miniglut_dpy, miniglut_win, vsync);
	} else if(glx_swap_interval_mesa) {
		glx_swap_interval_mesa(vsync);
	}
}
#endif
#ifdef WIN32
static void set_vsync(int vsync)
{
	if(wgl_swap_interval_ext) {
		wgl_swap_interval_ext(vsync);
	}
}
#endif
#ifdef __APPLE__
static void set_vsync(int vsync)
{
	/* TODO */
}
#endif
