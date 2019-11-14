#include "pico_gfx.h"
#include "utils.h"

#include <string.h>
#include <array>
#include <map>

#include "hal_core.h"

static pico_api::colour_t* backbuffer = nullptr;
static int buffer_size_x = 0;
static int buffer_size_y = 0;
static int buffer_stride = 0;

static pico_api::colour_t* spritebuffer = nullptr;
static uint8_t* spriteflags = nullptr;
static uint8_t* mapbuffer = nullptr;

static pico_api::colour_t* fontbuffer = nullptr;

struct GraphicsState {
	pico_api::colour_t fg = 7;
	pico_api::colour_t bg = 0;
	uint16_t pattern = 0;
	bool pattern_transparent = false;
	bool pattern_with_colour = false;
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

namespace pico_private {
	using namespace pico_api;

	static void restore_palette() {
		for (size_t n = 0; n < currentGraphicsState->palette_map.size(); n++) {
			currentGraphicsState->palette_map[n] = (colour_t)n;
		}
		GFX_RestorePalette();
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

	static void blitter(colour_t* spritebuffer,
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
			colour_t* spr = spritebuffer + ((spr_y + y * dy) & 0x7f) * 128;

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

	static void stretch_blitter(colour_t* spritebuffer,
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
			blitter(spritebuffer, scr_x, scr_y, spr_x, spr_y, scr_w, scr_h, flip_x, flip_y);
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
			colour_t* spr = spritebuffer + (((spr_y + y * dy) >> 16) & 0x7f) * 128;

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
					*pix = fg;
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

namespace pico_api {
	void color(uint16_t c) {
		currentGraphicsState->fg = pico_private::fgcolor(c);
		currentGraphicsState->bg = pico_private::bgcolor(c);
	}

	void cls(colour_t c) {
		colour_t p = currentGraphicsState->palette_map[c];
		memset(backbuffer, p, buffer_size_x * buffer_size_y);

		currentGraphicsState->text_x = 0;
		currentGraphicsState->text_y = 0;
	}

	void cls() {
		cls(0);
	}

	uint8_t fget(int n) {
		return spriteflags[n & 0xff];
	}

	bool fget(int n, int bit) {
		return (fget(n) >> bit) & 1;
	}

	void fset(int n, uint8_t val) {
		spriteflags[n & 0xff] = val;
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
		pico_private::blitter(spritebuffer, x, y, spr_x, spr_y, w * 8, h * 8, flip_x, flip_y);
	}

	void sspr(int sx, int sy, int sw, int sh, int dx, int dy) {
		pico_private::apply_camera(dx, dy);
		pico_private::blitter(spritebuffer, dx, dy, sx, sy, sw, sh);
	}

	void sspr(int sx, int sy, int sw, int sh, int dx, int dy, int dw, int dh) {
		pico_private::apply_camera(dx, dy);
		pico_private::stretch_blitter(spritebuffer, sx, sy, sw, sh, dx, dy, dw, dh);
	}

	void
	sspr(int sx, int sy, int sw, int sh, int dx, int dy, int dw, int dh, bool flip_x, bool flip_y) {
		pico_private::apply_camera(dx, dy);
		pico_private::stretch_blitter(spritebuffer, sx, sy, sw, sh, dx, dy, dw, dh, flip_x, flip_y);
	}

	colour_t sget(int x, int y) {
		y &= 0x7f;
		x &= 0x7f;
		return spritebuffer[y * 128 + x];
	}

	void sset(int x, int y) {
		sset(x, y, currentGraphicsState->fg);
	}

	void sset(int x, int y, colour_t c) {
		y &= 0x7f;
		x &= 0x7f;
		spritebuffer[y * 128 + x] = c;
	}

	void pset(int x, int y) {
		pset(x, y, currentGraphicsState->fg);
	}

	void pset(int x, int y, uint16_t c, uint16_t pat) {
		if (currentGraphicsState->pattern_with_colour) {
			fillp(pat, false);
		}
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

	void rect(int x0, int y0, int x1, int y1, uint16_t c, uint16_t pat) {
		if (currentGraphicsState->pattern_with_colour) {
			fillp(pat, false);
		}
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

	void rectfill(int x0, int y0, int x1, int y1, uint16_t c, uint16_t p) {
		using namespace pico_private;
		if (currentGraphicsState->pattern_with_colour) {
			fillp(p, false);
		}

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

	void circ(int xm, int ym, int r, uint16_t c, uint16_t pat) {
		if (currentGraphicsState->pattern_with_colour) {
			fillp(pat, false);
		}
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

	void circfill(int xm, int ym, int r, uint16_t c, uint16_t pat) {
		if (currentGraphicsState->pattern_with_colour) {
			fillp(pat, false);
		}
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

	void line(int x0, int y0, int x1, int y1, uint16_t c, uint16_t pat) {
		if (currentGraphicsState->pattern_with_colour) {
			fillp(pat, false);
		}
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
		map(cell_x, cell_y, scr_x, scr_y, 128, 64);
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
		x &= 0x7f;
		y &= 0x3f;
		return mapbuffer[y * 128 + x];
	}

	void mset(int x, int y, uint8_t v) {
		x &= 0x7f;
		y &= 0x3f;
		mapbuffer[y * 128 + x] = v;
	}

	void pal(colour_t c0, colour_t c1, int p) {
		if (p) {
			if (!currentGraphicsState->extendedPalette) {
				c1 = (c1 & 0xf) | ((c1 & 0xf0) >> 3);
			}
			GFX_MapPaletteIndex(c0, c1);
		} else {
			currentGraphicsState->palette_map[c0 & 0xf] = c1 & 0xf;
		}
	}

	void pal(colour_t c0, colour_t c1) {
		pal(c0, c1, 0);
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
			if (ch >= 0x10 && ch < 0x80) {
				int index = ch - 0x10;
				pico_private::blitter(fontbuffer, x, y, (index % 16) * 8, (index / 16) * 8, 4, 5);
				x += 4;
			} else if (ch >= 0x80) {
				int index = ch - 0x80;
				pico_private::blitter(fontbuffer, x, y, (index % 16) * 8, (index / 16) * 8 + 56, 8,
				                      5);
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

	uint8_t gfx_peek(uint16_t a) {
		auto cg = currentGraphicsState;
		if (a >= 0x5f00 && a <= 0x5f0f) {  // pal
			if (cg->extendedPalette) {
				return cg->palette_map[a - 0x5f00];
			} else {
				return cg->palette_map[a - 0x5f00] + (cg->transparent[a - 0x5f00] ? 16 : 0);
			}
		}
		if (a >= 0x5f10 && a <= 0x5f1f) {
			// TODO:
			// screen pal
		}
		switch (a) {
			case 0x5f20:  // clip x1
				return uint8_t(cg->clip_x1);
			case 0x5f21:  // clip y1
				return uint8_t(cg->clip_y1);
			case 0x5f22:  // clip x2
				return uint8_t(cg->clip_x2);
			case 0x5f23:  // clip y2
				return uint8_t(cg->clip_y2);
			case 0x5f25:  // pen colour
				return (cg->fg & 0x0f) | ((cg->bg & 0x0f) << 4);
			case 0x5f26:  // cursor x
				return uint8_t(cg->text_x);
			case 0x5f27:  // cursor y
				return uint8_t(cg->text_x);
			case 0x5f28:  // camera x offset lo byte
				return uint8_t(cg->camera_x);
			case 0x5f29:  // camera x offset hi byte
				return uint8_t(cg->camera_x >> 8);
			case 0x5f2a:  // camera y offset lo byte
				return uint8_t(cg->camera_y);
			case 0x5f2b:  // camera y offset hi byte
				return uint8_t(cg->camera_y >> 8);
			case 0x5f2c:  // pixel double/mirror
				// TODO:
				return 0;
			case 0x5f2d:  // devkit mode
				return 0;
			case 0x5f2e:  // persist palette
				return 0;
			case 0x5f2f:  // pause music
				// TODO:
				return 0;
			case 0x5f30:  // supress pause menu
				// TODO:
				return 0;
			case 0x5f31:  // fill pattern lo byte
				return uint8_t(cg->pattern);
			case 0x5f32:  // fill pattern hi byte
				return uint8_t(cg->pattern >> 8);
			case 0x5f33:  // fill pattern transparency
				return uint8_t(cg->pattern_transparent);
			case 0x5f34:  // accept pattern in colour param
				return uint8_t(cg->pattern_with_colour);
			case 0x5f3c:  // line end x lo byte
				return uint8_t(cg->line_x);
			case 0x5f3d:  // line end x hi byte
				return uint8_t(cg->line_x >> 8);
			case 0x5f3e:  // line end y lo byte
				return uint8_t(cg->line_y);
			case 0x5f3f:  // line end y hi byte
				return uint8_t(cg->line_y >> 8);
		}
		return 0;
	}

	void gfx_poke(uint16_t a, uint8_t v) {
		auto cg = currentGraphicsState;

		if (a >= 0x5f00 && a <= 0x5f0f) {
			pico_api::pal(a - 0x5f00, v);
			if (!cg->extendedPalette) {
				palt(a - 0x5f00, (v & 0xf0) != 0);
			}
		}
		if (a >= 0x5f10 && a <= 0x5f1f) {
			pico_api::pal(a - 0x5f10, v, 1);
		}
		switch (a) {
			case 0x5f20:  // clip x1
				cg->clip_x1 = v;
				break;
			case 0x5f21:  // clip y1
				cg->clip_y1 = v;
				break;
			case 0x5f22:  // clip x2
				cg->clip_x2 = v;
				break;
			case 0x5f23:  // clip y2
				cg->clip_y2 = v;
				break;
			case 0x5f25:  // pen colour
				cg->fg = v & 0x0f;
				cg->bg = (v & 0x0f) >> 4;
				break;
			case 0x5f26:  // cursor x
				cg->text_x = v;
				break;
			case 0x5f27:  // cursor y
				cg->text_y = v;
				break;
			case 0x5f28:  // camera x offset lo byte
				cg->camera_x = (cg->camera_x & 0xff00) | v;
				break;
			case 0x5f29:  // camera x offset hi byte
				cg->camera_x = (cg->camera_x & 0x00ff) | (uint16_t(v) << 8);
				break;
			case 0x5f2a:  // camera y offset lo byte
				cg->camera_y = (cg->camera_y & 0xff00) | v;
				break;
			case 0x5f2b:  // camera y offset hi byte
				cg->camera_y = (cg->camera_y & 0x00ff) | (uint16_t(v) << 8);
				break;
			case 0x5f2c:  // pixel double/mirror
				// TODO:
				break;
			case 0x5f2d:  // devkit mode
				// TODO:
				break;
			case 0x5f2e:  // persist palette
				// TODO:
				break;
			case 0x5f2f:  // pause music
				// TODO:
				break;
			case 0x5f30:  // supress pause menu
				// TODO:
				break;
			case 0x5f31:  // fill pattern lo byte
				cg->pattern = (cg->pattern & 0xff00) | v;
				break;
			case 0x5f32:  // fill pattern hi byte
				cg->pattern = (cg->pattern & 0x00ff) | (uint16_t(v) << 8);
				break;
			case 0x5f33:  // fill pattern transparency
				cg->pattern_transparent = (v != 0);
				break;
			case 0x5f34:  // accept pattern with colour
				cg->pattern_with_colour = (v != 0);
				break;
			case 0x5f3c:  // line end x lo byte
				cg->line_x = (cg->line_x & 0xff00) | v;
				break;
			case 0x5f3d:  // line end x hi byte
				cg->line_x = (cg->line_x & 0x00ff) | (uint16_t(v) << 8);
				break;
			case 0x5f3e:  // line end y lo byte
				cg->line_y = (cg->line_y & 0xff00) | v;
				break;
			case 0x5f3f:  // line end y hi byte
				cg->line_y = (cg->line_y & 0x00ff) | (uint16_t(v) << 8);
				break;
		}
	}
}  // namespace pico_api

namespace pico_apix {

	void xpal(bool enable) {
		currentGraphicsState->extendedPalette = enable;
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
}  // namespace pico_apix

namespace pico_control {

	void gfx_init() {
		extendedGraphicsStates.clear();
		pico_apix::gfxstate(0);
	}

	void set_backbuffer(pico_api::colour_t* buffer, int width, int height, int stride) {
		backbuffer = buffer;
		buffer_size_x = width;
		buffer_size_y = height;
		buffer_stride = stride;
		currentGraphicsState->max_clip_x = width;
		currentGraphicsState->max_clip_y = height;
	}

	void set_spritebuffer(pico_api::colour_t* buffer) {
		spritebuffer = buffer;
	}

	void set_spriteflags(uint8_t* buffer) {
		spriteflags = buffer;
	}

	void set_mapbuffer(uint8_t* buffer) {
		mapbuffer = buffer;
	}

	void set_fontbuffer(pico_api::colour_t* buffer) {
		fontbuffer = buffer;
	}

}  // namespace pico_control
