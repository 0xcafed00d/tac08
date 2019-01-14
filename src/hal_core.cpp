#include "hal_core.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_clipboard.h>
#include <SDL2/SDL_rwops.h>
#include <array>
#include <iostream>
#include <string>

#include "config.h"

static SDL_Window* sdlWin = nullptr;
static SDL_Renderer* sdlRen = nullptr;
static SDL_Texture* sdlTex = nullptr;
static SDL_PixelFormat* sdlPixFmt = nullptr;
static SDL_Joystick* joystick = nullptr;
static int screenWidth = config::INIT_SCREEN_WIDTH;
static int screenHeight = config::INIT_SCREEN_HEIGHT;

static std::array<pixel_t, 256> original_palette;
static std::array<pixel_t, 256> palette;

const size_t palette_sz = 16;
uint32_t palette_rgb[palette_sz] = {0x000000, 0x1d2b53, 0x7e2553, 0x008751, 0xab5236, 0x5f574f,
                                    0xc2c3c7, 0xfff1e8, 0xff004d, 0xffa300, 0xffec27, 0x00e436,
                                    0x29adff, 0x83769c, 0xff77a8, 0xffccaa};

static void throw_error(std::string msg) {
	msg += SDL_GetError();
	throw(gfx_exception(msg));
}

