#include <iostream>

extern "C" {
#include <SDL2/SDL.h>
}

#include "hal_core.h"
#include "pico_core.h"
#include "pico_data.h"
#include "pico_script.h"

#include "game.h"

int safe_main(int, char**) {
	GFX_Init(512, 512);
	GFX_CreateBackBuffer(128, 128);
	pico_control::init(128, 128);
	pico_data::load_test_data();
	pico_data::load_font_data();
	pico_data::load_script();

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
					// pico_init();
					pico_script::run("_init");
					init = true;
				}

				// pico_update();
				// pico_draw();
				pico_script::run("_update");
				pico_script::run("_draw");

				GFX_UnlockBackBuffer();

				ticks = SDL_GetTicks();
			}
			GFX_Flip();
		}
	}
	GFX_End();
	return 0;
}

int main(int argc, char** argv) {
	try {
		return safe_main(argc, argv);
	} catch (gfx_exception& err) {
		std::cerr << err.what() << std::endl;
	} catch (pico_script::error& err) {
		std::cerr << err.what() << std::endl;
	}
}
