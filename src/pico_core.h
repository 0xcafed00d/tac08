#ifndef PICO_CORE_H
#define PICO_CORE_H

#include "hal_core.h"

namespace pico_api {
	typedef uint8_t colour_t;

	void cls(colour_t c);
	void cls();

	void pal(colour_t c0, colour_t c1);
	void pal();

	void palt(colour_t col, bool t);
	void palt();

	void color(uint8_t c);

	uint8_t fget(int n);
	bool fget(int n, int bit);
	void fset(int n, uint8_t val);
	void fset(int n, int bit, bool val);

	void spr(int n, int x, int y);
	void spr(int n, int x, int y, int w, int h);
	void spr(int n, int x, int y, int w, int h, bool flip_x, bool flip_y);
	void sspr(int sx, int sy, int sw, int sh, int dx, int dy);

	colour_t sget(int x, int y);
	void sset(int x, int y);
	void sset(int x, int y, colour_t c);

	void pset(int x, int y);
	void pset(int x, int y, colour_t colour);

	void rect(int x0, int y0, int x1, int y1);
	void rect(int x0, int y0, int x1, int y1, colour_t c);
	void rectfill(int x0, int y0, int x1, int y1);
	void rectfill(int x0, int y0, int x1, int y1, colour_t c);

	void circ(int x, int y, int r);
	void circ(int x, int y, int r, colour_t c);
	void circfill(int x, int y, int r);
	void circfill(int x, int y, int r, colour_t c);

	void line(int x0, int y0, int x1, int y1);
	void line(int x0, int y0, int x1, int y1, colour_t c);

	void map(int cell_x, int cell_y);
	void map(int cell_x, int cell_y, int scr_x, int scr_y);
	void map(int cell_x, int cell_y, int scr_x, int scr_y, int cell_w, int cell_h);
	void map(int cell_x, int cell_y, int scr_x, int scr_y, int cell_w, int cell_h, int layer);
	uint8_t mget(int x, int y);
	void mset(int x, int y, uint8_t v);

	void print(std::string str);
	void print(std::string str, int x, int y);
	void print(std::string str, int x, int y, colour_t c);

	int btn(int n, int player = 0);
	int btnp(int n, int player = 0);

	void clip(int x, int y, int w, int h);
	void clip();

	void fillp(int pattern);
	void fillp();

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