void GFX_Init(int x, int y) {
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_AUDIO) != 0) {
		throw_error("SDL_Init Error: ");
	}

	sdlWin = SDL_CreateWindow("tac08", 100, 100, x, y, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
	if (sdlWin == nullptr) {
		throw_error("SDL_CreateWindow Error: ");
	}

	sdlRen = SDL_CreateRenderer(sdlWin, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	if (sdlRen == nullptr) {
		throw_error("SDL_CreateRenderer Error: ");
	}
	SDL_ShowCursor(SDL_DISABLE);

	if (SDL_NumJoysticks() > 0) {
		joystick = SDL_JoystickOpen(0);
		if (joystick) {
			std::cout << "name: " << SDL_JoystickName(joystick) << std::endl;
			std::cout << "buttons: " << SDL_JoystickNumButtons(joystick) << std::endl;
			std::cout << "axis: " << SDL_JoystickNumAxes(joystick) << std::endl;
		}
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

pixel_t GFX_GetPixel(uint8_t r, uint8_t g, uint8_t b) {
	return (pixel_t)SDL_MapRGB(sdlPixFmt, r, g, b);
}

void GFX_CreateBackBuffer(int x, int y) {
	screenWidth = x;
	screenHeight = y;

	sdlTex = SDL_CreateTexture(sdlRen, SDL_PIXELFORMAT_RGB565, SDL_TEXTUREACCESS_STREAMING,
	                           config::MAX_SCREEN_WIDTH, config::MAX_SCREEN_HEIGHT);
	if (sdlTex == nullptr) {
		throw_error("SDL_CreateTexture Error: ");
	}

	sdlPixFmt = SDL_AllocFormat(SDL_PIXELFORMAT_RGB565);
	for (size_t i = 0; i < palette_sz; i++) {
		auto pal = palette_rgb[i];
		original_palette[i] = GFX_GetPixel((pal >> 16) & 0xff, (pal >> 8) & 0xff, pal & 0xff);
	}

	GFX_RestorePalette();
}

void GFX_SetBackBufferSize(int x, int y) {
	screenWidth = x;
	screenHeight = y;
}

void GFX_RestorePalette() {
	palette = original_palette;
}

void GFX_RestorePaletteIndex(uint8_t i) {
	palette[i] = original_palette[i];
}

void GFX_SetPaletteIndex(uint8_t i, uint8_t r, uint8_t g, uint8_t b) {
	palette[i] = GFX_GetPixel(r, g, b);
}

void GFX_CopyBackBuffer(uint8_t* buffer, int buffer_w, int buffer_h) {
	pixel_t* pixels;
	int pitch;

	int res = SDL_LockTexture(sdlTex, NULL, (void**)&pixels, &pitch);
	if (res < 0) {
		throw_error("SDL_LockTexture Error: ");
	}
	for (int y = 0; y < buffer_h; y++) {
		for (int x = 0; x < buffer_w; x++) {
			pixels[x] = palette[buffer[x]];
			x++;
			pixels[x] = palette[buffer[x]];
		}
		pixels += pitch / sizeof(pixel_t);
		buffer += buffer_w;
	}

	SDL_UnlockTexture(sdlTex);
}

static SDL_Rect getDisplayArea(SDL_Window* win, double* scale = nullptr) {
	int winx, winy;
	SDL_GetWindowSize(win, &winx, &winy);

	SDL_Rect r = {0, 0, winx, winy};
	double xscale = (double)winx / (double)screenWidth;
	double yscale = (double)winy / (double)screenHeight;

	if (xscale * screenHeight > winy) {
		r.w = yscale * screenWidth;
		r.x = winx / 2 - r.w / 2;
		if (scale)
			*scale = yscale;
	} else {
		r.h = xscale * screenHeight;
		r.y = winy / 2 - r.h / 2;
		if (scale)
			*scale = xscale;
	}
	return r;
}

void GFX_Flip() {
	SDL_RenderClear(sdlRen);
	SDL_Rect dr = getDisplayArea(sdlWin);
	SDL_Rect sr = {0, 0, screenWidth, screenHeight};
	SDL_RenderCopy(sdlRen, sdlTex, &sr, &dr);
	SDL_RenderPresent(sdlRen);
}

static uint8_t keyState = 0;
static uint8_t joyState = 0;
static int mouseWheel = 0;

static inline void set_state_bit(uint8_t& state, uint8_t bit, bool condition, bool value) {
	if (condition) {
		if (value) {
			state |= (1 << bit);
		} else {
			state &= ~(1 << bit);
		}
	}
}

void INP_ProcessInputEvents(const SDL_Event& ev) {
	if (ev.type == SDL_KEYDOWN || ev.type == SDL_KEYUP) {
		set_state_bit(keyState, 0, ev.key.keysym.sym == SDLK_LEFT, ev.type == SDL_KEYDOWN);
		set_state_bit(keyState, 1, ev.key.keysym.sym == SDLK_RIGHT, ev.type == SDL_KEYDOWN);
		set_state_bit(keyState, 2, ev.key.keysym.sym == SDLK_UP, ev.type == SDL_KEYDOWN);
		set_state_bit(keyState, 3, ev.key.keysym.sym == SDLK_DOWN, ev.type == SDL_KEYDOWN);
		set_state_bit(keyState, 4, ev.key.keysym.sym == SDLK_z, ev.type == SDL_KEYDOWN);
		set_state_bit(keyState, 5, ev.key.keysym.sym == SDLK_x, ev.type == SDL_KEYDOWN);
		set_state_bit(keyState, 6, ev.key.keysym.sym == SDLK_p, ev.type == SDL_KEYDOWN);
		set_state_bit(keyState, 6, ev.key.keysym.sym == SDLK_RETURN, ev.type == SDL_KEYDOWN);
		set_state_bit(keyState, 7, ev.key.keysym.sym == SDLK_ESCAPE, ev.type == SDL_KEYDOWN);
	} else if (ev.type == SDL_MOUSEWHEEL) {
		mouseWheel += ev.wheel.y;
	} else if (ev.type == SDL_JOYAXISMOTION) {
		// std::cout << "axis: " << (int)ev.jaxis.axis << "=" << ev.jaxis.value << std::endl;
		set_state_bit(joyState, 0, ev.jaxis.axis == 0, ev.jaxis.value < -500);
		set_state_bit(joyState, 1, ev.jaxis.axis == 0, ev.jaxis.value > 500);
		set_state_bit(joyState, 2, ev.jaxis.axis == 1, ev.jaxis.value < -500);
		set_state_bit(joyState, 3, ev.jaxis.axis == 1, ev.jaxis.value > 500);
	} else if (ev.type == SDL_JOYBUTTONDOWN || ev.type == SDL_JOYBUTTONUP) {
		// std::cout << "btn: " << (int)ev.jbutton.button << "=" << (bool)ev.jbutton.state <<
		// std::endl;
		set_state_bit(joyState, 4, ev.jbutton.button == 1, (bool)ev.jbutton.state);
		set_state_bit(joyState, 5, ev.jbutton.button == 0, (bool)ev.jbutton.state);
		set_state_bit(joyState, 6, ev.jbutton.button == 7, (bool)ev.jbutton.state);
	}
}

bool EVT_ProcessEvents() {
	SDL_Event e;
	while (SDL_PollEvent(&e)) {
		if (e.type == SDL_QUIT) {
			return false;
		} else {
			INP_ProcessInputEvents(e);
		}
	}
	return true;
}

uint8_t INP_GetInputState() {
	return keyState | joyState;
}

uint32_t TIME_GetTime_ms() {
	return SDL_GetTicks();
}

uint32_t TIME_GetElapsedTime_ms(uint32_t start) {
	return SDL_GetTicks() - start;
}

uint64_t TIME_GetProfileTime() {
	return SDL_GetPerformanceCounter();
}

uint64_t TIME_GetElapsedProfileTime_us(uint64_t start) {
	uint64_t now = SDL_GetPerformanceCounter();
	return ((now - start) * 1000000) / SDL_GetPerformanceFrequency();
}

uint64_t TIME_GetElapsedProfileTime_ms(uint64_t start) {
	uint64_t now = SDL_GetPerformanceCounter();
	return ((now - start) * 1000) / SDL_GetPerformanceFrequency();
}

static void scaleMouse(int& x, int& y) {
	double scale;
	SDL_Rect r = getDisplayArea(sdlWin, &scale);
	x -= r.x;
	y -= r.y;
	x = x / scale;
	y = y / scale;
}

MouseState INP_GetMouseState() {
	MouseState ms;

	int b = SDL_GetMouseState(&ms.x, &ms.y);
	scaleMouse(ms.x, ms.y);

	ms.buttons = (b & SDL_BUTTON(SDL_BUTTON_LEFT)) ? 1 : 0;
	ms.buttons |= (b & SDL_BUTTON(SDL_BUTTON_RIGHT)) ? 2 : 0;
	ms.buttons |= (b & SDL_BUTTON(SDL_BUTTON_MIDDLE)) ? 4 : 0;

	ms.wheel = mouseWheel;
	mouseWheel = 0;
	return ms;
}

std::string FILE_LoadFile(std::string name) {
	std::string data;
	SDL_RWops* file = SDL_RWFromFile(name.c_str(), "r");
	if (file) {
		size_t sz = SDL_RWsize(file);
		if (sz) {
			data.resize(sz, ' ');
			SDL_RWread(file, &data[0], sz, 1);
		}
		SDL_RWclose(file);
	}

	return data;
}

std::string FILE_LoadGameState(std::string name) {
	const char* path = SDL_GetPrefPath("simulatedsimian", "tac08");
	name = std::string(path) + name;
	SDL_free((void*)path);

	return FILE_LoadFile(name);
}

void FILE_SaveGameState(std::string name, const std::string& data) {
	const char* path = SDL_GetPrefPath("simulatedsimian", "tac08");
	name = std::string(path) + name;
	SDL_free((void*)path);

	SDL_RWops* file = SDL_RWFromFile(name.c_str(), "w");
	if (file) {
		SDL_RWwrite(file, data.c_str(), data.length(), 1);
	}
	SDL_RWclose(file);
}

std::string FILE_ReadClip() {
	std::string res;
	if (SDL_HasClipboardText()) {
		auto t = SDL_GetClipboardText();
		if (t) {
			res = t;
			SDL_free(t);
		}
	}
	return res;
}

void FILE_WriteClip(const std::string& data) {
	SDL_SetClipboardText(data.c_str());
}
