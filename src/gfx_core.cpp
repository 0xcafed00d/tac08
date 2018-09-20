#include "gfx_core.h"
#include <string>

static SDL_Window* sdlWin = nullptr;
static SDL_Renderer* sdlRen = nullptr;
static SDL_Texture* sdlTex = nullptr;
static SDL_PixelFormat* sdlPixFmt = nullptr;

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
}

pixel_t GFX_GetPixel(uint8_t r, uint8_t g, uint8_t b) {
	return (pixel_t)SDL_MapRGB(sdlPixFmt, r, g, b);
}

void GFX_LockBackBuffer(pixel_t** pixels, int* pitch) {
	int res = SDL_LockTexture(sdlTex, NULL, (void**)pixels, pitch);
	if (res < 0) {
		throw_error("SDL_LockTexture Error: ");
	}
}

void GFX_UnlockBackBuffer() {
	SDL_UnlockTexture(sdlTex);
}

void GFX_Flip() {
	SDL_RenderClear(sdlRen);
	SDL_RenderCopy(sdlRen, sdlTex, NULL, NULL);
	SDL_RenderPresent(sdlRen);
}
