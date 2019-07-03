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

	struct Line {
		int file;
		std::string line;
	};

	struct Cart {
		std::map<std::string, std::string> sections;
		std::vector<Line> source;
		std::vector<std::string> files;
	};

	void load(std::string filename);
	void loadassets(std::string filename, Cart& parentCart);
	void extractCart(Cart& cart);
	Cart& getCart();

	struct LineInfo {
		std::string filename;
		int localLineNum;
		std::string sourceLine;
	};
	LineInfo getLineInfo(const Cart& cart, int lineNum);
	std::string convert_emojis(const std::string& lua);

}  // namespace pico_cart

#endif /* PICO_CART_H */
