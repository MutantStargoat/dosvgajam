#ifndef AUDIO_H_
#define AUDIO_H_

/* mixer channels */
enum {
	AU_DEFAULT,
	AU_MASTER,
	AU_PCM,
	AU_MUSIC
};

typedef int (*au_pcm_callback_func)(void *buffer, int size, void *cls);

struct au_sample;
struct au_music;

int au_init(void);
void au_shutdown(void);

/* PCM playback */
struct au_sample *au_load_sample(const char *fname);
void au_free_sample(struct au_sample *samp);

int au_start_player(int rate, int bits, int nchan);
void au_stop_player(void);

/* returns playback track number, or -1 on failure */
int au_play_sample(struct au_sample *samp);
void au_stop_sample(struct au_sample *samp);
/* returns number of active samples */
int au_sample_playing(void);

void au_pcm_play(int rate, int bits, int nchan);
void au_pcm_pause(void);
void au_pcm_resume(void);
void au_pcm_stop(void);
int au_pcm_playing(void);

/* MIDI playback */

struct au_music *au_load_music(const char *fname);
void au_free_music(struct au_music *mus);

void au_play_music(struct au_music *mus);
void au_stop_music(struct au_music *mus);
struct au_music *au_music_playing(void);

/* mixer */

/* audio volume: 0-255 */
void au_setvolume(int ctl, int vol);
int au_getvolume(int ctl);

#endif	/* AUDIO_H_ */
