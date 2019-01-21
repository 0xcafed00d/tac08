#include "utils.h"

#include <assert.h>
#include <algorithm>
#include <string>

namespace path {
	std::string normalisePath(std::string path) {
		std::replace(path.begin(), path.end(), '\\', '/');
		return path;
	}

	std::string getPath(std::string path) {
		std::size_t pos = path.find_last_of('/');
		if (pos == std::string::npos) {
			return "";
		}
		return path.substr(0, pos + 1);
	}

	std::string getFilename(std::string path) {
		std::size_t pos = path.find_last_of('/');
		if (pos == std::string::npos) {
			return path;
		}
		return path.substr(pos + 1);
	}

	std::pair<std::string, std::string> splitFilename(std::string filename) {
		std::size_t pos = filename.find_last_of('.');
		if (pos == std::string::npos) {
			return std::make_pair(filename, "");
		}

		return std::make_pair(filename.substr(0, pos), filename.substr(pos + 1));
	}

	void test() {
		assert(normalisePath("/wibble\\test\\wibble/hello") == "/wibble/test/wibble/hello");

		assert(getPath("/wibble/hatstand/file.ext") == "/wibble/hatstand/");
		assert(getPath("file.ext") == "");
		assert(getPath("/file.ext") == "/");
		assert(getPath("/path/") == "/path/");

		assert(getFilename("/path/") == "");
		assert(getFilename("/path/name.txt") == "name.txt");
		assert(getFilename("name.txt") == "name.txt");

		typedef std::pair<std::string, std::string> pair;

		assert(splitFilename("name.txt") == pair("name", "txt"));
		assert(splitFilename("name") == pair("name", ""));
		assert(splitFilename(".txt") == pair("", "txt"));
	}
}  // namespace path

