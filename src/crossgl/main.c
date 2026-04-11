#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <assert.h>
#ifndef __APPLE__
#include "miniglut.h"
#else
#include <GLUT/glut.h>
#endif
#include "app.h"
#include "timer.h"
#include "options.h"

static void display(void);
static void idle(void);
static void reshape(int x, int y);
static void keydown(unsigned char key, int x, int y);
static void keyup(unsigned char key, int x, int y);
static void skeydown(int key, int x, int y);
static void skeyup(int key, int x, int y);
static int translate_special(int skey);
static void mouse_button(int bn, int st, int x, int y);
static void mouse_motion(int x, int y);
static void set_fullscreen(int fs);

static unsigned int num_pressed;
static unsigned char keystate[256];

static unsigned long start_time;

int win_width, win_height;
float win_aspect;


int main(int argc, char **argv)
{
	glutInit(&argc, argv);

	/* constrain default scale factor on low-res */
	if(glutGet(GLUT_SCREEN_HEIGHT) <= 960 && opt.scale > 2) {
		opt.scale = 2;
	} else if(glutGet(GLUT_SCREEN_HEIGHT) <= 1024 && opt.scale > 3) {
		opt.scale = 3;
	}

	glutInitWindowSize(320 * opt.scale, 240 * opt.scale);

	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);
	glutCreateWindow("Mutant Stargoat");

	glutDisplayFunc(display);
	glutIdleFunc(idle);
	glutReshapeFunc(reshape);
	glutKeyboardFunc(keydown);
	glutKeyboardUpFunc(keyup);
	glutSpecialFunc(skeydown);
	glutSpecialUpFunc(skeyup);
	glutMouseFunc(mouse_button);
	glutMotionFunc(mouse_motion);

	/*glutSetCursor(GLUT_CURSOR_NONE);*/

	glDisable(GL_DITHER);
#ifndef NO_GLTEX
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_CULL_FACE);
#endif

	time_msec = 0;

	if(opt.fullscreen) {
		set_fullscreen(opt.fullscreen);
		reshape(glutGet(GLUT_SCREEN_WIDTH), glutGet(GLUT_SCREEN_HEIGHT));
	} else {
		reshape(glutGet(GLUT_WINDOW_WIDTH), glutGet(GLUT_WINDOW_HEIGHT));
	}

	if(app_init() == -1) {
		return 1;
	}
	atexit(app_shutdown);

	glutMainLoop();
	return 0;
}

void app_quit(void)
{
	exit(0);
}

void wait_vsync(void)
{
}

int kb_isdown(int key)
{
	switch(key) {
	case KEY_ANY:
		return num_pressed;

	case KEY_ALT:
		return keystate[KEY_LALT] + keystate[KEY_RALT];

	case KEY_CTRL:
		return keystate[KEY_LCTRL] + keystate[KEY_RCTRL];
	}

	if(isalpha(key)) {
		key = tolower(key);
	}
	return keystate[key];
}

/* timer */
void init_timer(int res_hz)
{
}

void reset_timer(void)
{
	start_time = glutGet(GLUT_ELAPSED_TIME);
	time_msec = get_msec();
}

unsigned long get_msec(void)
{
	return glutGet(GLUT_ELAPSED_TIME) - start_time;
}

#ifdef _WIN32
#include <windows.h>

void sleep_msec(unsigned long msec)
{
	Sleep(msec);
}

#else
#include <unistd.h>

void sleep_msec(unsigned long msec)
{
	usleep(msec * 1000);
}
#endif

void app_abort(void)
{
	abort();
}

static void display(void)
{
	time_msec = get_msec();
	app_display();
}

static void idle(void)
{
	glutPostRedisplay();
}

