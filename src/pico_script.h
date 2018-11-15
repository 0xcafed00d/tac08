#ifndef PICO_SCRIPT_H
#define PICO_SCRIPT_H

#include <stdexcept>

#include "string"

namespace pico_script {

	struct error : public std::runtime_error {
		using std::runtime_error::runtime_error;
	};

	void load(std::string script);
	void reset();
	bool run(std::string function, bool optional);
	bool do_menu();

}  // namespace pico_script

#endif /* PICO_SCRIPT_H */
