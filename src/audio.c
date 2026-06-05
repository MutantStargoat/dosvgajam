#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "audio.h"
#include "audrv.h"
#include "aufile.h"

#define NUM_TRACKS	4

struct au_sample {
	int rate, bits, chan;
	unsigned long size;
	void *samples;
};

struct track {
	struct au_sample *samp;
	unsigned long offs;
};

static int trk_bits, trk_chan, ntracks;
static struct track track[NUM_TRACKS];

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
	memset(track, 0, sizeof track);

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

int au_pcm_isplaying(void)
{
	return drv.isplaying();
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


struct au_sample *au_load_sample(const char *fname)
{
	struct au_sample *samp = 0;
	struct au_file *file;

	if(!(file = au_open(fname))) {
		fprintf(stderr, "failed to open audio file: %s\n", fname);
		goto err;
	}

	if(!(samp = malloc(sizeof *samp))) {
		fprintf(stderr, "au_load_sample: failed to allocate sample\n");
		goto err;
	}
	if(!(samp->samples = malloc(file->size))) {
		fprintf(stderr, "au_load_sample(\"%s\"): failed to allocate PCM buffer (%ld bytes)\n",
				fname, samp->size);
		goto err;
	}
	samp->rate = file->rate;
	samp->bits = file->bits;
	samp->chan = file->chan;
	samp->size = file->size;

	if(au_read(file, samp->samples, file->size) < file->size) {
		fprintf(stderr, "au_load_sample: EOF while reading from %s\n", fname);
		goto err;
	}
	au_close(file);

	return samp;

err:
	au_free_sample(samp);
	au_close(file);
	return 0;
}

void au_free_sample(struct au_sample *samp)
{
	if(!samp) return;

	free(samp->samples);
	free(samp);
}

static int pcmplay_b8m(void *buf, int size, void *cls)
{
	int i, minsz;
	struct track *trk = 0;
	unsigned char *src, *dst = buf;

	if(!ntracks) {
		memset(buf, 0, size);
		return size;
	}

	/* find the first active track and just copy it */
	for(i=0; i<NUM_TRACKS; i++) {
		if(track[i].samp) {
			trk = track + i;
			break;
		}
	}
	minsz = size < trk->samp->size ? size : trk->samp->size;
	memcpy(buf, (unsigned char*)trk->samp->samples + trk->offs, minsz);
	trk->offs += minsz;
	if(trk->offs >= trk->samp->size) {
		trk->samp = 0;	/* sample finished, release the track */
		ntracks--;
	}

	/* rest of the tracks, add them up */
	while(++trk < track + NUM_TRACKS) {
		if(!trk->samp) continue;

		minsz = size < trk->samp->size ? size : trk->samp->size;
		src = (unsigned char*)trk->samp->samples + trk->offs;
		for(i=0; i<minsz; i++) {
			int val = (int)dst[i] - 128;
			dst[i] = (unsigned char)(val + ((int)src[i] - 128) + 128);
		}
		trk->offs += minsz;
		if(trk->offs >= trk->samp->size) {
			trk->samp = 0;	/* sample finished, release the track */
			ntracks--;
		}
	}
}

int au_start_player(int rate, int bits, int nchan)
{
	if(trk_bits || trk_chan) return -1;
	if(!bits || !nchan || !rate) return -1;

	trk_bits = bits;
	trk_chan = nchan;

	memset(track, 0, sizeof track);
	ntracks = 0;

	switch((bits << 4) | nchan) {
	case 0x81:
		au_pcm_set_callback(pcmplay_b8m, 0);
		break;
	/*case 0x82:
		au_pcm_set_callback(pcmplay_b8s, 0);
		break;
	case 0x101:
		au_pcm_set_callback(pcmplay_b16m, 0);
		break;
	case 0x102:
		au_pcm_set_callback(pcmplay_b16s, 0);
		break;*/
	default:
		fprintf(stderr, "au_start_player: invalid mode requested: %d bits, %d channels\n",
				bits, nchan);
		return -1;
	}

	au_pcm_play(rate, bits, nchan);
	return 0;
}

void au_stop_player(void)
{
	au_pcm_stop();
	trk_bits = trk_chan = 0;
}

/* dummy PCM driver */

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
