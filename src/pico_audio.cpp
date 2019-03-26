#include <map>
#include <sstream>

#include "pico_audio.h"

#include "hal_audio.h"
#include "log.h"
#include "pico_cart.h"
#include "pico_core.h"

namespace pico_private {
#pragma pack(1)
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

#pragma pack()

	std::map<int, int> sfx_map;

	void load_wavs() {
		sfx_map[-1] = -1;
		for (int n = 0; n < 64; n++) {
			std::string name = pico_cart::getCart().sections["base_path"] +
			                   pico_cart::getCart().sections["cart_name"] + std::to_string(n) +
			                   ".wav";
			try {
				int id = AUDIO_LoadWav(name.c_str());
				sfx_map[n] = id;
			} catch (audio_exception& e) {
				// logr << "failed to load wav: " << e.what();
			}
		}
	}

	int get_wavid(int sfx_id) {
		auto i = sfx_map.find(sfx_id);
		if (i != sfx_map.end()) {
			return i->second;
		}
		return -1;
	}

}  // namespace pico_private

namespace pico_control {
	void audio_init() {
		TraceFunction();
		AUDIO_StopAll();
		pico_private::sfx_map.clear();
	}

	void set_music_from_cart(std::string& data) {
		TraceFunction();
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
		TraceFunction();
		std::istringstream str(data);
		std::string line;
		pico_private::SFX* sfx_ptr = (pico_private::SFX*)pico_control::get_sfx_data();
		int linenum = 0;
		while (std::getline(str, line)) {
			int o[5] = {0};
			if (sscanf(line.c_str(), "%02x%02x%02x%02x", o, o + 1, o + 2, o + 3) == 4) {
				sfx_ptr->mode = o[0];
				sfx_ptr->speed = o[1];
				sfx_ptr->loopstart = o[2];
				sfx_ptr->loopend = o[3];

				int offset = 8;
				for (int n = 0; n < 32; n++) {
					if (sscanf(line.c_str() + offset, "%01x%01x%01x%01x%01x", o, o + 1, o + 2,
					           o + 3, o + 4) == 5) {
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
			sfx_ptr++;
			linenum++;
		}
	}

	void sound_tick() {
		if (pico_private::sfx_map.empty()) {
			pico_private::load_wavs();
		}
	}

	void stop_all_audio() {
		TraceFunction();
		AUDIO_StopAll();
	}

}  // namespace pico_control

namespace pico_api {

	void sfx(int n) {
		sfx(n, -1);
	}

	void sfx(int n, int channel) {
		if (channel == -1) {
			channel = AUDIO_AvailableChan(true);
		}
		if (n >= 0 && n <= 63) {
			int wavid = pico_private::get_wavid(n);
			if (wavid >= 0) {
				pico_private::SFX* sfx_ptr = (pico_private::SFX*)pico_control::get_sfx_data();
				sfx_ptr += wavid;

				if (sfx_ptr->loopstart == 0 && sfx_ptr->loopend == 0) {
					AUDIO_Play(wavid, channel, false);
				} else {
					int speed = sfx_ptr->speed;
					int lstart = speed * sfx_ptr->loopstart;
					int lend = speed * (sfx_ptr->loopend + 1) - 1;
					AUDIO_Play(wavid, channel, lstart, lend);
				}
			}
		}
		if (n == -1) {
			AUDIO_Stop(channel);
		}
		if (n == -2) {
			AUDIO_StopLoop(channel);
		}
	}

	void sfx(int n, int channel, int offset) {
		sfx(n, channel, offset, 32);
	}

	void sfx(int n, int channel, int offset, int length) {
		if (offset == 0) {
			sfx(n, channel);
		} else {
			if (channel == -1) {
				channel = AUDIO_AvailableChan(true);
			}

			int wavid = pico_private::get_wavid(n);
			if (wavid >= 0) {
				pico_private::SFX* sfx_ptr = (pico_private::SFX*)pico_control::get_sfx_data();
				sfx_ptr += wavid;

				int speed = sfx_ptr->speed;
				int start = speed * offset;
				int end = speed * (offset + length);
				AUDIO_Play(wavid, channel, start, end, false);
			}
		}
	}

	void music(int n) {
	}
	void music(int n, int fadems) {
	}
	void music(int n, int fadems, int channelmask) {
	}

}  // namespace pico_api
