#ifndef PICO_GFX_H
#define PICO_GFX_H

#include <stdint.h>
#include <string>

namespace pico_api {
	typedef uint8_t colour_t;

	void cls(colour_t c);
	void cls();

	void pal(colour_t c0, colour_t c1, int p);
	void pal(colour_t c0, colour_t c1);
	void pal();

	void palt(colour_t col, bool t);
	void palt();

	void color(uint16_t c);

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
	void pset(int x, int y, uint16_t colour, uint16_t pat = 0);
	colour_t pget(int x, int y);

	void rect(int x0, int y0, int x1, int y1);
	void rect(int x0, int y0, int x1, int y1, uint16_t c, uint16_t pat = 0);
	void rectfill(int x0, int y0, int x1, int y1);
	void rectfill(int x0, int y0, int x1, int y1, uint16_t c, uint16_t pat = 0);

	void circ(int x, int y, int r);
	void circ(int x, int y, int r, uint16_t c, uint16_t pat = 0);
	void circfill(int x, int y, int r);
	void circfill(int x, int y, int r, uint16_t c, uint16_t pat = 0);

	void line(int x, int y);
	void line(int x0, int y0, int x1, int y1);
	void line(int x0, int y0, int x1, int y1, uint16_t c, uint16_t pat = 0);

	void map(int cell_x, int cell_y);
	void map(int cell_x, int cell_y, int scr_x, int scr_y);
	void map(int cell_x, int cell_y, int scr_x, int scr_y, int cell_w, int cell_h);
	void map(int cell_x, int cell_y, int scr_x, int scr_y, int cell_w, int cell_h, uint8_t layer);
	uint8_t mget(int x, int y);
	void mset(int x, int y, uint8_t v);

	void cursor(int x, int y);
	void cursor(int x, int y, uint16_t c);
	void print(std::string str);
	void print(std::string str, int x, int y);
	void print(std::string str, int x, int y, uint16_t c);

	void camera();
	void camera(int x, int y);

	void clip(int x, int y, int w, int h);
	void clip();

	void fillp(int pattern, bool transparent);
	void fillp();

	uint8_t gfx_peek(uint16_t a);
	void gfx_poke(uint16_t a, uint8_t v);

}  // namespace pico_api

namespace pico_apix {
	void xpal(bool enable);
	void gfxstate(int index);
}  // namespace pico_apix

namespace pico_control {
	void gfx_init();
	void set_backbuffer(pico_api::colour_t* buffer, int width, int height, int stride);
	void set_spritebuffer(pico_api::colour_t* buffer);
	void set_spriteflags(uint8_t* buffer);
	void set_mapbuffer(uint8_t* buffer);
	void set_fontbuffer(pico_api::colour_t* buffer);
}  // namespace pico_control

#endif /* PICO_GFX_H */
