#include <array>
#include <map>

#include "hal_palette.h"
#include "log.h"

// Pico8 palette
static PaletteInfo pico8_palette{
    {0x000000, 0x1d2b53, 0x7e2553, 0x008751, 0xab5236, 0x5f574f, 0xc2c3c7, 0xfff1e8, 0xff004d,
     0xffa300, 0xffec27, 0x00e436, 0x29adff, 0x83769c, 0xff77a8, 0xffccaa},
    16,
};

// AAP-64 PALETTE Created by Adigun A. Polack https://lospec.com/palette-list/aap-64
static PaletteInfo aap64_palette{
    {0x060608, 0x141013, 0x3b1725, 0x73172d, 0xb4202a, 0xdf3e23, 0xfa6a0a, 0xf9a31b,
     0xffd541, 0xfffc40, 0xd6f264, 0x9cdb43, 0x59c135, 0x14a02e, 0x1a7a3e, 0x24523b,
     0x122020, 0x143464, 0x285cc4, 0x249fde, 0x20d6c7, 0xa6fcdb, 0xffffff, 0xfef3c0,
     0xfad6b8, 0xf5a097, 0xe86a73, 0xbc4a9b, 0x793a80, 0x403353, 0x242234, 0x221c1a,
     0x322b28, 0x71413b, 0xbb7547, 0xdba463, 0xf4d29c, 0xdae0ea, 0xb3b9d1, 0x8b93af,
     0x6d758d, 0x4a5462, 0x333941, 0x422433, 0x5b3138, 0x8e5252, 0xba756a, 0xe9b5a3,
     0xe3e6ff, 0xb9bffb, 0x849be4, 0x588dbe, 0x477d85, 0x23674e, 0x328464, 0x5daf8d,
     0x92dcba, 0xcdf7e2, 0xe4d2aa, 0xc7b08b, 0xa08662, 0x796755, 0x5a4e44, 0x423934},
    64,
};

// ZX-Spectrum palette
static PaletteInfo zx_palette{
    {0x000000, 0x0022c7, 0x002bfb, 0xd62816, 0xff331c, 0xd433c7, 0xff40fc, 0x00c525, 0x00f92f,
     0x00c7c9, 0x00fbfe, 0xccc82a, 0xfffc36, 0xcacaca, 0xffffff},
    16,
};

// Alternative Gameboy palette by Andrade. https://lospec.com/palette-list/andrade-gameboy
static PaletteInfo gameboy_palette{
    {0x202020, 0x5e6745, 0xaeba89, 0xe3eec0},
    4,
};

static std::map<std::string, const PaletteInfo&> palette_map = {
    {"pico8", pico8_palette},
    {"aap64", aap64_palette},
    {"zx", zx_palette},
    {"gameboy", gameboy_palette},
};

const PaletteInfo& GFX_GetPaletteInfo(const std::string& name) {
	auto pal_itr = palette_map.find(name);
	// return default pico8 palette if name not found
	if (pal_itr == palette_map.end()) {
		logr << "palette: " << name << "not found loading default pico8 palette";
		return pico8_palette;
	}
	return pal_itr->second;
}
