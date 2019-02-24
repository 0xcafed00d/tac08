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

	inline std::string trimleft(const std::string& str, const char* wschars = " \t") {
		std::string result = str;

		result.erase(0, str.find_first_not_of(wschars));
		return result;
	}

	inline std::string trimright(const std::string& str, const char* wschars = " \t") {
		std::string result = str;

		result.erase(str.find_last_not_of(wschars) + 1);
		return result;
	}

	inline std::string trimboth(const std::string& str, const char* wschars = " \t") {
		return trimleft(trimright(str, wschars), wschars);
	}

}  // namespace utils

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

#endif
