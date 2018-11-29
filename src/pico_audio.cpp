#include "pico_audio.h"
#include "hal_audio.h"

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
		while (std::getline(str, line)) {
			puts(line.c_str());
		}
	}

	void set_sfx_from_cart(std::string& data) {
		std::istringstream str(data);
		std::string line;
		while (std::getline(str, line)) {
			puts(line.c_str());
		}
	}

}  // namespace pico_control