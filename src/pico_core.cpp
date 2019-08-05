#include <assert.h>
#include <string.h>
#include <algorithm>
#include <array>

#include "pico_core.h"

#include "config.h"
#include "log.h"
#include "pico_audio.h"
#include "pico_cart.h"
#include "pico_memory.h"
#include "pico_script.h"
#include "utils.h"

static pico_api::colour_t* backbuffer_store = nullptr;
static pico_api::colour_t* backbuffer = nullptr;
static pico_api::colour_t* backbuffer_guard1 = nullptr;
static pico_api::colour_t* backbuffer_guard2 = nullptr;

static const int guard_size = 256 * 16;

static int buffer_size_x = 0;
static int buffer_size_y = 0;

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

static std::string lastLoadedCart;

static InputState inputState[4];
static MouseState mouseState;
static std::string cartDataName;
static bool pauseMenuRequested = false;
static bool pauseMenuActive = false;

struct GraphicsState {
	pico_api::colour_t fg = 7;
	pico_api::colour_t bg = 0;
	uint16_t pattern = 0;
	bool pattern_transparent = false;
	int text_x = 0;
	int text_y = 0;
	int clip_x1 = 0;
	int clip_y1 = 0;
	int clip_x2 = 128;
	int clip_y2 = 128;
	int max_clip_x = 128;
	int max_clip_y = 128;
	int camera_x = 0;
	int camera_y = 0;
	int line_x = 0;
	int line_y = 0;
	std::array<pico_api::colour_t, 256> palette_map;
	std::array<bool, 256> transparent;
	bool extendedPalette = false;
};

static GraphicsState* currentGraphicsState = nullptr;
static std::map<int, GraphicsState> extendedGraphicsStates;

struct SpriteSheet {
	pico_api::colour_t sprite_data[128 * 128];
	uint8_t flags[256];
};

static SpriteSheet spriteSheet;
static SpriteSheet* currentSprData = &spriteSheet;
static std::map<int, SpriteSheet> extendedSpriteSheets;

static SpriteSheet fontSheet;
static SpriteSheet* currentFontData = &fontSheet;
static std::map<int, SpriteSheet> extendedFontSheets;

struct MapSheet {
	uint8_t map_data[128 * 64];
};

static uint8_t cart_data[pico_ram::MEM_CART_DATA_SIZE] = {0};
static uint8_t scratch_data[pico_ram::MEM_SCRATCH_SIZE] = {0};
static uint8_t music_data[pico_ram::MEM_MUSIC_SIZE] = {0};
static uint8_t sfx_data[pico_ram::MEM_SFX_SIZE] = {0};

static MapSheet mapSheet;
static MapSheet* currentMapData = &mapSheet;
static std::map<int, MapSheet> extendedMapSheets;

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
static pico_ram::SplitNibbleMemoryArea mem_screen(backbuffer,
                                                  pico_ram::MEM_SCREEN_ADDR,
                                                  pico_ram::MEM_SCREEN_SIZE);

static pico_ram::LinearMemoryAreaDF mem_cart_data(cart_data,
                                                  pico_ram::MEM_CART_DATA_ADDR,
                                                  pico_ram::MEM_CART_DATA_SIZE);

static pico_ram::LinearMemoryArea mem_scratch_data(scratch_data,
                                                   pico_ram::MEM_SCRATCH_ADDR,
                                                   pico_ram::MEM_SCRATCH_SIZE);

static pico_ram::LinearMemoryArea mem_music_data(music_data,
                                                 pico_ram::MEM_MUSIC_ADDR,
                                                 pico_ram::MEM_MUSIC_SIZE);

static pico_ram::LinearMemoryArea mem_sfx_data(sfx_data,
                                               pico_ram::MEM_SFX_ADDR,
                                               pico_ram::MEM_SFX_SIZE);

namespace pico_private {
	using namespace pico_api;

	void init_guards() {
		for (int i = 0; i < guard_size; i++) {
			backbuffer_guard1[i] = i;
			backbuffer_guard2[i] = i;
		}
	}

	void check_guards() {
		for (int i = 0; i < guard_size; i++) {
			assert(backbuffer_guard1[i] == (i & 0xff));
			assert(backbuffer_guard2[i] == (i & 0xff));
		}
	}

	static void restore_palette() {
		for (size_t n = 0; n < currentGraphicsState->palette_map.size(); n++) {
			currentGraphicsState->palette_map[n] = (colour_t)n;
		}
	}

	static void restore_transparency() {
		for (size_t n = 0; n < currentGraphicsState->palette_map.size(); n++) {
			currentGraphicsState->transparent[n] = false;
		}
		currentGraphicsState->transparent[0] = true;
	}

	// test if rectangle is within cliping rectangle
	static bool is_visible(int x, int y, int w, int h) {
		if (x >= currentGraphicsState->clip_x2)
			return false;
		if (y >= currentGraphicsState->clip_y2)
			return false;
		if (x + w <= currentGraphicsState->clip_x1)
			return false;
		if (y + h <= currentGraphicsState->clip_y1)
			return false;
		if (w <= 0 || h <= 0)
			return false;

		return true;
	}

