#include <assert.h>
#include <algorithm>
#include <fstream>
#include <map>
#include <set>
#include <sstream>

#include "hal_core.h"
#include "pico_audio.h"
#include "pico_cart.h"
#include "pico_core.h"
#include "pico_script.h"

#include "utf8-util.h"

namespace path {
	std::string normalisePath(std::string path) {
		std::replace(path.begin(), path.end(), '\\', '/');
		return path;
	}

	std::string getPath(std::string path) {
		std::size_t pos = path.find_last_of('/');
		if (pos == std::string::npos) {
			return "";
		}
		return path.substr(0, pos + 1);
	}

	std::string getFilename(std::string path) {
		std::size_t pos = path.find_last_of('/');
		if (pos == std::string::npos) {
			return path;
		}
		return path.substr(pos + 1);
	}

	std::pair<std::string, std::string> splitFilename(std::string filename) {
		std::size_t pos = filename.find_last_of('.');
		if (pos == std::string::npos) {
			return std::make_pair(filename, "");
		}

		return std::make_pair(filename.substr(0, pos), filename.substr(pos + 1));
	}

	void test() {
		assert(normalisePath("/wibble\\test\\wibble/hello") == "/wibble/test/wibble/hello");

		assert(getPath("/wibble/hatstand/file.ext") == "/wibble/hatstand/");
		assert(getPath("file.ext") == "");
		assert(getPath("/file.ext") == "/");
		assert(getPath("/path/") == "/path/");

		assert(getFilename("/path/") == "");
		assert(getFilename("/path/name.txt") == "name.txt");
		assert(getFilename("name.txt") == "name.txt");

		typedef std::pair<std::string, std::string> pair;

		assert(splitFilename("name.txt") == pair("name", "txt"));
		assert(splitFilename("name") == pair("name", ""));
		assert(splitFilename(".txt") == pair("", "txt"));
	}
}  // namespace path

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

	void load(std::string filename) {
		filename = path::normalisePath(filename);

		std::string data = FILE_LoadFile(filename);
		if (data.size() == 0) {
			throw error(std::string("failed to open cart file: ") + filename);
		}

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
