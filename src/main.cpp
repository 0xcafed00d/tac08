#include <iostream>

extern "C" {
#include <SDL2/SDL.h>
}

#include "hal_core.h"
#include "pico_core.h"
#include "pico_data.h"

#include "game.h"

int main(int, char**) {
	GFX_Init(512, 512);
	GFX_CreateBackBuffer(128, 128);
	pico_control::init(128, 128);
	pico_data::load_test_data();
	pico_data::load_font_data();

	bool init = false;

	uint32_t ticks = 0;

	while (true) {
		SDL_Event e;
		if (SDL_PollEvent(&e)) {
			if (e.type == SDL_QUIT) {
				break;
			} else if (e.type == SDL_KEYDOWN || e.type == SDL_KEYUP) {
				INP_ProcessInputEvents(e);
			}
		} else {
			using namespace pico_api;

			if ((SDL_GetTicks() - ticks) > 20) {
				pixel_t* pixels;
				int pitch;
				GFX_LockBackBuffer(&pixels, &pitch);
				pico_control::set_buffer(pixels, pitch);
				pico_control::set_input_state(INP_GetInputState());

				if (!init) {
					pico_init();
					init = true;
				}

				pico_update();
				pico_draw();
				GFX_UnlockBackBuffer();

				ticks = SDL_GetTicks();
			}
			GFX_Flip();
		}
	}
	GFX_End();
}
