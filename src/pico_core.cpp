#include "pico_core.h"

static pixel_t palette[16];
static pixel_t* backbuffer;
static int backbuffer_pitch;
static int buffer_size_x;
static int buffer_size_y;

static pixel_t sprite_data[128][128];

namespace pico_control {
	void init(int x, int y) {
		buffer_size_x = x;
		buffer_size_y = y;

		palette[0] = GFX_GetPixel(0, 0, 0);
		palette[1] = GFX_GetPixel(29, 43, 83);
		palette[2] = GFX_GetPixel(126, 37, 83);
		palette[3] = GFX_GetPixel(0, 135, 81);
		palette[4] = GFX_GetPixel(171, 82, 54);
		palette[5] = GFX_GetPixel(95, 87, 79);
		palette[6] = GFX_GetPixel(194, 195, 199);
		palette[7] = GFX_GetPixel(255, 241, 232);
		palette[8] = GFX_GetPixel(255, 0, 77);
		palette[9] = GFX_GetPixel(255, 163, 0);
		palette[10] = GFX_GetPixel(255, 240, 36);
		palette[11] = GFX_GetPixel(0, 231, 86);
		palette[12] = GFX_GetPixel(41, 173, 255);
		palette[13] = GFX_GetPixel(131, 118, 156);
		palette[14] = GFX_GetPixel(255, 119, 168);
		palette[15] = GFX_GetPixel(255, 204, 170);
	}

	void set_buffer(pixel_t* buffer, int pitch) {
		backbuffer = buffer;
		backbuffer_pitch = pitch;
	}

	void set_sprite_data(std::string data, std::string flags) {
	}

	void set_map_data(std::string data) {
	}

}  // namespace pico_control

namespace pico_api {

	void cls(colour c) {
		pixel_t p = palette[c];
		pixel_t* pix = backbuffer;
		for (int y = 0; y < buffer_size_y; y++) {
			for (int x = 0; x < buffer_size_x; x++) {
				pix[x] = p;
			}
			pix += backbuffer_pitch / sizeof(pixel_t);
		}
	}

	void spr(int n, int x, int y, int w, int h, bool flip_x, bool flip_y) {
	}

	void map(int cell_x, int cell_y, int scr_x, int scr_y, int cell_w, int cell_h, int layer) {
	}

	int btn(int n) {
		return 0;
	}

	int btnp(int n) {
		return 0;
	}
}  // namespace pico_api