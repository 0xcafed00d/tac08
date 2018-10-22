#include <iostream>

#include <SDL2/SDL.h>

#include "hal_core.h"
#include "pico_cart.h"
#include "pico_core.h"
#include "pico_data.h"
#include "pico_script.h"

int safe_main(int argc, char** argv) {
	GFX_Init(512, 512);
	GFX_CreateBackBuffer(128, 128);
	pico_control::init(128, 128);
	pico_data::load_font_data();

	if (argc > 1) {
		pico_cart::load(argv[1]);
	} else {
		std::cerr << "no cart specified" << std::endl;
		return 1;
	}

	bool init = false;

	uint32_t ticks = 0;

	while (true) {
		SDL_Event e;
		if (SDL_PollEvent(&e)) {
			if (e.type == SDL_QUIT) {
				break;
			} else {
				INP_ProcessInputEvents(e);
			}
		} else {
			using namespace pico_api;

			if ((SDL_GetTicks() - ticks) > 20) {
				pico_control::set_input_state(INP_GetInputState());
				pico_control::set_mouse_state(INP_GetMouseState());

				if (!init) {
					pico_script::run("_init", true);
					init = true;
				}

				pico_script::run("_update", true);
				pico_script::run("_draw", true);

				int buffer_w;
				int buffer_h;
				pico_api::colour_t* buffer = pico_control::get_buffer(buffer_w, buffer_h);
				GFX_CopyBackBuffer(buffer, buffer_w, buffer_h);

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
	} catch (pico_cart::error& err) {
		std::cerr << err.what() << std::endl;
	}
}
