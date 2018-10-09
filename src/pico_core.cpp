#include "pico_core.h"
#include "pico_memory.h"

#include <iostream>

static pixel_t* backbuffer;
static int backbuffer_pitch;
static int buffer_size_x;
static int buffer_size_y;
static pixel_t base_palette[16];

struct InputState {
	uint8_t old = 0;
	uint8_t current = 0;
	uint8_t repcnt = 0;

	void set(int state) {
		old = current;
		current = state;

		if (justPressed()) {
			repcnt = 16;
		}

		repcnt--;
		if (repcnt == 0)
			repcnt = 4;
	}

	bool isPressed(uint8_t key) {
		return ((current >> key) & 1) == 1;
	}

	uint8_t isPressed() {
		return current;
	}

	bool justPressed(uint8_t key) {
		return (justPressed() >> key) & 1;
	}

	uint8_t justPressed() {
		return ~old & current;
	}

	bool justPressedRpt(uint8_t key) {
		return justPressed(key) || (isPressed(key) && (repcnt == 1));
	}

	uint8_t justReleased() {
		return old & ~current;
	}

	bool justReleased(uint8_t key) {
		return (justReleased() >> key) & 1;
	}
};

static InputState inputState[4];

struct GraphicsState {
	pico_api::colour_t fg = 7;
	pico_api::colour_t bg = 0;
	uint16_t pattern = 0;
	int text_x = 0;
	int text_y = 0;
	int clip_x1 = 0;
	int clip_y1 = 0;
	int clip_x2 = 128;
	int clip_y2 = 128;
	int camera_x = 0;
	int camera_y = 0;
	pixel_t palette[16];
	bool transparent[16];
};

static GraphicsState graphicsState;
static GraphicsState* currentGraphicsState = &graphicsState;

struct SpriteSheet {
	pico_api::colour_t sprite_data[128 * 128];
	uint8_t flags[256];
};

static SpriteSheet fontSheet;
static SpriteSheet spriteSheet;
static SpriteSheet* currentSprData = &spriteSheet;

struct MapSheet {
	uint8_t map_data[128 * 64];
};

static MapSheet mapSheet;
static MapSheet* currentMapData = &mapSheet;

static pico_ram::RAM ram;
static pico_ram::SplitNibbleMemoryArea mem_gfx(spriteSheet.sprite_data,
                                               pico_ram::MEM_GFX_ADDR,
                                               pico_ram::MEM_GFX_SIZE);
static pico_ram::SplitNibbleMemoryArea mem_gfx2(spriteSheet.sprite_data + 128 * 64,
                                                pico_ram::MEM_GFX2_MAP2_ADDR,
                                                pico_ram::MEM_GFX2_MAP2_SIZE);
static pico_ram::LinearMemoryArea mem_map2(mapSheet.map_data + 128 * 32,
                                           pico_ram::MEM_GFX2_MAP2_ADDR,
                                           pico_ram::MEM_GFX2_MAP2_SIZE);
static pico_ram::DualMemoryArea mem_gfx2_map2(&mem_map2,
                                              &mem_gfx2);  // shared memory between gfx2 & map2
static pico_ram::LinearMemoryArea mem_map(mapSheet.map_data,
                                          pico_ram::MEM_MAP_ADDR,
                                          pico_ram::MEM_MAP_SIZE);
static pico_ram::LinearMemoryArea mem_flags(spriteSheet.flags,
                                            pico_ram::MEM_GFX_PROPS_ADDR,
                                            pico_ram::MEM_GFX_PROPS_SIZE);

// static pico_ram::SplitNibbleMemoryArea mem_screen();

