#ifndef SDL_AUDIO_H
#define SDL_AUDIO_H

#include <stdint.h>
#include <stdexcept>

struct audio_exception : public std::runtime_error {
	using std::runtime_error::runtime_error;
};

void AUDIO_Init();
void AUDIO_Shutdown();

int AUDIO_LoadWav(const char* name, bool trim = true);
void AUDIO_Play(int id, int chan, bool loop);
void AUDIO_Play(int id, int chan, int start, int end, bool loop);
void AUDIO_Play(int id, int chan, int loop_start, int loop_end);
void AUDIO_StopAll();
void AUDIO_Stop(int chan);
void AUDIO_StopLoop(int chan);
bool AUDIO_isPlaying(int chan);
int AUDIO_AvailableChan(bool force = false);

#endif /* SDL_AUDIO_H */