	static void blitter(SpriteSheet& sprites,
	                    int scr_x,
	                    int scr_y,
	                    int spr_x,
	                    int spr_y,
	                    int spr_w,
	                    int spr_h,
	                    bool flip_x = false,
	                    bool flip_y = false) {
		if (!is_visible(scr_x, scr_y, spr_w, spr_h))
			return;

		int scr_w = spr_w;
		int scr_h = spr_h;

		// left clip
		if (scr_x < currentGraphicsState->clip_x1) {
			int nclip = currentGraphicsState->clip_x1 - scr_x;
			scr_x = currentGraphicsState->clip_x1;
			scr_w -= nclip;
			if (!flip_x) {
				spr_x += nclip;
			} else {
				spr_w -= nclip;
			}
		}

		// right clip
		if (scr_x + scr_w > currentGraphicsState->clip_x2) {
			int nclip = (scr_x + scr_w) - currentGraphicsState->clip_x2;
			scr_w -= nclip;
		}

		// top clip
		if (scr_y < currentGraphicsState->clip_y1) {
			int nclip = currentGraphicsState->clip_y1 - scr_y;
			scr_y = currentGraphicsState->clip_y1;
			scr_h -= nclip;
			if (!flip_y) {
				spr_y += nclip;
			} else {
				spr_h -= nclip;
			}
		}

		// bottom clip
		if (scr_y + scr_h > currentGraphicsState->clip_y2) {
			int nclip = (scr_y + scr_h) - currentGraphicsState->clip_y2;
			scr_h -= nclip;
		}

		int dy = 1;
		if (flip_y) {
			spr_y += spr_h - 1;
			dy = -dy;
		}

		colour_t* pix = backbuffer + scr_y * buffer_size_x + scr_x;
		for (int y = 0; y < scr_h; y++) {
			colour_t* spr = sprites.sprite_data + ((spr_y + y * dy) & 0x7f) * 128;

			if (!flip_x) {
				for (int x = 0; x < scr_w; x++) {
					colour_t c = spr[(spr_x + x) & 0x7f];
					if (!currentGraphicsState->transparent[c]) {
						pix[x] = currentGraphicsState->palette_map[c];
					}
				}
			} else {
				for (int x = 0; x < scr_w; x++) {
					colour_t c = spr[(spr_x + spr_w - x - 1) & 0x7f];
					if (!currentGraphicsState->transparent[c]) {
						pix[x] = currentGraphicsState->palette_map[c];
					}
				}
			}
			pix += buffer_size_x;
		}
	}

