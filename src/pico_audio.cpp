#include "pico_audio.h"
#include "hal_audio.h"
#include "pico_core.h"

#include <sstream>

namespace pico_api {

	void sfx(int n) {
	}
	void sfx(int n, int channel) {
	}
	void sfx(int n, int channel, int offset) {
	}
	void sfx(int n, int channel, int offset, int length) {
	}

	void music(int n) {
	}
	void music(int n, int fadems) {
	}
	void music(int n, int fadems, int channelmask) {
	}

}  // namespace pico_api

namespace pico_control {
	void set_music_from_cart(std::string& data) {
		std::istringstream str(data);
		std::string line;
		uint8_t* ptr = pico_control::get_music_data();
		while (std::getline(str, line)) {
			// puts(line.c_str());
			int o[5] = {0};
			if (sscanf(line.c_str(), "%02x %02x%02x%02x%02x", o, o + 1, o + 2, o + 3, o + 4) == 5) {
				for (int n = 0; n < 4; n++) {
					ptr[n] = o[n + 1];
				}
				if (o[0] & 1) {
					ptr[0] |= 0x80;
				}
				if (o[0] & 2) {
					ptr[1] |= 0x80;
				}
				if (o[0] & 4) {
					ptr[2] |= 0x80;
				}
				ptr += 4;
			}
		}
	}

	void set_sfx_from_cart(std::string& data) {
		std::istringstream str(data);
		std::string line;
		while (std::getline(str, line)) {
			// puts(line.c_str());
		}
	}

}  // namespace pico_control