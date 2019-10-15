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

static pico_api::colour_t* backbuffer = nullptr;

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

}  // namespace pico_private

namespace pico_control {

	void init_backbuffer_mem(int x, int y) {
		TraceFunction();
		x = utils::limit(x, config::MIN_SCREEN_WIDTH, config::MAX_SCREEN_WIDTH);
		y = utils::limit(y, config::MIN_SCREEN_HEIGHT, config::MAX_SCREEN_HEIGHT);

		buffer_size_x = x;
		buffer_size_y = y;

		pico_control::set_backbuffer(backbuffer, x, y, x);
	}

	void init() {
		TraceFunction();
		if (!backbuffer) {
			backbuffer = new uint8_t[config::MAX_SCREEN_WIDTH * config::MAX_SCREEN_HEIGHT];
		}
		gfx_init();

		init_backbuffer_mem(config::INIT_SCREEN_WIDTH, config::INIT_SCREEN_HEIGHT);
		pico_control::set_spritebuffer(spriteSheet.sprite_data);
		pico_control::set_spriteflags(spriteSheet.flags);
		pico_control::set_mapbuffer(mapSheet.map_data);
		pico_control::set_fontbuffer(fontSheet.sprite_data);

		mem_screen.setData(backbuffer);

		cartDataName = "";

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
		gfx_init();
		init_backbuffer_mem(config::INIT_SCREEN_WIDTH, config::INIT_SCREEN_HEIGHT);
		stop_all_audio();
		audio_init();
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
		pico_api::clip();
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
		pico_control::set_spritebuffer(currentSprData->sprite_data);
		pico_control::set_spriteflags(currentSprData->flags);
	}

	void sprites(int page) {
		if (extendedSpriteSheets.find(page) == extendedSpriteSheets.end()) {
			memset(&extendedSpriteSheets[page], 0, sizeof(SpriteSheet));
		}
		currentSprData = &extendedSpriteSheets[page];
		pico_control::set_spritebuffer(currentSprData->sprite_data);
		pico_control::set_spriteflags(currentSprData->flags);
	}

	void maps() {
		currentMapData = &mapSheet;
		pico_control::set_mapbuffer(currentMapData->map_data);
	}

	void maps(int page) {
		if (extendedMapSheets.find(page) == extendedMapSheets.end()) {
			memset(&extendedMapSheets[page], 0, sizeof(MapSheet));
		}
		currentMapData = &extendedMapSheets[page];
		pico_control::set_mapbuffer(currentMapData->map_data);
	}

	void fonts() {
		currentFontData = &fontSheet;
		pico_control::set_fontbuffer(currentFontData->sprite_data);
	}

	void fonts(int page) {
		if (extendedFontSheets.find(page) == extendedFontSheets.end()) {
			memset(&extendedFontSheets[page], 0, sizeof(SpriteSheet));
		}
		currentFontData = &extendedFontSheets[page];
		pico_control::set_fontbuffer(currentFontData->sprite_data);
	}

	void fullscreen(bool enable) {
		GFX_SetFullScreen(enable);
	}

	void assetload(std::string filename) {
		pico_cart::loadassets(filename, pico_cart::getCart());
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

	int dbg_getsrclines() {
		return pico_cart::getCart().source.size();
	}

	std::string getkey() {
		return INP_GetKeyPress();
	}

}  // namespace pico_apix
