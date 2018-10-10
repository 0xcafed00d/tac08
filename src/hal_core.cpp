#include "hal_core.h"
#include <iostream>
#include <string>

static SDL_Window* sdlWin = nullptr;
static SDL_Renderer* sdlRen = nullptr;
static SDL_Texture* sdlTex = nullptr;
static SDL_PixelFormat* sdlPixFmt = nullptr;
static SDL_Joystick* joystick = nullptr;
static pixel_t palette[256] = {0};

static void throw_error(std::string msg) {
	msg += SDL_GetError();
	throw(gfx_exception(msg));
}

void GFX_Init(int x, int y) {
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK) != 0) {
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

uint8_t INP_GetInputState() {
	return keyState | joyState;
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
