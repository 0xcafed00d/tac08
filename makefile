CXX = g++

# modify these paths to point to your local sdl install. 
SDL_PATH_LIB = /usr/local/lib
SDL_PATH_INC = /usr/local/include

SDL_LIB = -L$(SDL_PATH_LIB) -lSDL2
SDL_INCLUDE = -I$(SDL_PATH_LIB)

LUA_LIB =  -Lsrc/z8lua -lluaz8 
UTF8_UTIL_BASE = src/utf8-util/utf8-util

DEFINES = -DTAC08_PLATFORM=PLATFORM_DESKTOP_LINUX

CXXFLAGS_DEBUG = -DDEBUG -ggdb -Wall -c -std=c++11 $(SDL_INCLUDE) -I$(UTF8_UTIL_BASE) $(DEFINES)
CXXFLAGS_RELEASE = -O3 -Wall -c -std=c++11 $(SDL_INCLUDE) -I$(UTF8_UTIL_BASE) $(DEFINES)

CXXFLAGS = $(CXXFLAGS_DEBUG)

LDFLAGS = $(SDL_LIB) $(LUA_LIB) 
EXE = tac08

all: $(EXE)

$(EXE): bin/main.o bin/hal_core.o bin/hal_palette.o bin/hal_audio.o bin/pico_core.o bin/pico_gfx.o bin/pico_audio.o bin/pico_memory.o bin/pico_data.o bin/pico_script.o bin/pico_cart.o bin/utf8-util.o bin/utils.o bin/log.o bin/crypt.o
	$(CXX) $^ $(LDFLAGS) -o $@
	@echo "Built All The Things!!!"
	
bin/main.o: src/main.cpp src/hal_core.h src/hal_audio.h src/pico_core.h src/pico_audio.h src/pico_data.h src/pico_data.h src/pico_script.h src/pico_cart.h src/config.h src/log.h 
	$(CXX) $(CXXFLAGS) $< -o $@

bin/hal_core.o: src/hal_core.cpp src/hal_core.h src/hal_palette.h src/config.h src/log.h src/crypt.h
	$(CXX) $(CXXFLAGS) $< -o $@

bin/hal_palette.o: src/hal_palette.cpp src/hal_palette.h
	$(CXX) $(CXXFLAGS) $< -o $@

bin/hal_audio.o: src/hal_audio.cpp src/hal_audio.h src/config.h src/log.h
	$(CXX) $(CXXFLAGS) $< -o $@

bin/pico_core.o: src/pico_core.cpp src/pico_core.h src/pico_audio.h src/pico_memory.h src/pico_script.h src/pico_cart.h src/config.h src/utils.h src/log.h
	$(CXX) $(CXXFLAGS) $< -o $@

bin/pico_gfx.o: src/pico_gfx.cpp src/pico_gfx.h src/hal_core.h src/config.h src/utils.h src/log.h
	$(CXX) $(CXXFLAGS) $< -o $@

bin/pico_audio.o: src/pico_audio.cpp src/pico_core.h src/pico_audio.h src/pico_cart.h src/hal_core.h src/hal_audio.h src/log.h
	$(CXX) $(CXXFLAGS) $< -o $@

bin/pico_data.o: src/pico_data.cpp src/pico_data.h src/pico_core.h src/log.h
	$(CXX) $(CXXFLAGS) $< -o $@

bin/pico_memory.o: src/pico_memory.cpp src/pico_memory.h src/log.h
	$(CXX) $(CXXFLAGS) $< -o $@

bin/pico_cart.o: src/pico_cart.cpp src/pico_cart.h src/pico_audio.h src/pico_core.h src/pico_script.h src/utils.h src/log.h
	$(CXX) $(CXXFLAGS) $< -o $@

bin/pico_script.o: src/pico_script.cpp src/pico_script.h src/pico_core.h src/pico_audio.h src/pico_cart.h src/hal_audio.h src/hal_core.h src/log.h src/firmware.lua
	$(CXX) $(CXXFLAGS) $< -o $@

bin/utils.o: src/utils.cpp src/utils.h
	$(CXX) $(CXXFLAGS) $< -o $@

bin/log.o: src/log.cpp src/log.h
	$(CXX) $(CXXFLAGS) $< -o $@

bin/utf8-util.o: $(UTF8_UTIL_BASE)/utf8-util.cpp
	$(CXX) $(CXXFLAGS) $< -o $@

bin/crypt.o: src/crypt.cpp
	$(CXX) $(CXXFLAGS) $< -o $@

clean:
	@rm bin/*.o || true 
	@rm $(EXE) || true
	
run: all
	./$(EXE)  

copy: all
	@cp tac08 ~/bin/ || true
