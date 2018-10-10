#include "hal_core.h"
#include <iostream>
#include <string>

static SDL_Window* sdlWin = nullptr;
static SDL_Renderer* sdlRen = nullptr;
static SDL_Texture* sdlTex = nullptr;
static SDL_PixelFormat* sdlPixFmt = nullptr;
static pixel_t palette[256] = {0};

static void throw_error(std::string msg) {
	msg += SDL_GetError();
	throw(gfx_exception(msg));
}

void GFX_Init(int x, int y) {
	if (SDL_Init(SDL_INIT_VIDEO) != 0) {
		throw_error("SDL_Init Error: ");
	}

	sdlWin = SDL_CreateWindow("thing!", 100, 100, x, y, SDL_WINDOW_SHOWN);
	if (sdlWin == nullptr) {
		throw_error("SDL_CreateWindow Error: ");
	}

	sdlRen = SDL_CreateRenderer(sdlWin, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	if (sdlRen == nullptr) {
		throw_error("SDL_CreateRenderer Error: ");
	}
	SDL_ShowCursor(SDL_DISABLE);
}

void GFX_End() {
	if (sdlRen) {
		SDL_DestroyRenderer(sdlRen);
	}
	if (sdlWin) {
		SDL_DestroyWindow(sdlWin);
	}
	if (sdlPixFmt) {
		SDL_FreeFormat(sdlPixFmt);
	}
	if (sdlTex) {
		SDL_DestroyTexture(sdlTex);
	}
	SDL_Quit();
}

void GFX_CreateBackBuffer(int x, int y) {
	sdlTex = SDL_CreateTexture(sdlRen, SDL_PIXELFORMAT_RGB565, SDL_TEXTUREACCESS_STREAMING, x, y);
	if (sdlTex == nullptr) {
		throw_error("SDL_CreateTexture Error: ");
	}

	sdlPixFmt = SDL_AllocFormat(SDL_PIXELFORMAT_RGB565);
	palette[0] = GFX_GetPixel(0, 0, 0);
	palette[1] = GFX_GetPixel(29, 43, 83);
	palette[2] = GFX_GetPixel(126, 37, 83);
	palette[3] = GFX_GetPixel(0, 135, 81);
	palette[4] = GFX_GetPixel(171, 82, 54);
	palette[5] = GFX_GetPixel(95, 87, 79);
	palette[6] = GFX_GetPixel(194, 195, 199);
	palette[7] = GFX_GetPixel(255, 241, 232);
	palette[8] = GFX_GetPixel(255, 0, 77);
	palette[9] = GFX_GetPixel(255, 163, 0);
	palette[10] = GFX_GetPixel(255, 240, 36);
	palette[11] = GFX_GetPixel(0, 231, 86);
	palette[12] = GFX_GetPixel(41, 173, 255);
	palette[13] = GFX_GetPixel(131, 118, 156);
	palette[14] = GFX_GetPixel(255, 119, 168);
	palette[15] = GFX_GetPixel(255, 204, 170);
}

pixel_t GFX_GetPixel(uint8_t r, uint8_t g, uint8_t b) {
	return (pixel_t)SDL_MapRGB(sdlPixFmt, r, g, b);
}

void GFX_CopyBackBuffer(uint8_t* buffer, int buffer_w, int buffer_h) {
	pixel_t* pixels;
	int pitch;

	int res = SDL_LockTexture(sdlTex, NULL, (void**)&pixels, &pitch);
	if (res < 0) {
		throw_error("SDL_LockTexture Error: ");
	}
	for (int y = 0; y < buffer_h; y++) {  // TODO:: optimise loop???
		for (int x = 0; x < buffer_w; x++) {
			pixels[x] = palette[buffer[x]];
		}
		pixels += pitch / sizeof(pixel_t);
		buffer += buffer_w;
	}

	SDL_UnlockTexture(sdlTex);
}

void GFX_Flip() {
	SDL_RenderClear(sdlRen);
	SDL_RenderCopy(sdlRen, sdlTex, NULL, NULL);
	SDL_RenderPresent(sdlRen);
}

static uint8_t state = 0;
static int mouseWheel = 0;

void INP_ProcessInputEvents(const SDL_Event& ev) {
	if (ev.type == SDL_KEYDOWN || ev.type == SDL_KEYUP) {
		int mask = -1;
		switch (ev.key.keysym.sym) {
			case SDLK_LEFT:
				mask = 1;
				break;
			case SDLK_RIGHT:
				mask = 2;
				break;
			case SDLK_UP:
				mask = 4;
				break;
			case SDLK_DOWN:
				mask = 8;
				break;
			case SDLK_z:
				mask = 16;
				break;
			case SDLK_x:
				mask = 32;
				break;
			case SDLK_p:
				mask = 64;
				break;
		}

		if (mask != -1) {
			if (ev.type == SDL_KEYDOWN) {
				state = state | mask;
			}
			if (ev.type == SDL_KEYUP) {
				state = state & (~mask);
			}
		}
	} else if (ev.type == SDL_MOUSEWHEEL) {
		mouseWheel += ev.wheel.y;
	}
}

uint8_t INP_GetInputState() {
	return state;
}

uint32_t TIME_GetTicks() {
	return SDL_GetTicks();
}

MouseState INP_GetMouseState() {
	MouseState ms;
	int b = SDL_GetMouseState(&ms.x, &ms.y);
	ms.buttons = (b & SDL_BUTTON(SDL_BUTTON_LEFT)) ? 1 : 0;
	ms.buttons |= (b & SDL_BUTTON(SDL_BUTTON_RIGHT)) ? 2 : 0;
	ms.buttons |= (b & SDL_BUTTON(SDL_BUTTON_MIDDLE)) ? 4 : 0;

	ms.x /= 4;  // TODO: scale based on window size
	ms.y /= 4;
	ms.wheel = mouseWheel;
	mouseWheel = 0;
	return ms;
}
