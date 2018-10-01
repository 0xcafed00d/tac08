#ifndef PICO_CART_H
#define PICO_CART_H

#include <stdexcept>
#include <string>

namespace pico_cart {

	struct error : public std::runtime_error {
		using std::runtime_error::runtime_error;
	};

	void load(std::string filename);
}  // namespace pico_cart

#endif /* PICO_CART_H */
