#ifndef SOUND_DEFINED
#define SOUND_DEFINED

#include <util/delay.h>
#include "ff.h"

void sound_init();
int play_wav_auto(char* filename);
int play_wav_auto_pgm(char* filename);
int play_wav(char* filename);
int play_wav_pgm(char* filename);
int rec_wav(char* filename);
int sound_read();
int sound_write();
void sound_stop(void);
void beep(int freq, int length);

#endif
