#ifndef GFX_CORE_H
#define GFX_CORE_H

#include <stdint.h>
#include <stdexcept>

struct gfx_exception : public std::runtime_error {
	using std::runtime_error::runtime_error;
};

typedef uint16_t pixel_t;

void GFX_Init(int x, int y);
void GFX_End();

void GFX_CreateBackBuffer(int x, int y);
void GFX_CopyBackBuffer(uint8_t* buffer, int buffer_w, int buffer_h);
void GFX_CreateBackBuffer(int x, int y);
void GFX_SetBackBufferSize(int x, int y);

void GFX_Flip();

void GFX_RestorePalette();
void GFX_RestorePaletteIndex(uint8_t i);
void GFX_SetPaletteIndex(uint8_t i, uint8_t r, uint8_t g, uint8_t b);

std::string FILE_LoadFile(std::string name);
std::string FILE_LoadGameState(std::string name);
void FILE_SaveGameState(std::string name, const std::string& data);
std::string FILE_ReadClip();
void FILE_WriteClip(const std::string& data);

bool EVT_ProcessEvents();
uint8_t INP_GetInputState();

uint32_t TIME_GetTime_ms();
uint32_t TIME_GetElapsedTime_ms(uint32_t start);

uint64_t TIME_GetProfileTime();
uint64_t TIME_GetElapsedProfileTime_us(uint64_t start);
uint64_t TIME_GetElapsedProfileTime_ms(uint64_t start);

void SYSLOG_LogMessage(const char* msg);

struct MouseState {
	int x;
	int y;
	int buttons;
	int wheel;
};

MouseState INP_GetMouseState();

#endif /* GFX_CORE_H */
