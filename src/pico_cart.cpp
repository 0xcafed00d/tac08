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

	static std::set<std::string> valid_sections = {"__lua__",  "__gfx__",   "__gfx8__",
	                                               "__font__", "__gff__",   "__map__",
	                                               "__sfx__",  "__music__", "__label__"};

	void do_load(std::istream& s, Cart& cart, std::string filename);

	bool check_include_file(const std::string& line, Cart& cart, int filenum) {
		if (line.size() && line[0] == '#' && line.find("#include") == 0) {
			cart.source.push_back(Line{filenum, std::string("-- ") + line});
			std::string incfile = cart.sections["base_path"] + utils::trimboth(line.substr(8));
			incfile = path::removeRelative(incfile);
			logr << "Loading include file " << incfile;
			std::string data = FILE_LoadFile(incfile);
			if (data.size() == 0) {
				throw error(std::string("failed to open include file: ") + incfile);
			}
			std::istringstream s(data);
			do_load(s, cart, incfile);
			return true;
		}
		return false;
	}

	void do_load(std::istream& s, Cart& cart, std::string filename) {
		TraceFunction();
		cart.files.push_back(filename);
		int filenum = cart.files.size() - 1;

		std::string line;
		while (std::getline(s, line)) {
			line = utils::trimright(line, " \n\r");
			if (!check_include_file(line, cart, filenum)) {
				if (valid_sections.find(line) != valid_sections.end()) {
					cart.sections["cur_sect"] = line;
					logr << "section " << line;
				} else {
					if (cart.sections["cur_sect"] == "__lua__") {
						cart.source.push_back(Line{filenum, line});
					} else {
						cart.sections[cart.sections["cur_sect"]] += line + "\n";
					}
				}
			}
		}
	}

	LineInfo getLineInfo(const Cart& cart, int lineNum) {
		LineInfo li;
		li.sourceLine = cart.source[lineNum].line;
		li.filename = cart.files[cart.source[lineNum].file];
		li.localLineNum = 0;

		auto filenum = cart.source[lineNum].file;
		for (int l = lineNum; l >= 0; l--) {
			if (cart.source[l].file == filenum) {
				li.localLineNum++;
			}
		}

		// if its the main p8 file add extra to skip file header.
		if (cart.source[lineNum].file == 0) {
			li.localLineNum += 3;
		}

		return li;
	}

	std::string getCartName() {
		return "";
	}

	std::string getCartPath() {
		return "";
	}

	std::string convert_emojis(const std::string& lua) {
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

	static Cart loadedCart;

	Cart& getCart() {
		return loadedCart;
	}

	// if the filename begins with a $ then the path of the new cart of relative to the one
	// callng load
	void load(std::string filename) {
		path::test();
		TraceFunction();

		logr << "Request cart load: " << filename;

		filename = path::normalisePath(filename);

		if (loadedCart.sections.find("base_path") != loadedCart.sections.end() &&
		    filename.length() && filename[0] == '$') {
			filename = loadedCart.sections["base_path"] + filename.substr(1);
		}

		std::string data = FILE_LoadFile(filename);
		logr << "loaded: " << data.size() << "bytes";
		if (data.size() == 0) {
			throw error(std::string("failed to open cart file: ") + filename);
		}

		loadedCart = Cart{};
		logr << "Loading cart: " << filename;
		loadedCart.sections["filename"] = filename;
		loadedCart.sections["base_path"] = path::getPath(filename);
		loadedCart.sections["cart_name"] = path::splitFilename(path::getFilename(filename)).first;
		loadedCart.sections["cur_sect"] = "header";

		std::istringstream s(data);
		do_load(s, loadedCart, filename);
	}

	void extractAssets(Cart& cart);

	void loadassets(std::string filename, Cart& parentCart) {
		TraceFunction();

		logr << "Request asset load: " << filename;
		filename = path::normalisePath(filename);
		filename = loadedCart.sections["base_path"] + filename;

		std::string data = FILE_LoadFile(filename);
		logr << "loaded: " << data.size() << "bytes";
		if (data.size() > 0) {
			Cart c;
			c.sections["cur_sect"] = "header";
			std::istringstream s(data);
			do_load(s, c, filename);
			extractAssets(c);
		}
	}

	void extractAssets(Cart& cart) {
		pico_control::set_sprite_data_4bit(cart.sections["__gfx__"]);
		pico_control::set_sprite_data_8bit(cart.sections["__gfx8__"]);
		pico_control::set_sprite_flags(cart.sections["__gff__"]);
		pico_control::set_font_data(cart.sections["__font__"]);
		pico_control::set_map_data(cart.sections["__map__"]);
		pico_control::set_music_from_cart(cart.sections["__music__"]);
		pico_control::set_sfx_from_cart(cart.sections["__sfx__"]);
	}

	void extractCart(Cart& cart) {
		extractAssets(cart);
		pico_script::load(cart);
	}

}  // namespace pico_cart