static pico_ram::SplitNibbleMemoryArea mem_font(fontSheet.sprite_data, 0x8000, 0x2000);

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
		clip_axis(scr_x, spr_x, w, currentGraphicsState->clip_x1, currentGraphicsState->clip_x2);
		clip_axis(scr_y, spr_y, h, currentGraphicsState->clip_y1, currentGraphicsState->clip_y2);

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

	static int clip_rect(int& x0, int& y0, int& x1, int& y1) {
		int flags = 0;

		if (x0 < currentGraphicsState->clip_x1) {
			x0 = currentGraphicsState->clip_x1;
			flags |= 1;
		}
		if (y0 < currentGraphicsState->clip_y1) {
			y0 = currentGraphicsState->clip_y1;
			flags |= 2;
		}
		if (x1 >= currentGraphicsState->clip_x2) {
			x1 = currentGraphicsState->clip_x2 - 1;
			flags |= 4;
		}
		if (y1 >= currentGraphicsState->clip_y2) {
			y1 = currentGraphicsState->clip_y2 - 1;
			flags |= 8;
		}
		return 0;
	}

	template <typename T>
	T limit(T n, T min, T max) {
		if (n < min)
			return min;
		if (n > max)
			return max;
		return n;
	}

	void hline(int x0, int x1, int y, colour_t c) {
		if (y < 0 || y > 127) {
			return;
		}
		x0 = limit(x0, 0, 127);
		x1 = limit(x1, 0, 127);

		pixel_t* pix = backbuffer + y * backbuffer_pitch / sizeof(pixel_t);
		pixel_t p = currentGraphicsState->palette[c & 0x0f];

		for (int x = x0; x <= x1; x++) {
			pix[x] = p;
		}
	}

	void vline(int y0, int y1, int x, colour_t c) {
		if (x < 0 || x > 127) {
			return;
		}
		y0 = limit(y0, 0, 127);
		y1 = limit(y1, 0, 127);

		pixel_t* pix = backbuffer + y0 * backbuffer_pitch / sizeof(pixel_t);
		pixel_t p = currentGraphicsState->palette[c & 0x0f];

		for (int y = y0; y <= y1; y++) {
			pix[x] = p;
			pix += backbuffer_pitch / sizeof(pixel_t);
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

		ram.addMemoryArea(&mem_gfx);
		ram.addMemoryArea(&mem_gfx2_map2);
		ram.addMemoryArea(&mem_map);
		ram.addMemoryArea(&mem_flags);
		ram.addMemoryArea(&mem_font);
	}

	void set_buffer(pixel_t* buffer, int pitch) {
		backbuffer = buffer;
		backbuffer_pitch = pitch;
	}

	void copy_data_to_ram(uint16_t addr, const std::string& data, bool gfx = true) {
		for (size_t n = 0; n < data.length(); n++) {
			char buf[3] = {0};

			if (data[n] > ' ') {
				if (gfx) {
					buf[1] = data[n++];
					buf[0] = data[n];
				} else {
					buf[0] = data[n++];
					buf[1] = data[n];
				}
				pico_api::poke(addr++, strtol(buf, nullptr, 16));
			}
		}
	}

	void set_sprite_data(std::string data, std::string flags) {
		copy_data_to_ram(pico_ram::MEM_GFX_ADDR, data);
		copy_data_to_ram(pico_ram::MEM_GFX_PROPS_ADDR, flags, false);
	}

	void set_font_data(std::string data) {
		copy_data_to_ram(0x8000, data);
	}

	void set_map_data(std::string data) {
		copy_data_to_ram(pico_ram::MEM_MAP_ADDR, data, false);
		// ram.dump(0x0000, 0x4000);
	}

	void set_input_state(int state, int player) {
		inputState[player].set(state);
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

	uint8_t peek(uint16_t a) {
		return ram.peek(a);
	}

	uint32_t peek4(uint16_t a) {
		return 0;
	}

	void poke(uint16_t a, uint8_t v) {
		ram.poke(a, v);
	}

	void poke4(uint16_t a, uint32_t v) {
	}

	uint32_t dget(uint16_t a) {
		return 0;
	}

	void dset(uint16_t a, uint32_t v) {
	}

	void cls() {
		cls(0);
	}

	uint8_t fget(int n) {
		return currentSprData->flags[n & 0xff];
	}

	bool fget(int n, int bit) {
		return (fget(n) >> bit) & 1;
	}

	void fset(int n, uint8_t val) {
		currentSprData->flags[n & 0xff] = val;
	}

	void fset(int n, int bit, bool val) {
		if (val)
			fset(n, fget(n) | (1 << bit));
		else
			fset(n, fget(n) & ~(1 << bit));
	}

	void spr(int n, int x, int y) {
		spr(n, x, y, 1, 1, false, false);
	}

	void spr(int n, int x, int y, int w, int h) {
		spr(n, x, y, w, h, false, false);
	}

	void spr(int n, int x, int y, int w, int h, bool flip_x, bool flip_y) {
		int spr_x = (n % 16) * 8;
		int spr_y = (n / 16) * 8;
		pico_private::blitter(*currentSprData, x, y, spr_x, spr_y, w * 8, h * 8);
	}

	void sspr(int sx, int sy, int sw, int sh, int dx, int dy) {
		pico_private::blitter(*currentSprData, dx, dy, sx, sy, sw, sh);
	}

	colour_t sget(int x, int y) {
		y &= 0x7f;
		x &= 0x7f;
		return currentSprData->sprite_data[y * 128 + x];
	}

	void sset(int x, int y) {
		sset(x, y, currentGraphicsState->fg);
	}

	void sset(int x, int y, colour_t c) {
		y &= 0x7f;
		x &= 0x7f;
		currentSprData->sprite_data[y * 128 + x] = c;
	}

	void pset(int x, int y) {
		pset(x, y, currentGraphicsState->fg);
	}

	void pset(int x, int y, colour_t colour) {
		if (x < currentGraphicsState->clip_x1 || x >= currentGraphicsState->clip_x2 ||
		    y < currentGraphicsState->clip_y1 || y >= currentGraphicsState->clip_y2) {
			return;
		}

		pixel_t* pix = backbuffer + y * backbuffer_pitch / sizeof(pixel_t) + x;
		*pix = currentGraphicsState->palette[colour & 0x0f];
	}

	void rect(int x0, int y0, int x1, int y1) {
		rect(x0, y0, x1, y1, currentGraphicsState->fg);
	}

	void rect(int x0, int y0, int x1, int y1, colour_t c) {
		pico_private::hline(x0, x1, y0, c);
		pico_private::hline(x0, x1, y1, c);
		pico_private::vline(y0, y1, x0, c);
		pico_private::vline(y0, y1, x1, c);
	}

	void rectfill(int x0, int y0, int x1, int y1) {
		rectfill(x0, y0, x1, y1, currentGraphicsState->fg);
	}

	void rectfill(int x0, int y0, int x1, int y1, colour_t c) {
		pico_private::clip_rect(x0, y0, x1, y1);
		pixel_t* pix = backbuffer + y0 * backbuffer_pitch / sizeof(pixel_t);
		pixel_t p1 = currentGraphicsState->palette[c & 0x0f];
		pixel_t p2 = currentGraphicsState->palette[(c >> 4) & 0x0f];

		uint16_t pat = currentGraphicsState->pattern;

		for (int y = y0; y <= y1; y++) {
			for (int x = x0; x <= x1; x++) {
				pix[x] = ((pat >> ((3 - (x & 0x3)) + (3 - (y & 0x3)) * 4)) & 1) ? p2 : p1;
			}
			pix += backbuffer_pitch / sizeof(pixel_t);
		}
	}

	void circ(int x, int y, int r) {
		circ(x, y, r, currentGraphicsState->fg);
	}

	void circ(int x, int y, int r, colour_t c) {
	}

	void circfill(int x, int y, int r) {
		circfill(x, y, r, currentGraphicsState->fg);
	}

	void circfill(int x, int y, int r, colour_t c) {
		rectfill(x, y, x + r, y + r, c);
	}

	void line(int x0, int y0, int x1, int y1) {
		line(x0, y0, x1, y1, currentGraphicsState->fg);
	}

	void line(int x0, int y0, int x1, int y1, colour_t c) {
	}

	void map(int cell_x, int cell_y) {
		map(cell_x, cell_y, 0, 0);
	}

	void map(int cell_x, int cell_y, int scr_x, int scr_y) {
		map(cell_x, cell_y, scr_x, scr_y, 16, 16);
	}

	void map(int cell_x, int cell_y, int scr_x, int scr_y, int cell_w, int cell_h) {
		map(cell_x, cell_y, scr_x, scr_y, cell_w, cell_h, 0);
	}

	void map(int cell_x, int cell_y, int scr_x, int scr_y, int cell_w, int cell_h, uint8_t layer) {
		for (int y = 0; y < cell_h; y++) {
			for (int x = 0; x < cell_w; x++) {
				uint8_t cell = mget(cell_x + x, cell_y + y);
				if (cell && ((layer == 0) || ((fget(cell) & layer) == layer))) {
					spr(cell, scr_x + x * 8, scr_y + y * 8);
				}
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
		currentGraphicsState->palette[c0 & 0xf] = base_palette[c1 & 0xf];
	}

	void pal() {
		pico_private::restore_palette();
		pico_private::restore_transparency();
	}

	void palt(colour_t col, bool t) {
		currentGraphicsState->transparent[col] = t;
	}

	void palt() {
		pico_private::restore_transparency();
	}

	void color(uint8_t c) {
		currentGraphicsState->fg = c & 0xf;
		currentGraphicsState->bg = c >> 4;
	}

	void print(std::string str) {
		print(str, currentGraphicsState->text_x, currentGraphicsState->text_y);
	}

	void print(std::string str, int x, int y) {
		print(str, x, y, currentGraphicsState->fg);
	}

	void print(std::string str, int x, int y, colour_t c) {
		pixel_t old = currentGraphicsState->palette[7];
		bool oldt = currentGraphicsState->transparent[0];

		currentGraphicsState->palette[7] = base_palette[c & 0xf];
		currentGraphicsState->transparent[0] = true;

		for (size_t n = 0; n < str.length(); n++) {
			uint8_t ch = str[n];
			if (ch >= 0x20 && ch < 0x80) {
				int index = ch - 32;
				pico_private::blitter(fontSheet, x, y, (index % 32) * 4, (index / 32) * 6, 4, 5);
				x += 4;
			} else if (ch >= 0x80 && ch <= 0x99) {
				int index = ch - 0x80;
				pico_private::blitter(fontSheet, x, y, (index % 16) * 8, (index / 16) * 6 + 18, 8,
				                      5);
				x += 8;
			} else if (ch == '\n') {
				x = 0;
				y += 6;
			}
		}

		currentGraphicsState->text_x = 0;
		currentGraphicsState->text_y = y + 6;

		currentGraphicsState->palette[7] = old;
		currentGraphicsState->transparent[0] = oldt;

		currentGraphicsState->fg = c & 0xf;
	}

	int btn() {
		return inputState[0].current;
	}

	int btn(int n, int player) {
		return inputState[player].isPressed(n);
	}

	int btnp() {
		return inputState[0].justPressed();  // TODO: impl repeat on this
	}

	int btnp(int n, int player) {
		return inputState[player].justPressedRpt(n);
	}

	void clip(int x, int y, int w, int h) {
		currentGraphicsState->clip_x1 = x;
		currentGraphicsState->clip_y1 = y;
		currentGraphicsState->clip_x2 = x + w;
		currentGraphicsState->clip_y2 = y + h;
	}

	void clip() {
		clip(0, 0, 128, 128);
	}

	void camera() {
		camera(0, 0);
	}

	void camera(int x, int y) {
		currentGraphicsState->camera_x = x;
		currentGraphicsState->camera_y = y;
	}

	void fillp() {
		fillp(0);
	}

	void fillp(int pattern) {
		currentGraphicsState->pattern = pattern;
	}

	int stat(int key, std::string& sval, int& ival) {
		ival = 0;
		return 2;
	}

}  // namespace pico_api
