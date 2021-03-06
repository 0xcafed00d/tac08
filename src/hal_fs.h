#ifndef HAL_FS_H
#define HAL_FS_H

#include <string>

namespace hal_fs {
	struct finfo {
		std::string name;
		bool dir = false;
	};

	std::string cwd();
	finfo files();
	void cd(const char* dir);

	// read / write from anywhere
	std::string loadFile(std::string name);
	bool saveFile(std::string name, std::string data);

	// read / write from default game save file dir
	std::string loadGameState(std::string name);
	void saveGameState(std::string name, std::string data);

	// read / write from clip board
	std::string readClip();
	void writeClip(const std::string& data);

}  // namespace hal_fs

#endif /* HAL_FS_H */
