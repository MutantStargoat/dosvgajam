#include <stdlib.h>
#include "audio.h"

struct au_music {
	int foo;
};

static struct au_music *cur_mus;


int au_init(void)
{
	return 0;
}

void au_shutdown(void)
{
}

struct au_music *au_load_music(const char *fname)
{
	struct au_music *mus;

	if(!(mus = calloc(1, sizeof *mus))) {
		return 0;
	}
	return mus;
}

void au_free_music(struct au_music *mus)
{
	free(mus);
}


void au_play_music(struct au_music *mus)
{
	cur_mus = mus;
}

void au_stop_music(struct au_music *mus)
{
	cur_mus = 0;
}

struct au_music *au_music_playing(void)
{
	return cur_mus;
}


void au_music_volume(int vol)
{
}
