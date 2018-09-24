#include "game.h"
using namespace pico_api;

float bg_pos1;
float bg_pos2;
int x = 0;
int y = 0;

void pico_init() {
	bg_pos1 = 0;
	bg_pos2 = 0;
}

void update_bg() {
	bg_pos1 -= 1;
	bg_pos2 -= 1.5;
	if (bg_pos1 <= -128)
		bg_pos1 += 128;
	if (bg_pos2 <= -128)
		bg_pos2 += 128;
}

void pico_update() {
	update_bg();
	if (btn(0)) {
		x = x - 1;
	}
	if (btn(1)) {
		x = x + 1;
	}

	if (btn(2)) {
		y = y - 1;
	}
	if (btn(3)) {
		y = y + 1;
	}
}

void draw_bg1() {
	palt(0, false);
	map(0, 0, bg_pos1, 0, 16, 14);
	map(0, 0, bg_pos1 + 128, 0, 16, 14);
	palt(0, true);
}

void draw_bg2() {
	map(0, 14, floorf(bg_pos2), 112, 16, 2);
	map(0, 14, floorf(bg_pos2) + 128, 112, 16, 2);
}

void pico_draw() {
	draw_bg1();
	draw_bg2();
	pal(13, 5);
	spr(64, 9, 41, 14, 2);
	pal(13, 7);
	spr(64, 7, 39, 14, 2);
	pal(13, 9);
	spr(64, 8, 40, 14, 2);
	pal();
	print("press x or tap screen to start", 3, 70, 7);
	spr(1, x, y);
}
