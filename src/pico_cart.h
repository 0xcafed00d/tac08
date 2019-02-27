#ifndef PICO_CART_H
#define PICO_CART_H

#include <map>
#include <stack>
#include <stdexcept>
#include <string>
#include <vector>

namespace pico_cart {

	struct error : public std::runtime_error {
		using std::runtime_error::runtime_error;
	};

	struct Cart {
		struct lineinfo {
			std::string file;
			size_t line;
		};

		std::map<std::string, std::string> sections;
		std::vector<std::string> source;
	};

	void load(std::string filename);
	void extractCart(Cart& cart);
	Cart& getCart();

	Cart::lineinfo getLineInfo(const Cart& cart, size_t lineNum);
	std::string convert_emojis(const std::string& lua);

}  // namespace pico_cart

#endif /* PICO_CART_H */
