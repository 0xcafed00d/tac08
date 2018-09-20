#include <iostream>

extern "C" {
#include <SDL2/SDL.h>
}

#include "gfx_core.h"
#include "pico_core.h"
#include "pico_data.h"

int main(int, char**) {
	GFX_Init(512, 512);
	GFX_CreateBackBuffer(128, 128);
	pico_control::init(128, 128);
	pico_data::load_test_data();

	while (true) {
		SDL_Event e;
		if (SDL_PollEvent(&e)) {
			if (e.type == SDL_QUIT) {
				break;
			}
		} else {
			using namespace pico_api;

			pixel_t* pixels;
			int pitch;
			GFX_LockBackBuffer(&pixels, &pitch);
			pico_control::set_buffer(pixels, pitch);

			cls(1);
			map(0, 0, 0, 0, 16, 16);

			pal(13, 5);
			spr(64, 9, 41, 14, 2);
			pal(13, 7);
			spr(64, 7, 39, 14, 2);
			pal(13, 9);
			spr(64, 8, 40, 14, 2);
			pal();

			GFX_UnlockBackBuffer();
			GFX_Flip();
		}
	}
	GFX_End();
}
