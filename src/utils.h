#ifndef UTILS_H
#define UTILS_H

#include <string>

namespace path {
	std::string normalisePath(std::string path);
	std::string getPath(std::string path);
	std::string getFilename(std::string path);
	std::pair<std::string, std::string> splitFilename(std::string filename);
	void test();
}  // namespace path

namespace utils {
	template <typename T>
	T limit(T n, T min, T max) {
		if (n < min)
			return min;
		if (n > max)
			return max;
		return n;
	}
}  // namespace utils

#endif
