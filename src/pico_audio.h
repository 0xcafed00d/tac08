#ifndef PICO_AUDIO_H
#define PICO_AUDIO_H

#include "hal_core.h"

namespace pico_api {
	void sfx(int n);
	void sfx(int n, int channel);
	void sfx(int n, int channel, int offset);
	void sfx(int n, int channel, int offset, int length);

	void music(int n);
	void music(int n, int fadems);
	void music(int n, int fadems, int channelmask);

}  // namespace pico_api

#endif /* PICO_AUDIO_H */
