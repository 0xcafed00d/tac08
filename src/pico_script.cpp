#include "pico_script.h"
#include "z8lua/lualib.h"

namespace pico_script {
	void load(std::string script);
	void reset();
	void run(std::string function);

}  // namespace pico_script