static void reshape(int x, int y)
{
#ifdef NO_GLTEX
	float xzoom = (float)x / FB_WIDTH;
	float yzoom = (float)y / FB_HEIGHT;
#endif

	win_width = x;
	win_height = y;
	win_aspect = (float)x / (float)y;
	glViewport(0, 0, x, y);

#ifdef NO_GLTEX
	if(yzoom < xzoom) {
		xzoom = yzoom;
		xoffs = (x - (int)(FB_WIDTH * yzoom)) >> 1;
		yoffs = 0;
	} else {
		xoffs = 0;
		yoffs = (y - (int)(FB_HEIGHT * xzoom)) >> 1;
	}
	glPixelZoom(xzoom, xzoom);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, x, 0, y, -1, 1);
	glRasterPos2i(xoffs, yoffs);
#endif
}

static void keydown(unsigned char key, int x, int y)
{
	modkeys = glutGetModifiers();

	if((key == '\n' || key == '\r') && (modkeys & GLUT_ACTIVE_ALT)) {
		opt.fullscreen ^= 1;
		set_fullscreen(opt.fullscreen);
		return;
	}
	keystate[key] = 1;
	app_keyboard(key, 1);
}

static void keyup(unsigned char key, int x, int y)
{
	keystate[key] = 0;
	app_keyboard(key, 0);
}

static void skeydown(int key, int x, int y)
{
#ifndef NO_GLTEX
	if(key == GLUT_KEY_F5) {
		opt.scaler = (opt.scaler + 1) % NUM_SCALERS;

		if(opt.scaler == SCALER_LINEAR) {
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		} else {
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		}
	}
#endif
	key = translate_special(key);
	keystate[key] = 1;
	app_keyboard(key, 1);
}

static void skeyup(int key, int x, int y)
{
	key = translate_special(key);
	keystate[key] = 0;
	app_keyboard(key, 0);
}

static int translate_special(int skey)
{
	switch(skey) {
	case 127:
		return 127;
	case GLUT_KEY_LEFT:
		return KEY_LEFT;
	case GLUT_KEY_RIGHT:
		return KEY_RIGHT;
	case GLUT_KEY_UP:
		return KEY_UP;
	case GLUT_KEY_DOWN:
		return KEY_DOWN;
	case GLUT_KEY_PAGE_UP:
		return KEY_PGUP;
	case GLUT_KEY_PAGE_DOWN:
		return KEY_PGDN;
	case GLUT_KEY_HOME:
		return KEY_HOME;
	case GLUT_KEY_END:
		return KEY_END;
	default:
		if(skey >= GLUT_KEY_F1 && skey <= GLUT_KEY_F12) {
			return KEY_F1 + skey - GLUT_KEY_F1;
		}
	}
	return 0;
}

static void map_mouse_pos(int *xp, int *yp)
{
	int x = *xp;
	int y = *yp;

	/* TODO */
	*xp = x * FB_WIDTH / win_width;
	*yp = y * FB_HEIGHT / win_height;
}

static void mouse_button(int bn, int st, int x, int y)
{
	int bit;

	map_mouse_pos(&x, &y);
	mouse_x = x;
	mouse_y = y;

	switch(bn) {
	case GLUT_LEFT_BUTTON:
		bit = 0;
		break;
	case GLUT_RIGHT_BUTTON:
		bit = 1;
		break;
	case GLUT_MIDDLE_BUTTON:
		bit = 2;
		break;
	}

	if(st == GLUT_DOWN) {
		mouse_bnstate |= 1 << bit;
	} else {
		mouse_bnstate &= ~(1 << bit);
	}

	app_mouse(bit, st == GLUT_DOWN, x, y);
}

static void mouse_motion(int x, int y)
{
	map_mouse_pos(&x, &y);
	mouse_x = x;
	mouse_y = y;

	app_motion(x, y);
}

static void set_fullscreen(int fs)
{
	static int win_x, win_y;

	if(fs) {
		win_x = glutGet(GLUT_WINDOW_WIDTH);
		win_y = glutGet(GLUT_WINDOW_HEIGHT);
		glutFullScreen();
	} else {
		glutReshapeWindow(win_x, win_y);
	}
}
