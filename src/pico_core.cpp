#include "pico_core.h"

static pixel_t* backbuffer;
static int backbuffer_pitch;
static int buffer_size_x;
static int buffer_size_y;
static pixel_t base_palette[16];
static int input_state[4] = {0, 0, 0, 0};

struct GraphicsState {
	pico_api::colour_t fg = 7;
	pico_api::colour_t bg = 0;
	int text_x = 0;
	int text_y = 0;
	pixel_t palette[16];
	bool transparent[16];
};

static GraphicsState graphicsState;
static GraphicsState* currentGraphicsState = &graphicsState;

struct SpriteSheet {
	pico_api::colour_t sprite_data[128 * 128];
	uint8_t flags;
};

static SpriteSheet fontSheet;
static SpriteSheet spriteSheet;
static SpriteSheet* currentSprData = &spriteSheet;

struct MapSheet {
	uint8_t map_data[128 * 64];
};

static MapSheet mapSheet;
static MapSheet* currentMapData = &mapSheet;

namespace pico_private {

	using namespace pico_api;

	static void restore_palette() {
		for (size_t n = 0; n < 16; n++) {
			currentGraphicsState->palette[n] = base_palette[n];
		}
	}

	static void restore_transparency() {
		for (size_t n = 0; n < 16; n++) {
			currentGraphicsState->transparent[n] = false;
		}
		currentGraphicsState->transparent[0] = true;
	}

	static void clip_axis(int& dest_pos, int& src_pos, int& len, int min, int max) {
		if (dest_pos < min) {
			len = len - (min - dest_pos);
			src_pos += (min - dest_pos);
			dest_pos = min;
		}

		if ((dest_pos + len) > max) {
			len = len - (dest_pos + len - max);
		}

		if (len < 0) {
			len = 0;
		}
	}

	static void
	blitter(SpriteSheet& sprites, int scr_x, int scr_y, int spr_x, int spr_y, int w, int h) {
		clip_axis(scr_x, spr_x, w, 0, 128);
		clip_axis(scr_y, spr_y, h, 0, 128);

		pixel_t* pix = backbuffer + scr_y * backbuffer_pitch / sizeof(pixel_t) + scr_x;
		colour_t* spr = sprites.sprite_data + spr_y * 128 + spr_x;
		for (int y = 0; y < h; y++) {
			for (int x = 0; x < w; x++) {
				colour_t c = spr[x];
				if (!currentGraphicsState->transparent[c]) {
					pix[x] = currentGraphicsState->palette[c];
				}
			}
			pix += backbuffer_pitch / sizeof(pixel_t);
			spr += 128;
		}
	}
}  // namespace pico_private

namespace pico_control {

	void init(int x, int y) {
		buffer_size_x = x;
		buffer_size_y = y;

		base_palette[0] = GFX_GetPixel(0, 0, 0);
		base_palette[1] = GFX_GetPixel(29, 43, 83);
		base_palette[2] = GFX_GetPixel(126, 37, 83);
		base_palette[3] = GFX_GetPixel(0, 135, 81);
		base_palette[4] = GFX_GetPixel(171, 82, 54);
		base_palette[5] = GFX_GetPixel(95, 87, 79);
		base_palette[6] = GFX_GetPixel(194, 195, 199);
		base_palette[7] = GFX_GetPixel(255, 241, 232);
		base_palette[8] = GFX_GetPixel(255, 0, 77);
		base_palette[9] = GFX_GetPixel(255, 163, 0);
		base_palette[10] = GFX_GetPixel(255, 240, 36);
		base_palette[11] = GFX_GetPixel(0, 231, 86);
		base_palette[12] = GFX_GetPixel(41, 173, 255);
		base_palette[13] = GFX_GetPixel(131, 118, 156);
		base_palette[14] = GFX_GetPixel(255, 119, 168);
		base_palette[15] = GFX_GetPixel(255, 204, 170);

		pico_private::restore_palette();
		pico_private::restore_transparency();
	}