	static void stretch_blitter(SpriteSheet& sprites,
	                            int spr_x,
	                            int spr_y,
	                            int spr_w,
	                            int spr_h,
	                            int scr_x,
	                            int scr_y,
	                            int scr_w,
	                            int scr_h,
	                            bool flip_x = false,
	                            bool flip_y = false) {
		if (spr_h == scr_h && spr_w == scr_w) {
			// use faster non stretch blitter if sprite is not stretched
			blitter(sprites, scr_x, scr_y, spr_x, spr_y, scr_w, scr_h, flip_x, flip_y);
			return;
		}

		if (!is_visible(scr_x, scr_y, scr_w, scr_h))
			return;

		spr_x = spr_x << 16;
		spr_y = spr_y << 16;
		spr_w = spr_w << 16;
		spr_h = spr_h << 16;

		int dx = spr_w / scr_w;
		int dy = spr_h / scr_h;

		// left clip
		if (scr_x < currentGraphicsState->clip_x1) {
			int nclip = currentGraphicsState->clip_x1 - scr_x;
			scr_x = currentGraphicsState->clip_x1;
			scr_w -= nclip;
			if (!flip_x) {
				spr_x += nclip * dx;
			} else {
				spr_w -= nclip * dx;
			}
		}

		// right clip
		if (scr_x + scr_w > currentGraphicsState->clip_x2) {
			int nclip = (scr_x + scr_w) - currentGraphicsState->clip_x2;
			scr_w -= nclip;
		}

		// top clip
		if (scr_y < currentGraphicsState->clip_y1) {
			int nclip = currentGraphicsState->clip_y1 - scr_y;
			scr_y = currentGraphicsState->clip_y1;
			scr_h -= nclip;
			if (!flip_y) {
				spr_y += nclip * dy;
			} else {
				spr_h -= nclip * dy;
			}
		}

		// bottom clip
		if (scr_y + scr_h > currentGraphicsState->clip_y2) {
			int nclip = (scr_y + scr_h) - currentGraphicsState->clip_y2;
			scr_h -= nclip;
		}

		if (flip_y) {
			spr_y += spr_h - 1 * dy;
			dy = -dy;
		}

		colour_t* pix = backbuffer + scr_y * buffer_size_x + scr_x;
		for (int y = 0; y < scr_h; y++) {
			colour_t* spr = sprites.sprite_data + (((spr_y + y * dy) >> 16) & 0x7f) * 128;

			if (!flip_x) {
				for (int x = 0; x < scr_w; x++) {
					colour_t c = spr[((spr_x + x * dx) >> 16) & 0x7f];
					if (!currentGraphicsState->transparent[c]) {
						pix[x] = currentGraphicsState->palette_map[c];
					}
				}
			} else {
				for (int x = 0; x < scr_w; x++) {
					colour_t c = spr[((spr_x + spr_w - (x + 1) * dx) >> 16) & 0x7f];
					if (!currentGraphicsState->transparent[c]) {
						pix[x] = currentGraphicsState->palette_map[c];
					}
				}
			}
			pix += buffer_size_x;
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

	inline void normalise_coords(int& c0, int& c1) {
		if (c0 > c1)
			std::swap(c0, c1);
	}

	void hline(int x0, int x1, int y) {
		normalise_coords(x0, x1);
		x1++;
		if (y < currentGraphicsState->clip_y1 || y >= currentGraphicsState->clip_y2) {
			return;
		}
		x0 = utils::limit(x0, currentGraphicsState->clip_x1, currentGraphicsState->clip_x2);
		x1 = utils::limit(x1, currentGraphicsState->clip_x1, currentGraphicsState->clip_x2);

		colour_t fg = currentGraphicsState->palette_map[currentGraphicsState->fg];
		colour_t bg = currentGraphicsState->palette_map[currentGraphicsState->bg];

		colour_t* pix = backbuffer + y * buffer_size_x;
		uint16_t pat = currentGraphicsState->pattern;
		bool pattr = currentGraphicsState->pattern_transparent;

		if (pat == 0) {
			memset(pix + x0, fg, x1 - x0);
		} else if (pat == 0xffff && !pattr) {
			memset(pix + x0, bg, x1 - x0);
		} else {
			if (pattr) {
				for (int x = x0; x < x1; x++) {
					if (((pat >> ((3 - (x & 0x3)) + (3 - (y & 0x3)) * 4)) & 1) == 0) {
						pix[x] = fg;
					}
				}
			} else {
				for (int x = x0; x < x1; x++) {
					pix[x] = ((pat >> ((3 - (x & 0x3)) + (3 - (y & 0x3)) * 4)) & 1) ? bg : fg;
				}
			}
		}
	}

	void vline(int y0, int y1, int x) {
		if (x < currentGraphicsState->clip_x1 || x >= currentGraphicsState->clip_x2) {
			return;
		}

		y0 = utils::limit(y0, currentGraphicsState->clip_y1, currentGraphicsState->clip_y2);
		y1 = utils::limit(y1, currentGraphicsState->clip_y1, currentGraphicsState->clip_y2);

		colour_t* pix = backbuffer + y0 * buffer_size_x;

		colour_t fg = currentGraphicsState->palette_map[currentGraphicsState->fg];
		colour_t bg = currentGraphicsState->palette_map[currentGraphicsState->bg];
		uint16_t pat = currentGraphicsState->pattern;
		bool pattr = currentGraphicsState->pattern_transparent;

		if (pattr) {
			for (int y = y0; y < y1; y++) {
				if (((pat >> ((3 - (x & 0x3)) + (3 - (y & 0x3)) * 4)) & 1) == 0) {
					pix[x] = fg;
				}
				pix += buffer_size_x;
			}
		} else {
			for (int y = y0; y < y1; y++) {
				pix[x] = ((pat >> ((3 - (x & 0x3)) + (3 - (y & 0x3)) * 4)) & 1) ? bg : fg;
				pix += buffer_size_x;
			}
		}
	}

	void pset(int x, int y) {
		if (x < currentGraphicsState->clip_x1 || x >= currentGraphicsState->clip_x2 ||
		    y < currentGraphicsState->clip_y1 || y >= currentGraphicsState->clip_y2) {
			return;
		}

		colour_t* pix = backbuffer + y * buffer_size_x + x;
		uint16_t pat = currentGraphicsState->pattern;
		colour_t fg = currentGraphicsState->palette_map[currentGraphicsState->fg];
		colour_t bg = currentGraphicsState->palette_map[currentGraphicsState->bg];
		bool pattr = currentGraphicsState->pattern_transparent;

		if (pat == 0) {
			*pix = fg;
		} else if (pat == 0xffff && !pattr) {
			*pix = bg;
		} else {
			if (pattr) {
				if (((pat >> ((3 - (x & 0x3)) + (3 - (y & 0x3)) * 4)) & 1) == 0) {
					pix[x] = fg;
				}
			} else {
				*pix = ((pat >> ((3 - (x & 0x3)) + (3 - (y & 0x3)) * 4)) & 1) ? bg : fg;
			}
		}
	}

	void apply_camera(int& x, int& y) {
		x = x - currentGraphicsState->camera_x;
		y = y - currentGraphicsState->camera_y;
	}

	void copy_cartdata_to_ram(const std::string& data) {
		uint16_t addr = pico_ram::MEM_CART_DATA_ADDR;

		std::string buf;

		for (size_t n = 0; n < data.length(); n++) {
			if (data[n] > ' ') {
				buf += data[n];
				if (buf.length() == 8) {
					pico_api::poke4(addr, strtol(buf.c_str(), nullptr, 16));
					addr += 4;
					buf.clear();
				}
			}
		}
		mem_cart_data.clearDirty();
	}

	std::string get_cartdata_as_str() {
		uint16_t addr = pico_ram::MEM_CART_DATA_ADDR;
		uint16_t len = pico_ram::MEM_CART_DATA_SIZE;

		std::string result;
		char buffer[16];
		int pos = 0;
		for (int32_t n = addr; n < (addr + len); n += 4) {
			sprintf(buffer, "%08x", peek4(n));
			result += buffer;
			pos++;
			if (pos == 8) {
				result += '\n';
				pos = 0;
			}
		}

		return result;
	}

	void copy_data_to_ram(uint16_t addr, const std::string& data) {
		for (size_t n = 0; n < data.length(); n++) {
			char buf[3] = {0};

			if (data[n] > ' ') {
				buf[0] = data[n++];
				buf[1] = data[n];
				pico_api::poke(addr++, (uint8_t)strtol(buf, nullptr, 16));
			}
		}
	}

	void copy_gfxdata_to_ram(uint16_t addr, const std::string& data) {
		for (size_t n = 0; n < data.length(); n++) {
			char buf[3] = {0};

			if (data[n] > ' ') {
				buf[1] = data[n++];
				buf[0] = data[n];
				pico_api::poke(addr++, (uint8_t)strtol(buf, nullptr, 16));
			}
		}
	}

	void copy_data_to_sprites(SpriteSheet& sprites, const std::string& data, bool bits8) {
		uint16_t i = 0;

		for (size_t n = 0; n < data.length(); n++) {
			char buf[3] = {0};

			if (data[n] > ' ') {
				buf[0] = data[n++];
				buf[1] = data[n];
				auto val = (uint8_t)strtol(buf, nullptr, 16);
				if (bits8) {
					sprites.sprite_data[i++] = val;
				} else {
					sprites.sprite_data[i++] = val >> 4;
					sprites.sprite_data[i++] = val & 0x0f;
				}
			}
		}
	}

	inline colour_t fgcolor(uint16_t c) {
		if (currentGraphicsState->extendedPalette) {
			return c & 0xff;
		} else {
			return c & 0xf;
		}
	}

	inline colour_t bgcolor(uint16_t c) {
		if (currentGraphicsState->extendedPalette) {
			return c >> 8;
		} else {
			return (c >> 4) & 0xf;
		}
	}
}  // namespace pico_private

namespace pico_control {

	void init_backbuffer_mem(int x, int y) {
		TraceFunction();
		x = utils::limit(x, config::MIN_SCREEN_WIDTH, config::MAX_SCREEN_WIDTH);
		y = utils::limit(y, config::MIN_SCREEN_HEIGHT, config::MAX_SCREEN_HEIGHT);

		buffer_size_x = x;
		buffer_size_y = y;

		backbuffer = backbuffer_store + guard_size;
		backbuffer_guard1 = backbuffer_store;
		backbuffer_guard2 = backbuffer + x * y;

		pico_private::init_guards();
	}

	void init() {
		TraceFunction();
		if (!backbuffer_store) {
			backbuffer_store =
			    new uint8_t[config::MAX_SCREEN_WIDTH * config::MAX_SCREEN_HEIGHT + guard_size * 2];
		}

		init_backbuffer_mem(config::INIT_SCREEN_WIDTH, config::INIT_SCREEN_HEIGHT);
		mem_screen.setData(backbuffer);

		cartDataName = "";
		pico_apix::gfxstate(0);

		pauseMenuActive = false;

		ram.addMemoryArea(&mem_gfx);
		ram.addMemoryArea(&mem_gfx2_map2);
		ram.addMemoryArea(&mem_map);
		ram.addMemoryArea(&mem_flags);
		ram.addMemoryArea(&mem_screen);
		ram.addMemoryArea(&mem_cart_data);
		ram.addMemoryArea(&mem_scratch_data);
		ram.addMemoryArea(&mem_music_data);
		ram.addMemoryArea(&mem_sfx_data);

		audio_init();
	}

	void frame_start() {
	}

	void frame_end() {
		if (mem_cart_data.isDirty()) {
			if (!cartDataName.empty()) {
				FILE_SaveGameState(cartDataName + ".p8d.txt", pico_private::get_cartdata_as_str());
			}
			mem_cart_data.clearDirty();
		}
		if (pauseMenuRequested)
			begin_pause_menu();
	}

	pico_api::colour_t* get_buffer(int& width, int& height) {
		width = buffer_size_x;
		height = buffer_size_y;
		// pico_private::check_guards();
		return backbuffer;
	}

	void set_sprite_data_4bit(std::string data) {
		TraceFunction();
		if (data.size()) {
			if (currentSprData == &spriteSheet) {
				pico_private::copy_gfxdata_to_ram(pico_ram::MEM_GFX_ADDR, data);
			} else {
				pico_private::copy_data_to_sprites(*currentSprData, data, false);
			}
		}
	}

	void set_sprite_data_8bit(std::string data) {
		TraceFunction();
		if (data.size()) {
			logr << " loading 8bit sprite data";
			pico_private::copy_data_to_sprites(*currentSprData, data, true);
		}
	}

	void set_sprite_flags(std::string flags) {
		TraceFunction();
		pico_private::copy_data_to_ram(pico_ram::MEM_GFX_PROPS_ADDR, flags);
	}

	void set_font_data(std::string data) {
		TraceFunction();
		pico_private::copy_data_to_sprites(*currentFontData, data, false);
	}

	void set_map_data(std::string data) {
		TraceFunction();
		pico_private::copy_data_to_ram(pico_ram::MEM_MAP_ADDR, data);
	}

	void set_input_state(int state, int player) {
		inputState[player].set(state);
		if (player == 0) {
			if (inputState[0].justPressed(6) && !is_pause_menu()) {
				pauseMenuRequested = true;
			}
		}
	}

	void set_mouse_state(const MouseState& ms) {
		mouseState = ms;
	}

	void test_integrity() {
		pico_private::check_guards();
	}

	void begin_pause_menu() {
		pico_apix::gfxstate(-1);
		pauseMenuRequested = false;
		pauseMenuActive = true;
	}

	bool is_pause_menu() {
		return pauseMenuActive;
	}

	void end_pause_menu() {
		pauseMenuActive = false;
		pico_apix::gfxstate(0);
	}

	uint8_t* get_music_data() {
		return music_data;
	}
	uint8_t* get_sfx_data() {
		return sfx_data;
	}

	void restartCart() {
		TraceFunction();
		pauseMenuActive = false;
		init_backbuffer_mem(config::INIT_SCREEN_WIDTH, config::INIT_SCREEN_HEIGHT);
		extendedGraphicsStates[0] = GraphicsState{};
		stop_all_audio();
		audio_init();
		pico_private::restore_palette();
		pico_private::restore_transparency();
		pico_cart::extractCart(pico_cart::getCart());
		pico_apix::gfxstate(0);
	}

	void displayerror(const std::string& msg) {
		using namespace pico_api;
		using namespace pico_apix;
		screen(256, 256);
		cls();
		pal();
		palt();
		size_t pos = 0;
		int y = 8;
		print("script error:", 0, 0, 7);
		while (pos < msg.size()) {
			print(msg.substr(pos, 64), 0, y, 7);
			pos += 64;
			y += 8;
		}
	}

}  // namespace pico_control

namespace pico_api {

	void load(std::string cartname) {
		TraceFunction();
		pico_cart::load(cartname);
		lastLoadedCart = cartname;
		pico_control::restartCart();
	}

	void reload() {
		TraceFunction();
		load(pico_cart::getCart().sections["basepath"] + pico_cart::getCart().sections["filename"]);
	}

	void run() {
		TraceFunction();
		pico_control::restartCart();
	}

	void color(uint16_t c) {
		currentGraphicsState->fg = pico_private::fgcolor(c);
		currentGraphicsState->bg = pico_private::bgcolor(c >> 8);
	}

	void cls(colour_t c) {
		colour_t p = currentGraphicsState->palette_map[c];
		memset(backbuffer, p, buffer_size_x * buffer_size_y);

		currentGraphicsState->text_x = 0;
		currentGraphicsState->text_y = 0;
	}

	uint8_t peek(uint16_t a) {
		return ram.peek(a);
	}

	uint16_t peek2(uint16_t a) {
		uint16_t v = peek(a);
		v |= uint32_t(peek(a + 1)) << 8;
		return v;
	}

	uint32_t peek4(uint16_t a) {
		uint32_t v = peek(a);
		v |= uint32_t(peek(a + 1)) << 8;
		v |= uint32_t(peek(a + 2)) << 16;
		v |= uint32_t(peek(a + 3)) << 24;
		return v;
	}

	void poke(uint16_t a, uint8_t v) {
		if (a >= 0x5f00 && a <= 0x5f0f) {
			pal(a - 0x5f00, v);
			palt(a - 0x5f00, (v & 0x80) != 0);
		} else {
			ram.poke(a, v);
		}
	}

	void poke2(uint16_t a, uint16_t v) {
		poke(a, v);
		poke(a + 1, v >> 8);
	}

	void poke4(uint16_t a, uint32_t v) {
		poke(a, v);
		poke(a + 1, v >> 8);
		poke(a + 2, v >> 16);
		poke(a + 3, v >> 24);
	}

	void memory_set(uint16_t a, uint8_t val, uint16_t len) {
		for (uint32_t i = 0; i < len; i++) {
			ram.poke(a + i, val);
		}
	}

	void memory_cpy(uint16_t dest_a, uint16_t src_a, uint16_t len) {
		if (uint32_t(dest_a) - uint32_t(src_a) >= len) {
			for (uint32_t i = 0; i < len; i++) {
				ram.poke(dest_a + i, ram.peek(src_a + i));
			}
		} else {
			src_a += len;
			dest_a += len;
			for (int32_t i = len - 1; i >= 0; i++) {
				ram.poke(dest_a - i, ram.peek(src_a - i));
			}
		}
	}

	void cartdata(std::string name) {
		cartDataName = name;
		std::string data = FILE_LoadGameState(cartDataName + ".p8d.txt");
		pico_private::copy_cartdata_to_ram(data);
	}

	uint32_t dget(uint16_t a) {
		return peek4(pico_ram::MEM_CART_DATA_ADDR + ((a * 4) & 0xff));
	}

	void dset(uint16_t a, uint32_t v) {
		poke4(pico_ram::MEM_CART_DATA_ADDR + ((a * 4) & 0xff), v);
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
		pico_private::apply_camera(x, y);

		int spr_x = (n % 16) * 8;
		int spr_y = (n / 16) * 8;
		pico_private::blitter(*currentSprData, x, y, spr_x, spr_y, w * 8, h * 8, flip_x, flip_y);
	}

	void sspr(int sx, int sy, int sw, int sh, int dx, int dy) {
		pico_private::apply_camera(dx, dy);
		pico_private::blitter(*currentSprData, dx, dy, sx, sy, sw, sh);
	}

	void sspr(int sx, int sy, int sw, int sh, int dx, int dy, int dw, int dh) {
		pico_private::apply_camera(dx, dy);
		pico_private::stretch_blitter(*currentSprData, sx, sy, sw, sh, dx, dy, dw, dh);
	}

	void
	sspr(int sx, int sy, int sw, int sh, int dx, int dy, int dw, int dh, bool flip_x, bool flip_y) {
		pico_private::apply_camera(dx, dy);
		pico_private::stretch_blitter(*currentSprData, sx, sy, sw, sh, dx, dy, dw, dh, flip_x,
		                              flip_y);
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

	void pset(int x, int y, uint16_t c) {
		color(c);
		pico_private::apply_camera(x, y);
		pico_private::pset(x, y);
	}

	colour_t pget(int x, int y) {
		pico_private::apply_camera(x, y);
		x &= 0x7f;
		y &= 0x7f;
		return backbuffer[y * buffer_size_x + x];
	}

	void rect(int x0, int y0, int x1, int y1) {
		rect(x0, y0, x1, y1, currentGraphicsState->fg);
	}

	void rect(int x0, int y0, int x1, int y1, uint16_t c) {
		pico_private::apply_camera(x0, y0);
		pico_private::apply_camera(x1, y1);
		color(c);
		pico_private::normalise_coords(x0, x1);
		pico_private::normalise_coords(y0, y1);
		pico_private::hline(x0, x1, y0);
		pico_private::hline(x0, x1, y1);
		pico_private::vline(y0, y1, x0);
		pico_private::vline(y0, y1, x1);
	}

	void rectfill(int x0, int y0, int x1, int y1) {
		rectfill(x0, y0, x1, y1, currentGraphicsState->fg);
	}

	void rectfill(int x0, int y0, int x1, int y1, uint16_t c) {
		using namespace pico_private;

		pico_private::apply_camera(x0, y0);
		pico_private::apply_camera(x1, y1);
		color(c);
		pico_private::normalise_coords(x0, x1);
		pico_private::normalise_coords(y0, y1);

		pico_private::clip_rect(x0, y0, x1, y1);
		colour_t* pix = backbuffer + y0 * buffer_size_x;
		colour_t p1 = currentGraphicsState->palette_map[fgcolor(c)];
		colour_t p2 = currentGraphicsState->palette_map[bgcolor(c)];

		uint16_t pat = currentGraphicsState->pattern;
		bool pattr = currentGraphicsState->pattern_transparent;

		if (pattr) {
			for (int y = y0; y <= y1; y++) {
				for (int x = x0; x <= x1; x++) {
					if (((pat >> ((3 - (x & 0x3)) + (3 - (y & 0x3)) * 4)) & 1) == 0) {
						pix[x] = p1;
					}
				}
				pix += buffer_size_x;
			}
		} else {
			for (int y = y0; y <= y1; y++) {
				for (int x = x0; x <= x1; x++) {
					pix[x] = ((pat >> ((3 - (x & 0x3)) + (3 - (y & 0x3)) * 4)) & 1) ? p2 : p1;
				}
				pix += buffer_size_x;
			}
		}
	}

	void circ(int x, int y, int r) {
		circ(x, y, r, currentGraphicsState->fg);
	}

	void circ(int xm, int ym, int r, uint16_t c) {
		pico_private::apply_camera(xm, ym);
		color(c);
		if (r >= 0) {
			int x = -r, y = 0, err = 2 - 2 * r; /* II. Quadrant */
			do {
				pico_private::pset(xm - x, ym + y); /*   I. Quadrant */
				pico_private::pset(xm - y, ym - x); /*  II. Quadrant */
				pico_private::pset(xm + x, ym - y); /* III. Quadrant */
				pico_private::pset(xm + y, ym + x); /*  IV. Quadrant */
				r = err;
				if (r > x)
					err += ++x * 2 + 1; /* e_xy+e_x > 0 */
				if (r <= y)
					err += ++y * 2 + 1; /* e_xy+e_y < 0 */
			} while (x < 0);
		}
	}

	void circfill(int x, int y, int r) {
		circfill(x, y, r, currentGraphicsState->fg);
	}

	void circfill(int xm, int ym, int r, uint16_t c) {
		pico_private::apply_camera(xm, ym);
		color(c);
		if (r == 0) {
			pico_private::pset(xm, ym);
		} else if (r == 1) {
			pico_private::pset(xm, ym - 1);
			pico_private::hline(xm - 1, xm + 1, ym);
			pico_private::pset(xm, ym + 1);
		} else if (r > 0) {
			int x = -r, y = 0, err = 2 - 2 * r;
			do {
				pico_private::hline(xm - x, xm + x, ym + y);
				pico_private::hline(xm - x, xm + x, ym - y);
				r = err;
				if (r > x)
					err += ++x * 2 + 1;
				if (r <= y)
					err += ++y * 2 + 1;
			} while (x < 0);
		}
	}

	void line(int x, int y) {
		line(currentGraphicsState->line_x, currentGraphicsState->line_y, x, y,
		     currentGraphicsState->fg);
	}

	void line(int x0, int y0, int x1, int y1) {
		line(x0, y0, x1, y1, currentGraphicsState->fg);
	}

	void line(int x0, int y0, int x1, int y1, uint16_t c) {
		currentGraphicsState->line_x = x1;
		currentGraphicsState->line_y = y1;
		pico_private::apply_camera(x0, y0);
		pico_private::apply_camera(x1, y1);
		color(c);
		int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
		int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
		int err = dx + dy, e2; /* error value e_xy */

		for (;;) { /* loop */
			pico_private::pset(x0, y0);
			if (x0 == x1 && y0 == y1)
				break;
			e2 = 2 * err;
			if (e2 >= dy) {
				err += dy;
				x0 += sx;
			} /* e_xy+e_x > 0 */
			if (e2 <= dx) {
				err += dx;
				y0 += sy;
			} /* e_xy+e_y < 0 */
		}
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
		currentGraphicsState->palette_map[c0 & 0xf] = c1 & 0xf;
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

	void cursor(int x, int y) {
		currentGraphicsState->text_x = x;
		currentGraphicsState->text_y = y;
	}

	void cursor(int x, int y, uint16_t c) {
		color(c);
		currentGraphicsState->text_x = x;
		currentGraphicsState->text_y = y;
	}

	void print(std::string str) {
		print(str, currentGraphicsState->text_x, currentGraphicsState->text_y);
	}

	void print(std::string str, int x, int y) {
		print(str, x, y, currentGraphicsState->fg);
	}

	void print(std::string str, int x, int y, uint16_t c) {
		pico_private::apply_camera(x, y);
		color(c);

		colour_t old = currentGraphicsState->palette_map[7];
		bool oldt = currentGraphicsState->transparent[0];

		currentGraphicsState->palette_map[7] = currentGraphicsState->fg;
		currentGraphicsState->transparent[0] = true;

		currentGraphicsState->text_x = x;

		for (size_t n = 0; n < str.length(); n++) {
			uint8_t ch = str[n];
			if (ch >= 0x20 && ch < 0x80) {
				int index = ch - 32;
				pico_private::blitter(*currentFontData, x, y, (index % 32) * 4, (index / 32) * 6, 4,
				                      5);
				x += 4;
			} else if (ch >= 0x80 && ch <= 0x99) {
				int index = ch - 0x80;
				pico_private::blitter(*currentFontData, x, y, (index % 16) * 8,
				                      (index / 16) * 6 + 18, 8, 5);
				x += 8;
			} else if (ch == '\n') {
				x = currentGraphicsState->text_x;
				y += 6;
			}
		}

		currentGraphicsState->text_x = 0;
		currentGraphicsState->text_y = y + 6;

		currentGraphicsState->palette_map[7] = old;
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
		using namespace pico_private;

		currentGraphicsState->clip_x1 = utils::limit(x, 0, buffer_size_x);
		currentGraphicsState->clip_y1 = utils::limit(y, 0, buffer_size_y);
		currentGraphicsState->clip_x2 = utils::limit(x + w, 0, buffer_size_x);
		currentGraphicsState->clip_y2 = utils::limit(y + h, 0, buffer_size_y);
	}

	void clip() {
		clip(0, 0, currentGraphicsState->max_clip_x, currentGraphicsState->max_clip_y);
	}

	void camera() {
		camera(0, 0);
	}

	void camera(int x, int y) {
		currentGraphicsState->camera_x = x;
		currentGraphicsState->camera_y = y;
	}

	void fillp() {
		fillp(0, false);
	}

	void fillp(int pattern, bool transparent) {
		currentGraphicsState->pattern = pattern;
		currentGraphicsState->pattern_transparent = transparent;
	}

	int stat(int key, std::string& sval, int& ival) {
		switch (key) {
			case 32:
				ival = mouseState.x;
				return 2;
			case 33:
				ival = mouseState.y;
				return 2;
			case 34:
				ival = mouseState.buttons;
				return 2;
			case 36:
				ival = mouseState.wheel;
				return 2;
			case 102:
				sval = TOSTRING(TAC08_PLATFORM);
				return 1;
			case 400:
				sval = pico_cart::getCart().sections["base_path"];
				return 1;
			case 401:
				sval = pico_cart::getCart().sections["cart_name"];
				return 1;
			case 410:
				ival = buffer_size_x;
				return 2;
			case 411:
				ival = buffer_size_y;
				return 2;
			case 412: {
				int x, y;
				GFX_GetDisplayArea(&x, &y);
				ival = x;
				return 2;
			}
			case 413: {
				int x, y;
				GFX_GetDisplayArea(&x, &y);
				ival = y;
				return 2;
			}
		}

		ival = 0;
		return 2;
	}

}  // namespace pico_api

namespace pico_apix {
	void wrclip(const std::string& s) {
		FILE_WriteClip(s);
	}

	std::string rdclip() {
		return FILE_ReadClip();
	}

	void wrstr(const std::string& name, const std::string& s) {
		FILE_SaveGameState(cartDataName + "_" + name, s);
	}

	std::string rdstr(const std::string& name) {
		return FILE_LoadGameState(cartDataName + "_" + name);
	}

	void setpal(uint8_t i, uint8_t r, uint8_t g, uint8_t b) {
		GFX_SetPaletteIndex(i, r, g, b);
	}

	void selpal(const std::string& name) {
		GFX_SelectPalette(name);
	}

	void resetpal() {
		GFX_RestorePalette();
	}

	void resetpal(uint8_t i) {
		GFX_RestorePaletteIndex(i);
	}

	void screen(uint16_t w, uint16_t h) {
		pico_control::init_backbuffer_mem(w, h);
		currentGraphicsState->max_clip_x = w;
		currentGraphicsState->max_clip_y = h;
		pico_api::clip();
	}

	void xpal(bool enable) {
		currentGraphicsState->extendedPalette = enable;
	}

	void cursor(bool enable) {
		GFX_ShowHWMouse(enable);
	}

	void menu() {
		if (!pico_control::is_pause_menu())
			pauseMenuRequested = true;
	}

	void siminput(uint8_t state) {
		INP_SetSimState(state);
	}

	void sprites() {
		currentSprData = &spriteSheet;
	}

	void sprites(int page) {
		if (extendedSpriteSheets.find(page) == extendedSpriteSheets.end()) {
			memset(&extendedSpriteSheets[page], 0, sizeof(SpriteSheet));
		}
		currentSprData = &extendedSpriteSheets[page];
	}

	void maps() {
		currentMapData = &mapSheet;
	}

	void maps(int page) {
		if (extendedMapSheets.find(page) == extendedMapSheets.end()) {
			memset(&extendedMapSheets[page], 0, sizeof(MapSheet));
		}
		currentMapData = &extendedMapSheets[page];
	}

	void fonts() {
		currentFontData = &fontSheet;
	}

	void fonts(int page) {
		if (extendedFontSheets.find(page) == extendedFontSheets.end()) {
			memset(&extendedFontSheets[page], 0, sizeof(SpriteSheet));
		}
		currentFontData = &extendedFontSheets[page];
	}

	void fullscreen(bool enable) {
		GFX_SetFullScreen(enable);
	}

	void assetload(std::string filename) {
		pico_cart::loadassets(filename, pico_cart::getCart());
	}

	void gfxstate(int index) {
		if (extendedGraphicsStates.find(index) == extendedGraphicsStates.end()) {
			currentGraphicsState = &extendedGraphicsStates[index];
			GraphicsState* gs = currentGraphicsState;
			pico_private::restore_transparency();
			pico_private::restore_palette();
			gs->max_clip_x = buffer_size_x;
			gs->max_clip_y = buffer_size_y;
			pico_api::clip();
		} else {
			currentGraphicsState = &extendedGraphicsStates[index];
			GraphicsState* gs = currentGraphicsState;
			gs->clip_x1 = utils::limit(gs->clip_x1, 0, buffer_size_x);
			gs->clip_y1 = utils::limit(gs->clip_y1, 0, buffer_size_y);
			gs->clip_x2 = utils::limit(gs->clip_x2, 0, buffer_size_x);
			gs->clip_y2 = utils::limit(gs->clip_y2, 0, buffer_size_y);
			gs->max_clip_x = utils::limit(gs->max_clip_x, 0, buffer_size_x);
			gs->max_clip_y = utils::limit(gs->max_clip_y, 0, buffer_size_y);
		}
	}

	std::pair<std::string, bool> dbg_getsrc(std::string src, int line) {
		typedef std::pair<std::string, bool> return_t;

		if (size_t(line) <= pico_cart::getCart().source.size() && line >= 1) {
			auto l = pico_cart::getCart().source[line - 1].line;
			std::replace(l.begin(), l.end(), '\t', ' ');
			return return_t(l, true);
		}
		return return_t("", false);
	}
}  // namespace pico_apix
