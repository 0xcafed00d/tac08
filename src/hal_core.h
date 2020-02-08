#ifndef GFX_CORE_H
#define GFX_CORE_H

#include <stdint.h>

#include <stdexcept>

#include "log.h"

struct gfx_exception : public std::runtime_error {
	using std::runtime_error::runtime_error;
};

typedef uint16_t pixel_t;

void checkmem();

void GFX_Init(int x, int y);
void GFX_End();

void GFX_CreateBackBuffer(int x, int y);
void GFX_CopyBackBuffer(uint8_t* buffer, int buffer_w, int buffer_h);
void GFX_SetBackBufferSize(int x, int y);

void GFX_Flip();

void GFX_SelectPalette(const std::string& name);

void GFX_MapPaletteIndex(uint8_t to, uint8_t from);
void GFX_RestorePaletteMapping();
void GFX_RestorePaletteMappingIndex(uint8_t i);

void GFX_RestorePaletteRGB();
void GFX_RestorePaletteRGBIndex(uint8_t i);
void GFX_SetPaletteRGBIndex(uint8_t i, uint8_t r, uint8_t g, uint8_t b);

void GFX_ShowHWMouse(bool show);
void GFX_GetDisplayArea(int* w, int* h);
void GFX_ToggleFullScreen();
void GFX_SetFullScreen(bool fullscreen);
void GFX_SetZoom(int x, int y, double factor);

std::string FILE_LoadFile(std::string name);
std::string FILE_LoadGameState(std::string name);
void FILE_SaveGameState(std::string name, std::string data);
std::string FILE_ReadClip();
void FILE_WriteClip(const std::string& data);
std::string FILE_GetDefaultCartName();

bool EVT_ProcessEvents();
uint8_t INP_GetInputState();
void INP_SetSimState(uint8_t state);

uint32_t TIME_GetTime_ms();
uint32_t TIME_GetElapsedTime_ms(uint32_t start);

uint64_t TIME_GetProfileTime();
uint64_t TIME_GetElapsedProfileTime_us(uint64_t start);
uint64_t TIME_GetElapsedProfileTime_ms(uint64_t start);
void TIME_Sleep(int ms);

void SYSLOG_LogMessage(LogLevel l, const char* msg);

struct MouseState {
	int x;
	int y;
	int buttons;
	int wheel;
};

MouseState INP_GetMouseState();

struct TouchInfo {
	enum State { None = 0, JustPressed = 1, Pressed = 2, JustReleased = 4 };
	int x = 0;
	int y = 0;
	int state = None;
};

bool INP_TouchAvailable();
uint8_t INP_GetTouchMask();
TouchInfo INP_GetTouchInfo(int idx);
std::string INP_GetKeyPress();

void HAL_StartFrame();
void HAL_EndFrame();
void HAL_SetFrameRates(uint32_t target, uint32_t actual, uint32_t sys, uint32_t cpu);
uint32_t HAL_GetFrameRate(char fps_type);  // 't' = target, 'a' = actual, 's' = sys, 'c' = cpu

void PLATFORM_OpenURL(std::string url);

bool DEBUG_Trace();
void DEBUG_Trace(bool enable);
bool DEBUG_ReloadRequested();

#endif /* GFX_CORE_H */