	void set_buffer(pixel_t* buffer, int pitch) {
		backbuffer = buffer;
		backbuffer_pitch = pitch;
	}

	void set_sprite_data(std::string data, std::string flags) {
		size_t i = 0;
		for (size_t n = 0; n < data.length(); n++) {
			char buf[2] = {0};

			if (data[n] > ' ') {
				buf[0] = data[n];
				currentSprData->sprite_data[i++] = strtol(buf, nullptr, 16);
			}
		}
	}

	void set_font_data(std::string data) {
		SpriteSheet* old = currentSprData;
		currentSprData = &fontSheet;
		set_sprite_data(data, "");
		currentSprData = old;
	}

	void set_map_data(std::string data) {
		size_t i = 0;
		for (size_t n = 0; n < data.length(); n++) {
			char buf[3] = {0};

			if (data[n] > ' ') {
				buf[0] = data[n++];
				buf[1] = data[n];
				currentMapData->map_data[i++] = strtol(buf, nullptr, 16);
			}
		}
	}

	void set_input_state(int state, int player) {
		input_state[player] = state;
	}

}  // namespace pico_control

namespace pico_api {

	void cls(colour_t c) {
		pixel_t p = currentGraphicsState->palette[c];
		pixel_t* pix = backbuffer;
		for (int y = 0; y < buffer_size_y; y++) {
			for (int x = 0; x < buffer_size_x; x++) {
				pix[x] = p;
			}
			pix += backbuffer_pitch / sizeof(pixel_t);
		}
		currentGraphicsState->text_x = 0;
		currentGraphicsState->text_y = 0;
	}

	void cls() {
		cls(0);
	}

	void spr(int n, int x, int y, int w, int h, bool flip_x, bool flip_y) {
		int spr_x = (n % 16) * 8;
		int spr_y = (n / 16) * 8;
		pico_private::blitter(*currentSprData, x, y, spr_x, spr_y, w * 8, h * 8);
	}

	void map(int cell_x, int cell_y, int scr_x, int scr_y, int cell_w, int cell_h, int layer) {
		for (int y = 0; y < cell_h; y++) {
			for (int x = 0; x < cell_w; x++) {
				uint8_t cell = mget(cell_x + x, cell_y + y);
				spr(cell, scr_x + x * 8, scr_y + y * 8);
			}
		}
	}

	uint8_t mget(int x, int y) {
		return currentMapData->map_data[y * 128 + x];
	}

	void mset(int x, int y, uint8_t v) {
		currentMapData->map_data[y * 128 + x] = v;
	}

	void pal(colour_t c0, colour_t c1) {
		if (c0 == 255) {
			pico_private::restore_palette();
			pico_private::restore_transparency();
			return;
		}
		currentGraphicsState->palette[c0] = base_palette[c1];
		currentGraphicsState->bg = 0;
		currentGraphicsState->fg = 7;
	}

	void palt(colour_t col, bool t) {
		if (col == 255) {
			pico_private::restore_transparency();
			return;
		}
		currentGraphicsState->transparent[col] = t;
	}

	void print(std::string str, int x, int y, colour_t c) {
		if (x == INT32_MAX) {
			x = currentGraphicsState->text_x;
		}
		if (y == INT32_MAX) {
			y = currentGraphicsState->text_y;
		}
		if (c == UINT8_MAX) {
			c = currentGraphicsState->fg;
		}

		for (size_t n = 0; n < str.length(); n++) {
			char ch = str[n];
			if (ch >= ' ') {
				int index = ch - 32;
				pico_private::blitter(fontSheet, x, y, (index % 32) * 4, (index / 32) * 6, 4, 5);
				x += 4;
			} else if (ch == '\n') {
				x = 0;
				y += 6;
			}
		}

		currentGraphicsState->text_x = 0;
		currentGraphicsState->text_y = y + 6;
	}

	int btn(int n, int player) {
		return (input_state[player] >> n) & 1;
	}

	int btnp(int n, int player) {
		return (input_state[player] >> n) & 1;
	}
}  // namespace pico_api
