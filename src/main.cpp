#include <iostream>

extern "C" {
#include <SDL2/SDL.h>
}

#include "gfx_core.h"

int main(int, char**) {
	GFX_Init(512, 512);
	GFX_CreateBackBuffer(128, 128);

	int n = 0;

	while (true) {
		SDL_Event e;
		if (SDL_PollEvent(&e)) {
			if (e.type == SDL_QUIT) {
				break;
			}
		} else {
			pixel_t* pixels;
			int pitch;
			GFX_LockBackBuffer(&pixels, &pitch);
			memset(pixels, n++, 128 * 128);
			GFX_UnlockBackBuffer();
			GFX_Flip();
		}
	}
	GFX_End();
}
