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

	uint8_t peek(uint16_t a);
	uint32_t peek4(uint16_t a);
	void poke(uint16_t a, uint8_t v);
	void poke4(uint16_t a, uint32_t v);
	void memory_set(uint16_t a, uint8_t val, uint16_t len);
	void memory_cpy(uint16_t dest_a, uint16_t src_a, uint16_t len);

	uint32_t dget(uint16_t a);
	void dset(uint16_t a, uint32_t v);
	void cartdata(std::string name);

	void color(uint8_t c);

	uint8_t fget(int n);
	bool fget(int n, int bit);
	void fset(int n, uint8_t val);
	void fset(int n, int bit, bool val);

	void spr(int n, int x, int y);
	void spr(int n, int x, int y, int w, int h);
	void spr(int n, int x, int y, int w, int h, bool flip_x, bool flip_y);
	void sspr(int sx, int sy, int sw, int sh, int dx, int dy);
	void sspr(int sx, int sy, int sw, int sh, int dx, int dy, int dw, int dh);
	void
	sspr(int sx, int sy, int sw, int sh, int dx, int dy, int dw, int dh, bool flip_x, bool flip_y);

	colour_t sget(int x, int y);
	void sset(int x, int y);
	void sset(int x, int y, colour_t c);

	void pset(int x, int y);
	void pset(int x, int y, colour_t colour);
	colour_t pget(int x, int y);

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
	void map(int cell_x, int cell_y, int scr_x, int scr_y, int cell_w, int cell_h, uint8_t layer);
	uint8_t mget(int x, int y);
	void mset(int x, int y, uint8_t v);

	void cursor(int x, int y);
	void print(std::string str);
	void print(std::string str, int x, int y);
	void print(std::string str, int x, int y, colour_t c);

	int btn();
	int btn(int n, int player = 0);

	int btnp();
	int btnp(int n, int player = 0);

	void camera();
	void camera(int x, int y);

	void clip(int x, int y, int w, int h);
	void clip();

	void fillp(int pattern);
	void fillp();

	int stat(int key, std::string& sval, int& ival);

}  // namespace pico_api

namespace pico_control {
	void init(int x, int y);
	pico_api::colour_t* get_buffer(int& width, int& height);
	void set_sprite_data(std::string data, std::string flags);
	void set_map_data(std::string data);
	void set_font_data(std::string data);
	void set_input_state(int state, int player = 0);
	void set_mouse_state(const MouseState& ms);
	void copy_shared_data();
	void test_integrity();

}  // namespace pico_control

#endif /* PICO_CORE_H */