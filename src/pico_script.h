#ifndef PICO_SCRIPT_H
#define PICO_SCRIPT_H

#include <stdexcept>

#include "string"

#include "pico_cart.h"

namespace pico_script {

	struct error : public std::runtime_error {
		using std::runtime_error::runtime_error;
	};

	void load(const pico_cart::Cart& cart);
	bool symbolExist(const char* s);
	bool run(std::string function, bool optional, bool& restarted);
	bool do_menu();
	void unload_scripting();
	void tron();
	void troff();

}  // namespace pico_script

#endif /* PICO_SCRIPT_H */
