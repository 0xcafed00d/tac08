#ifndef PICO_CORE_H
#define PICO_CORE_H

#include "hal_core.h"

namespace pico_api {
	typedef uint8_t colour_t;

	void cls(colour_t c);
	void cls();

	void pal(colour_t c0 = 255, colour_t c1 = 255);
	void palt(colour_t col = 255, bool t = false);

	void spr(int n, int x, int y, int w = 1, int h = 1, bool flip_x = false, bool flip_y = false);

	void map(int cell_x, int cell_y, int scr_x, int scr_y, int cell_w, int cell_h, int layer = 0);
	uint8_t mget(int x, int y);
	void mset(int x, int y, uint8_t v);

	void print(std::string str, int x = INT32_MAX, int y = INT32_MAX, colour_t c = UINT8_MAX);

	int btn(int n, int player = 0);
	int btnp(int n, int player = 0);

}  // namespace pico_api

namespace pico_control {
	void init(int x, int y);
	void set_buffer(pixel_t* buffer, int pitch);
	void set_sprite_data(std::string data, std::string flags);
	void set_map_data(std::string data);
	void set_font_data(std::string data);
	void set_input_state(int state, int player = 0);
}  // namespace pico_control

#endif /* PICO_CORE_H */