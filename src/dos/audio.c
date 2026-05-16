#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <io.h>
#include "audio.h"

#ifndef NO_SOUND
#define __NO_DEFAULT_LIBS__
#include "muslib/muslib.h"

struct au_music {
	int handle;
};

static int driver;
static struct au_music *cur_mus;
static int volume;


int au_init(void)
{
	unsigned short port = -1, sbport = 0x220, mpu401port = 0x330;
	const char *bankfile = 0;
	int fd;

	MLinit(0);

	MLaddDriver(&OPL2driver);
	MLaddDriver(&OPL3driver);
	MLaddDriver(&MPU401driver);
	MLaddDriver(&SBMIDIdriver);

	if(MLinitTimer(TIMER_CNT140) != 0) {
		fprintf(stderr, "au_init: failed to initialize timer\n");
		return -1;
	}

	MLparseBlaster("A:P", &sbport, &mpu401port);

	printf("Detecting audio hardware... ");
	fflush(stdout);
	if(MLdetectHardware(DRV_OPL3, sbport, -1, -1)) {
		driver = DRV_OPL3;
		bankfile = "data/music/genmidi.op2";
		port = sbport;
		printf("SB/OPL3 port %x\n", port);

	} else if(MLdetectHardware(DRV_OPL2, 0x388, -1, -1)) {
		driver = DRV_OPL2;
		bankfile = "data/music/genmidi.op2";
		port = 0x388;
		printf("Adlib/OPL2\n");

	} else if(MLdetectHardware(DRV_MPU401, mpu401port, -1, -1)) {
		driver = DRV_MPU401;
		port = mpu401port;
		printf("MPU401\n");

	} else {
		putchar('\n');
		fprintf(stderr, "au_init: no audio hardware detected\n");
		MLshutdownTimer();
		return -1;
	}

	if(bankfile) {
		if((fd = open(bankfile, O_RDONLY | O_BINARY)) == -1) {
			fprintf(stderr, "au_init: failed to open bank file: %s\n", bankfile);
			MLshutdownTimer();
			return -1;
		}
		if(MLloadBank(driver, fd, 0) != 0) {
			fprintf(stderr, "au_init: failed to load bank file: %s\n", bankfile);
			MLshutdownTimer();
			close(fd);
			return -1;
		}
		close(fd);
	}

	if(MLinitHardware(driver, port, -1, -1) != 0) {
		fprintf(stderr, "au_init: failed to initialize the hardware\n");
		MLshutdownTimer();
		return -1;
	}

	volume = 256;
	return 0;
}

void au_shutdown(void)
{
	MLshutdownTimer();
	MLdeinit();
}

struct au_music *au_load_music(const char *fname)
{
	int fd;
	struct au_music *mus;

	if((fd = open(fname, O_RDONLY | O_BINARY)) == -1) {
		fprintf(stderr, "au_load_music: failed to open: %s\n", fname);
		return 0;
	}

	if(!(mus = calloc(1, sizeof *mus))) {
		close(fd);
		return 0;
	}

	if((mus->handle = MLallocHandle(driver)) < 0) {
		fprintf(stderr, "au_load_music: failed to allocate music handle\n");
		free(mus);
		close(fd);
		return 0;
	}

	if(MLloadMUS(mus->handle, fd, 1) != 0) {
		fprintf(stderr, "au_load_music: failed to load %s\n", fname);
		MLfreeHandle(mus->handle);
		free(mus);
		close(fd);
		return 0;
	}
	close(fd);

	return mus;
}

void au_free_music(struct au_music *mus)
{
	if(mus) {
		if(mus->handle >= 0) {
			MLfreeMUS(mus->handle);
			MLfreeHandle(mus->handle);
		}
		free(mus);
	}
}


void au_play_music(struct au_music *mus)
{
	MLsetVolume(mus->handle, volume);
	MLsetLoopCount(mus->handle, -1);

	if(MLplay(mus->handle) == 0) {
		cur_mus = mus;
	}
}

void au_stop_music(struct au_music *mus)
{
	if(cur_mus) {
		MLstop(mus->handle);
		cur_mus = 0;
	}
}

struct au_music *au_music_playing(void)
{
	return cur_mus;
}


void au_music_volume(int vol)
{
	volume = vol;
	if(cur_mus) {
		MLsetVolume(cur_mus->handle, vol);
	}
}
#endif
