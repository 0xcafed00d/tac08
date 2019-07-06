#include "utils.h"
#include "log.h"

#include <assert.h>
#include <algorithm>
#include <cstring>
#include <string>

namespace path {
	std::string removeRelative(std::string path) {
#ifdef __ANDROID__
		path = normalisePath(path);
		bool fromRoot = path.size() && path[0] == '/';
		std::vector<std::string> pathParts;
		utils::splitString(path, pathParts, "/");

		for (size_t n = 0; n < pathParts.size(); n++) {
			if (n && pathParts[n] == "..") {
				pathParts[n] = "";
				pathParts[n - 1] = "";
			}
		}

		std::string newPath = fromRoot ? "/" : "";
		for (size_t n = 0; n < pathParts.size(); n++) {
			if (pathParts[n].size()) {
				newPath += pathParts[n] + ((n == pathParts.size() - 1) ? "" : "/");
			}
		}
		return newPath;
#else
		return path;
#endif
	}

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

#ifdef __ANDROID__
		assert(removeRelative("/hello/wibble/../file.txt") == "/hello/file.txt");
		assert(removeRelative("hello/wibble/../file.txt") == "hello/file.txt");
		assert(removeRelative("../hello/wibble/../file.txt") == "../hello/file.txt");
#endif
	}
}  // namespace path

namespace utils {
	void splitString(const std::string& input,
	                 std::vector<std::string>& output,
	                 const char* sep_chars) {
		std::string::const_iterator i = input.begin();
		std::string outputString;

		while (i != input.end()) {
			if (strchr(sep_chars, *i) != NULL) {
				if (!outputString.empty()) {
					output.push_back(outputString);
					outputString.clear();
				}
			} else {
				outputString.push_back(*i);
			}
			++i;
		}

		if (!outputString.empty()) {
			output.push_back(outputString);
		}
	}

}  // namespace utils
