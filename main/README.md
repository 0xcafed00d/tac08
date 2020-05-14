# tac08 - odroid-go

## TODO

### Critical path
- [ ] Examine tac08 code. Understand SDL2 calls and port to SDL1 calls using esp32 SDL port.

### Project cleanup
- [ ] Fork `z8lua` and add esp32-specific changes in a branch. Add to `components/z8lua` as submodule.
	- Or, script copying `src/z8lua` to `components` and applying esp32-specific changes.

## Milestones
- Project compiles for esp32 with all source code (incompatible calls commented out)

## Change summary
- Added esp-idf scripts from https://github.com/concreted/odroid-go-starter-kit
- Copied `z8lua` into `components/z8lua` to make it a compiled component
	- Added `CMakeLists.txt` 
	- Renamed all source files from `.c` to `.cpp`
	- Commented out all `signal()` calls in `lua.cpp`
- Copied `SDL` library from https://github.com/jkirsons/Duke3D/ into `components/sdl`
- Added `src` to esp-idf compilation flow as `tac08` component
	- Created symbolic link for `src->components/tac08`
	- Added `CMakeLists.txt` 
- Created `main` - esp32 entrypoint
- Added top-level `CMakeLists.txt` for the project
