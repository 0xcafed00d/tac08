#ifndef PICO_CART_H
#define PICO_CART_H

#include <map>
#include <stdexcept>
#include <string>

namespace pico_cart {

	struct error : public std::runtime_error {
		using std::runtime_error::runtime_error;
	};

	typedef std::map<std::string, std::string> sections;

	void load(std::string filename);
	void extractCart(sections& cart);
	sections& getCart();

	std::string convert_emojis(std::string& lua);

}  // namespace pico_cart

#endif /* PICO_CART_H */
