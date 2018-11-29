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

namespace pico_control {
	void set_music_from_cart(std::string& data);
	void set_sfx_from_cart(std::string& data);
}  // namespace pico_control

#endif /* PICO_AUDIO_H */
