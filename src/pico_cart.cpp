#include <fstream>
#include <iostream>
#include <map>
#include <set>

#include "pico_cart.h"
#include "pico_core.h"
#include "pico_script.h"

namespace pico_cart {

	typedef std::map<std::string, std::string> sections;

	static std::set<std::string> valid_sections = {"__lua__", "__gfx__", "__gff__",
	                                               "__map__", "__sfx__", "__label__"};

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

	void load(std::string filename) {
		std::ifstream s(filename.c_str());
		if (!s) {
			throw error(std::string("failed to open cart file: ") + filename);
		}

		sections sect;
		sect["filename"] = filename;

		do_load(s, sect);

		pico_control::set_sprite_data(sect["__gfx__"], sect["__gff__"]);
		pico_control::set_map_data(sect["__map__"]);
		pico_script::load(sect["__lua__"]);
	}

}  // namespace pico_cart
