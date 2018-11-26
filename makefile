CXX = g++
# Update these paths to match your installation
# You may also need to update the linker option rpath, which sets where to look for
# the SDL2 libraries at runtime to match your install
SDL_LIB = -L/home/lmw/src/SDL2-2.0.9/build/.libs -lSDL2 -Wl,-rpath=/home/lmw/src/SDL2-2.0.9/build/.libs
#SDL_LIB = -L/usr/local/lib -lSDL2 -Wl,-rpath=/usr/local/lib
LUA_LIB =  -Lsrc/z8lua -llua 
SDL_INCLUDE = -I/usr/local/include
UTF8_UTIL_BASE = src/utf8-util/utf8-util

# CXXFLAGS = -ggdb -Wall -c -std=c++11 $(SDL_INCLUDE) -I$(UTF8_UTIL_BASE)
CXXFLAGS = -O3 -Wall -c -std=c++11 $(SDL_INCLUDE) -I$(UTF8_UTIL_BASE)
LDFLAGS = $(SDL_LIB) $(LUA_LIB) 
EXE = tac08

all: $(EXE)

$(EXE): bin/main.o bin/hal_core.o bin/hal_audio.o bin/pico_core.o bin/pico_audio.o bin/pico_memory.o bin/pico_data.o bin/pico_script.o bin/pico_cart.o bin/utf8-util.o
	$(CXX) $^ $(LDFLAGS) -o $@
	@cowsay "Built All The Things!!!"

bin/main.o: src/main.cpp src/hal_core.h src/pico_core.h src/pico_data.h src/pico_data.h src/pico_script.h src/pico_cart.h src/config.h 
	$(CXX) $(CXXFLAGS) $< -o $@

bin/hal_core.o: src/hal_core.cpp src/hal_core.h src/hal_audio.h src/config.h
	$(CXX) $(CXXFLAGS) $< -o $@

bin/hal_audio.o: src/hal_audio.cpp src/hal_audio.h src/config.h
	$(CXX) $(CXXFLAGS) $< -o $@

bin/pico_core.o: src/pico_core.cpp src/pico_core.h src/hal_core.h src/pico_memory.h src/config.h
	$(CXX) $(CXXFLAGS) $< -o $@

bin/pico_audio.o: src/pico_audio.cpp src/hal_core.h src/pico_memory.h src/hal_audio.h
	$(CXX) $(CXXFLAGS) $< -o $@

bin/pico_data.o: src/pico_data.cpp src/pico_data.h src/pico_core.h src/pico_script.h
	$(CXX) $(CXXFLAGS) $< -o $@

bin/pico_memory.o: src/pico_memory.cpp src/pico_memory.h 
	$(CXX) $(CXXFLAGS) $< -o $@

bin/pico_cart.o: src/pico_cart.cpp src/pico_cart.h src/pico_core.h
	$(CXX) $(CXXFLAGS) $< -o $@

bin/pico_script.o: src/pico_script.cpp src/pico_script.h src/pico_core.h src/pico_audio.h src/pico_cart.h src/hal_audio.h src/firmware.lua
	$(CXX) $(CXXFLAGS) $< -o $@

bin/utf8-util.o: $(UTF8_UTIL_BASE)/utf8-util.cpp
	$(CXX) $(CXXFLAGS) $< -o $@

clean:
	rm bin/*.o && rm $(EXE)
	
run: all
	./$(EXE) ../pico-8/gridbug/gridbug.p8 

