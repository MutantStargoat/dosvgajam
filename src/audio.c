#include <stdio.h>
#include "audio.h"
#include "audrv.h"

static struct audrv drv;

static au_pcm_callback_func cbfunc;
static void *cbcls;

int au_music_init(void);
void au_music_shutdown(void);
void au_music_volume(int vol);
int au_music_getvolume(void);

#ifdef MSDOS
int sb_detect(struct audrv *drv);	/* in src/dos/au_sb.c */
#endif

static void dummy_start(int rate, int bits, int nchan);
static void dummy_pause(void);
static void dummy_cont(void);
static void dummy_stop(void);
static int dummy_isplaying(void);
static void dummy_setvolume(int ctl, int vol);
static int dummy_getvolume(int ctl);

static struct audrv drv_dummy = {
	dummy_start, dummy_pause, dummy_cont, dummy_stop,
	dummy_setvolume, dummy_getvolume, dummy_isplaying
};

int au_init(void)
{
#ifdef MSDOS
	if(sb_detect(&drv)) goto musinit;
#endif

	fprintf(stderr, "No supported PCM audio device detected\n");
	drv = drv_dummy;

musinit:
	au_music_init();
	return 0;
}

void au_shutdown(void)
{
	au_music_shutdown();
}

void au_pcm_set_callback(au_pcm_callback_func func, void *cls)
{
	cbfunc = func;
	cbcls = cls;
}

int au_pcm_callback(void *buf, int sz)
{
	if(!cbfunc) {
		return 0;
	}
	return cbfunc(buf, sz, cbcls);
}

void au_pcm_play(int rate, int bits, int nchan)
{
	printf("play %d samples/s, %d bits, %s\n", rate, bits, nchan == 1 ? "mono" : "stereo");
	drv.start(rate, bits, nchan);
}

void au_pcm_pause(void)
{
	drv.pause();
}

void au_pcm_resume(void)
{
	drv.cont();
}

void au_pcm_stop(void)
{
	drv.stop();
}

void au_setvolume(int ctl, int vol)
{
	if(ctl == AU_MUSIC) {
		au_music_volume(vol);
	} else {
		drv.setvolume(ctl, vol);
	}
}

int au_getvolume(int ctl)
{
	if(ctl == AU_MUSIC) {
		return au_music_getvolume();
	}
	return drv.getvolume(ctl);
}

int au_pcm_isplaying(void)
{
	return drv.isplaying();
}


static void dummy_start(int rate, int bits, int nchan)
{
}

static void dummy_pause(void)
{
}

static void dummy_cont(void)
{
}

static void dummy_stop(void)
{
}

static int dummy_isplaying(void)
{
	return 0;
}

static int dummy_vol[4] = {255, 255, 255, 255};

static void dummy_setvolume(int ctl, int vol)
{
	dummy_vol[ctl] = vol;
}

static int dummy_getvolume(int ctl)
{
	return dummy_vol[ctl];
}
