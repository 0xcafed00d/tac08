#include <fstream>
#include <map>
#include <set>
#include <sstream>

#include "pico_cart.h"

#include "hal_core.h"
#include "log.h"
#include "pico_audio.h"
#include "pico_core.h"
#include "pico_script.h"
#include "utf8-util.h"
#include "utils.h"

std::map<char32_t, uint8_t> emoji = {
    {0x2588, 0x80}, {0x2592, 0x81}, {0x1f431, 0x82}, {0x2b07, 0x83}, {0x2591, 0x84},
    {0x273d, 0x85}, {0x25cf, 0x86}, {0x2665, 0x87},  {0x2609, 0x88}, {0xc6c3, 0x89},
    {0x2302, 0x8a}, {0x2b05, 0x8b}, {0x1f610, 0x8c}, {0x266a, 0x8d}, {0x1f17e, 0x8e},
    {0x25c6, 0x8f}, {0x2026, 0x90}, {0x27a1, 0x91},  {0x2605, 0x92}, {0x29d7, 0x93},
    {0x2b06, 0x94}, {0x2c7, 0x95},  {0x2227, 0x96},  {0x274e, 0x97}, {0x25a4, 0x98},
    {0x25a5, 0x99}};

namespace pico_cart {
	typedef std::map<std::string, std::string> sections;

	static std::set<std::string> valid_sections = {"__lua__", "__gfx__",   "__gff__",  "__map__",
	                                               "__sfx__", "__music__", "__label__"};

	void do_load(std::istream& s, sections& sect) {
		path::test();

		std::string line;
		std::string cur_sect = "header";

		while (std::getline(s, line)) {  // TODO: handle errors

			if (valid_sections.find(line) != valid_sections.end()) {
				cur_sect = line;
			} else {
				sect[cur_sect] += line + "\n";
			}
		}
	}

	std::string getCartName() {
		return "";
	}

	std::string getCartPath() {
		return "";
	}

	std::string convert_emojis(std::string& lua) {
		std::string res;
		for (char32_t codepoint : utf8::CodepointIterator(lua)) {
			if (codepoint < 0x80) {
				res.push_back(uint8_t(codepoint));
			} else {
				auto i = emoji.find(codepoint);
				if (i != emoji.end()) {
					res.push_back(i->second);
				}
			}
		}
		return res;
	}

	static sections cart;

	sections& getCart() {
		return cart;
	}

	// if the filename begins with a $ then the path of the new cart of relative to the one callng
	// load
	void load(std::string filename) {
		logr << "Request cart load: " << filename;

		filename = path::normalisePath(filename);

		if (cart.find("base_path") != cart.end() && filename.length() && filename[0] == '$') {
			filename = cart["base_path"] + filename.substr(1);
		}

		std::string data = FILE_LoadFile(filename);
		if (data.size() == 0) {
			throw error(std::string("failed to open cart file: ") + filename);
		}

		logr << "Loading cart: " << filename;

		cart["filename"] = filename;
		cart["base_path"] = path::getPath(filename);
		cart["cart_name"] = path::splitFilename(path::getFilename(filename)).first;

		std::istringstream s(data);
		do_load(s, cart);
	}

	void extractCart(sections& sect) {
		pico_control::set_sprite_data(sect["__gfx__"], sect["__gff__"]);
		pico_control::set_map_data(sect["__map__"]);
		pico_control::set_music_from_cart(sect["__music__"]);
		pico_control::set_sfx_from_cart(sect["__sfx__"]);
		pico_script::load(convert_emojis(sect["__lua__"]));
	}

}  // namespace pico_cart
