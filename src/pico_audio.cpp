#include "pico_audio.h"
#include "hal_audio.h"
#include "pico_core.h"

#include <sstream>

namespace pico_private {

	struct Note {
		union {
			uint8_t b1;
			struct {
				uint8_t pitch : 6;
				uint8_t w1 : 1;
				uint8_t w2 : 1;
			};
		};
		union {
			uint8_t b2;
			struct {
				uint8_t w3 : 1;
				uint8_t volume : 3;
				uint8_t effect : 3;
				uint8_t c : 1;
			};
		};
	};

	struct SFX {
		Note notes[32];
		uint8_t mode;
		uint8_t speed;
		uint8_t loopstart;
		uint8_t loopend;
	};
}  // namespace pico_private

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
		pico_private::SFX* sfx_ptr = (pico_private::SFX*)pico_control::get_sfx_data();
		while (std::getline(str, line)) {
			int o[5] = {0};
			sscanf(line.c_str(), "%02x%02x%02x%02x", o, o + 1, o + 2, o + 3);
			sfx_ptr->mode = o[0];
			sfx_ptr->speed = o[1];
			sfx_ptr->loopstart = o[2];
			sfx_ptr->loopend = o[3];

			int offset = 8;
			for (int n = 0; n < 32; n++) {
				sscanf(line.c_str() + offset, "%01x%01x%01x%01x%01x", o, o + 1, o + 2, o + 3,
				       o + 4);

				sfx_ptr->notes[n].pitch = (o[0] << 4) | o[1];
				sfx_ptr->notes[n].volume = o[3];
				sfx_ptr->notes[n].effect = o[4];

				sfx_ptr->notes[n].w1 = o[2];
				sfx_ptr->notes[n].w2 = o[2] >> 1;
				sfx_ptr->notes[n].w3 = o[2] >> 2;
				sfx_ptr->notes[n].c = o[2] >> 3;

				offset += 5;
			}
		}
	}

}  // namespace pico_control
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
