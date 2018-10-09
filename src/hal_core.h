#ifndef GFX_CORE_H
#define GFX_CORE_H

#include <SDL2/SDL.h>
#include <stdint.h>
#include <stdexcept>

struct gfx_exception : public std::runtime_error {
	using std::runtime_error::runtime_error;
};

typedef uint16_t pixel_t;

void GFX_Init(int x, int y);
void GFX_End();

pixel_t GFX_GetPixel(uint8_t r, uint8_t g, uint8_t b);

void GFX_CreateBackBuffer(int x, int y);
void GFX_LockBackBuffer(pixel_t** pixels, int* pitch);
void GFX_UnlockBackBuffer();

void GFX_Flip();

void INP_ProcessInputEvents(const SDL_Event& ev);
uint8_t INP_GetInputState();

uint32_t TIME_GetTicks();

struct MouseState {
	int x;
	int y;
	int buttons;
	int wheel;
};

MouseState INP_GetMouseState();

#endif /* GFX_CORE_H */
