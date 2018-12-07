#include <experimental/filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <set>

#include "pico_audio.h"
#include "pico_cart.h"
#include "pico_core.h"
#include "pico_script.h"

#include "utf8-util.h"

namespace fs = std::experimental::filesystem;

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
		std::string line;
		std::string cur_sect = "header";

		while (std::getline(s, line)) {  // TODO: handle errors

			if (valid_sections.find(line) != valid_sections.end()) {
				cur_sect = line;
				std::cout << line << std::endl;
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

	void load(std::string filename) {
		std::ifstream s(filename.c_str());
		if (!s) {
			throw error(std::string("failed to open cart file: ") + filename);
		}

		fs::path path(filename);
		cart["filename"] = filename;
		cart["base_path"] = path.remove_filename();
		cart["cart_name"] = path.filename().replace_extension("");

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
