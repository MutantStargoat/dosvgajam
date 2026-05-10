#ifndef AUDIO_H_
#define AUDIO_H_

struct au_music;

int au_init(void);
void au_shutdown(void);

struct au_music *au_load_music(const char *fname);
void au_free_music(struct au_music *mus);

void au_play_music(struct au_music *mus);
void au_stop_music(struct au_music *mus);
struct au_music *au_music_playing(void);

void au_music_volume(int vol);

#endif	/* AUDIO_H_ */
