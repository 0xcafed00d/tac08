#ifndef HAL_PALETTE_H
#define HAL_PALETTE_H

#include <stddef.h>
#include <stdint.h>
#include <array>
#include <string>

struct PaletteInfo {
	std::array<uint32_t, 256> pal;
	size_t size;
};

const PaletteInfo& GFX_GetPaletteInfo(const std::string& name);

#endif /* HAL_PALETTE_H */
