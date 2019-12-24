#include "hal_fs.h"

#include <dirent.h>
#include <unistd.h>

namespace hal_fs {
	std::string cwd() {
		auto buf = get_current_dir_name();
		std::string val(buf);
		free(buf);
		return val;
	}

	finfo files() {
		static DIR* dir_ptr = nullptr;

		if (!dir_ptr) {
			closedir(dir_ptr);
			dir_ptr = opendir(cwd().c_str());
		}

		if (dir_ptr) {
			auto entry = readdir(dir_ptr);
			if (entry) {
				finfo fi;
				fi.name = entry->d_name;
				fi.dir = entry->d_type == DT_DIR;
				return fi;
			} else {
				closedir(dir_ptr);
				dir_ptr = nullptr;
			}
		}
		return finfo{"", false};
	}

	void cd(const char* dir);

	/*
	    // read / write from anywhere
	    std::string loadFile(std::string name);
	    bool saveFile(std::string name, std::string data);

	    // read / write from default game save file dir
	    std::string loadGameState(std::string name);
	    void saveGameState(std::string name, std::string data);

	    // read / write from clip board
	    std::string readClip();
	    void writeClip(const std::string& data);
	*/

}  // namespace hal_fs