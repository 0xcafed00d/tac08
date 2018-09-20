#ifndef GFX_CORE_H
#define GFX_CORE_H

#include <SDL2/SDL.h>
#include <stdint.h>
#include <stdexcept>

typedef std::runtime_error gfx_exception;

typedef uint16_t pixel_t;

void GFX_Init(int x, int y);
void GFX_End();

pixel_t GFX_GetPixel(uint8_t r, uint8_t g, uint8_t b);

void GFX_CreateBackBuffer(int x, int y);
void GFX_LockBackBuffer(pixel_t** pixels, int* pitch);
void GFX_UnlockBackBuffer();

void GFX_Flip();

#endif /* GFX_CORE_H */
