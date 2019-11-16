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

const char* all_chars = R"(▮■□⁙⁘‖◀▶「」¥•、。゛゜
 !"#$%&'()*+,-./
0123456789:;<=>?
@𝘢𝘣𝘤𝘥𝘦𝘧𝘨𝘩𝘪𝘫𝘬𝘭𝘮𝘯𝘰
𝘱𝘲𝘳𝘴𝘵𝘶𝘷𝘸𝘹𝘺𝘻[\]^_
`abcdefghijklmno
pqrstuvwxyz{|}~○
█▒🐱⬇️░✽●♥☉웃⌂⬅️😐♪🅾️◆
…➡️★⧗⬆️ˇ∧❎▤▥あいうえおか
きくけこさしすせそたちつてとなに
ぬねのはひふへほまみむめもやゆよ
らりるれろわをんっゃゅょアイウエ
オカキクケコサシスセソタチツテト
ナニヌネノハヒフヘホマミムメモヤ
ユヨラリルレロワヲンッャュョ◜◝)";

std::map<char32_t, uint8_t> emoji = {
    {0x025ae, 0x10}, {0x025a0, 0x11}, {0x025a1, 0x12}, {0x02059, 0x13}, {0x02058, 0x14},
    {0x02016, 0x15}, {0x025c0, 0x16}, {0x025b6, 0x17}, {0x0300c, 0x18}, {0x0300d, 0x19},
    {0x000a5, 0x1a}, {0x02022, 0x1b}, {0x03001, 0x1c}, {0x03002, 0x1d}, {0x0309b, 0x1e},
    {0x0309c, 0x1f}, {0x00020, 0x20}, {0x00021, 0x21}, {0x00022, 0x22}, {0x00023, 0x23},
    {0x00024, 0x24}, {0x00025, 0x25}, {0x00026, 0x26}, {0x00027, 0x27}, {0x00028, 0x28},
    {0x00029, 0x29}, {0x0002a, 0x2a}, {0x0002b, 0x2b}, {0x0002c, 0x2c}, {0x0002d, 0x2d},
    {0x0002e, 0x2e}, {0x0002f, 0x2f}, {0x00030, 0x30}, {0x00031, 0x31}, {0x00032, 0x32},
    {0x00033, 0x33}, {0x00034, 0x34}, {0x00035, 0x35}, {0x00036, 0x36}, {0x00037, 0x37},
    {0x00038, 0x38}, {0x00039, 0x39}, {0x0003a, 0x3a}, {0x0003b, 0x3b}, {0x0003c, 0x3c},
    {0x0003d, 0x3d}, {0x0003e, 0x3e}, {0x0003f, 0x3f}, {0x00040, 0x40}, {0x1d622, 0x41},
    {0x1d623, 0x42}, {0x1d624, 0x43}, {0x1d625, 0x44}, {0x1d626, 0x45}, {0x1d627, 0x46},
    {0x1d628, 0x47}, {0x1d629, 0x48}, {0x1d62a, 0x49}, {0x1d62b, 0x4a}, {0x1d62c, 0x4b},
    {0x1d62d, 0x4c}, {0x1d62e, 0x4d}, {0x1d62f, 0x4e}, {0x1d630, 0x4f}, {0x1d631, 0x50},
    {0x1d632, 0x51}, {0x1d633, 0x52}, {0x1d634, 0x53}, {0x1d635, 0x54}, {0x1d636, 0x55},
    {0x1d637, 0x56}, {0x1d638, 0x57}, {0x1d639, 0x58}, {0x1d63a, 0x59}, {0x1d63b, 0x5a},
    {0x0005b, 0x5b}, {0x0005c, 0x5c}, {0x0005d, 0x5d}, {0x0005e, 0x5e}, {0x0005f, 0x5f},
    {0x00060, 0x60}, {0x00061, 0x61}, {0x00062, 0x62}, {0x00063, 0x63}, {0x00064, 0x64},
    {0x00065, 0x65}, {0x00066, 0x66}, {0x00067, 0x67}, {0x00068, 0x68}, {0x00069, 0x69},
    {0x0006a, 0x6a}, {0x0006b, 0x6b}, {0x0006c, 0x6c}, {0x0006d, 0x6d}, {0x0006e, 0x6e},
    {0x0006f, 0x6f}, {0x00070, 0x70}, {0x00071, 0x71}, {0x00072, 0x72}, {0x00073, 0x73},
    {0x00074, 0x74}, {0x00075, 0x75}, {0x00076, 0x76}, {0x00077, 0x77}, {0x00078, 0x78},
    {0x00079, 0x79}, {0x0007a, 0x7a}, {0x0007b, 0x7b}, {0x0007c, 0x7c}, {0x0007d, 0x7d},
    {0x0007e, 0x7e}, {0x025cb, 0x7f}, {0x02588, 0x80}, {0x02592, 0x81}, {0x1f431, 0x82},
    {0x02b07, 0x83}, {0x02591, 0x84}, {0x0273d, 0x85}, {0x025cf, 0x86}, {0x02665, 0x87},
    {0x02609, 0x88}, {0x0c6c3, 0x89}, {0x02302, 0x8a}, {0x02b05, 0x8b}, {0x1f610, 0x8c},
    {0x0266a, 0x8d}, {0x1f17e, 0x8e}, {0x025c6, 0x8f}, {0x02026, 0x90}, {0x027a1, 0x91},
    {0x02605, 0x92}, {0x029d7, 0x93}, {0x02b06, 0x94}, {0x002c7, 0x95}, {0x02227, 0x96},
    {0x0274e, 0x97}, {0x025a4, 0x98}, {0x025a5, 0x99}, {0x03042, 0x9a}, {0x03044, 0x9b},
    {0x03046, 0x9c}, {0x03048, 0x9d}, {0x0304a, 0x9e}, {0x0304b, 0x9f}, {0x0304d, 0xa0},
    {0x0304f, 0xa1}, {0x03051, 0xa2}, {0x03053, 0xa3}, {0x03055, 0xa4}, {0x03057, 0xa5},
    {0x03059, 0xa6}, {0x0305b, 0xa7}, {0x0305d, 0xa8}, {0x0305f, 0xa9}, {0x03061, 0xaa},
    {0x03064, 0xab}, {0x03066, 0xac}, {0x03068, 0xad}, {0x0306a, 0xae}, {0x0306b, 0xaf},
    {0x0306c, 0xb0}, {0x0306d, 0xb1}, {0x0306e, 0xb2}, {0x0306f, 0xb3}, {0x03072, 0xb4},
    {0x03075, 0xb5}, {0x03078, 0xb6}, {0x0307b, 0xb7}, {0x0307e, 0xb8}, {0x0307f, 0xb9},
    {0x03080, 0xba}, {0x03081, 0xbb}, {0x03082, 0xbc}, {0x03084, 0xbd}, {0x03086, 0xbe},
    {0x03088, 0xbf}, {0x03089, 0xc0}, {0x0308a, 0xc1}, {0x0308b, 0xc2}, {0x0308c, 0xc3},
    {0x0308d, 0xc4}, {0x0308f, 0xc5}, {0x03092, 0xc6}, {0x03093, 0xc7}, {0x03063, 0xc8},
    {0x03083, 0xc9}, {0x03085, 0xca}, {0x03087, 0xcb}, {0x030a2, 0xcc}, {0x030a4, 0xcd},
    {0x030a6, 0xce}, {0x030a8, 0xcf}, {0x030aa, 0xd0}, {0x030ab, 0xd1}, {0x030ad, 0xd2},
    {0x030af, 0xd3}, {0x030b1, 0xd4}, {0x030b3, 0xd5}, {0x030b5, 0xd6}, {0x030b7, 0xd7},
    {0x030b9, 0xd8}, {0x030bb, 0xd9}, {0x030bd, 0xda}, {0x030bf, 0xdb}, {0x030c1, 0xdc},
    {0x030c4, 0xdd}, {0x030c6, 0xde}, {0x030c8, 0xdf}, {0x030ca, 0xe0}, {0x030cb, 0xe1},
    {0x030cc, 0xe2}, {0x030cd, 0xe3}, {0x030ce, 0xe4}, {0x030cf, 0xe5}, {0x030d2, 0xe6},
    {0x030d5, 0xe7}, {0x030d8, 0xe8}, {0x030db, 0xe9}, {0x030de, 0xea}, {0x030df, 0xeb},
    {0x030e0, 0xec}, {0x030e1, 0xed}, {0x030e2, 0xee}, {0x030e4, 0xef}, {0x030e6, 0xf0},
    {0x030e8, 0xf1}, {0x030e9, 0xf2}, {0x030ea, 0xf3}, {0x030eb, 0xf4}, {0x030ec, 0xf5},
    {0x030ed, 0xf6}, {0x030ef, 0xf7}, {0x030f2, 0xf8}, {0x030f3, 0xf9}, {0x030c3, 0xfa},
    {0x030e3, 0xfb}, {0x030e5, 0xfc}, {0x030e7, 0xfd}, {0x025dc, 0xfe}, {0x025dd, 0xff},
};

namespace pico_private {

	void dump_utf8_allchars() {
		int ascii = 0x10;
		auto s = std::string(all_chars);
		for (char32_t codepoint : utf8::CodepointIterator(s)) {
			if (codepoint >= 0x10 && codepoint != 0xfe0f) {  // ignore variant prefix 0xfe0f
				printf("{0x%05x, 0x%02x},\n", codepoint, ascii);
				ascii++;
			}
		}
	}

}  // namespace pico_private

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
		pico_control::init_rom();
	}

	void extractCart(Cart& cart) {
		extractAssets(cart);
		pico_script::load(cart);
	}

}  // namespace pico_cart
