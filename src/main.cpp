#include <iostream>

#include "config.h"
#include "hal_audio.h"
#include "hal_core.h"
#include "pico_audio.h"
#include "pico_cart.h"
#include "pico_core.h"
#include "pico_data.h"
#include "pico_script.h"

int safe_main(int argc, char** argv) {
	GFX_Init(config::SCREEN_WIDTH * 4, config::SCREEN_HEIGHT * 4);
	AUDIO_Init();
	GFX_CreateBackBuffer(config::SCREEN_WIDTH, config::SCREEN_HEIGHT);
	pico_control::init(config::SCREEN_WIDTH, config::SCREEN_HEIGHT);
	pico_data::load_font_data();

	if (argc > 1) {
		pico_cart::load(argv[1]);
		pico_cart::extractCart(pico_cart::getCart());
	} else {
		std::cerr << "no cart specified" << std::endl;
		return 1;
	}

	bool init = false;

	uint32_t target_ticks = 20;
	uint32_t ticks = 0;

	uint32_t systemFrameCount = 0;
	uint32_t gameFrameCount = 0;
	uint32_t frameTimer = TIME_GetTime_ms();

	uint64_t updateTime = 0;
	uint64_t drawTime = 0;
	uint64_t copyBBTime = 0;

	while (EVT_ProcessEvents()) {
		using namespace pico_api;

		if ((TIME_GetTime_ms() - ticks) > target_ticks) {
			pico_control::frame_start();

			pico_control::set_input_state(INP_GetInputState());
			pico_control::set_mouse_state(INP_GetMouseState());

			if (!init) {
				pico_script::run("_init", true);
				init = true;
			}

			if (pico_control::is_pause_menu()) {
				if (pico_script::do_menu()) {
					pico_control::end_pause_menu();
				}
			} else {
				uint64_t updateTimeStart = TIME_GetProfileTime();
				if (!pico_script::run("_update", true)) {
					if (pico_script::run("_update60", true)) {
						target_ticks = 1;
					}
				}
				updateTime += TIME_GetElapsedProfileTime_us(updateTimeStart);

				uint64_t drawTimeStart = TIME_GetProfileTime();
				pico_script::run("_draw", true);
				drawTime += TIME_GetElapsedProfileTime_us(drawTimeStart);
			}

			int buffer_w;
			int buffer_h;
			pico_api::colour_t* buffer = pico_control::get_buffer(buffer_w, buffer_h);
			uint64_t copyBBStart = TIME_GetProfileTime();
			GFX_CopyBackBuffer(buffer, buffer_w, buffer_h);
			copyBBTime += TIME_GetElapsedProfileTime_us(copyBBStart);

			ticks = TIME_GetTime_ms();
			gameFrameCount++;
			pico_control::sound_tick();
			pico_control::frame_end();
		}
		systemFrameCount++;
		GFX_Flip();

		if (TIME_GetElapsedTime_ms(frameTimer) >= 1000) {
			updateTime /= systemFrameCount;
			drawTime /= systemFrameCount;
			copyBBTime /= systemFrameCount;
			/*
			            std::cout << "game FPS: " << gameFrameCount << " sys FPS: " <<
			   systemFrameCount
			                      << " update: " << updateTime / 1000.0f << "ms  draw: " << drawTime
			   / 1000.0f
			                      << "ms"
			                      << " bb copy: " << copyBBTime << "us" << std::endl;
			*/
			gameFrameCount = 0;
			systemFrameCount = 0;
			updateTime = 0;
			drawTime = 0;
			copyBBTime = 0;
			frameTimer = TIME_GetTime_ms();
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
