CXX = g++
# Update these paths to match your installation
# You may also need to update the linker option rpath, which sets where to look for
# the SDL2 libraries at runtime to match your install
SDL_LIB = -L/usr/local/lib -lSDL2 -Wl,-rpath=/usr/local/lib
LUA_LIB =  -Lsrc/z8lua -llua 
SDL_INCLUDE = -I/usr/local/include
UTF8_UTIL_BASE = src/utf8-util/utf8-util

CXXFLAGS = -Wall -Wno-unused-variable -Wno-unused-but-set-variable -c -std=c++11 $(SDL_INCLUDE) -I$(UTF8_UTIL_BASE)
LDFLAGS = $(SDL_LIB) $(LUA_LIB) 
EXE = thing

all: $(EXE)

$(EXE): main.o hal_core.o pico_core.o pico_data.o pico_script.o pico_cart.o utf8-util.o
	$(CXX) $^ $(LDFLAGS) -o $@
	@cowsay "Built All The Things!!!"

main.o: src/main.cpp src/hal_core.h src/pico_core.h src/pico_data.h src/pico_script.h src/pico_cart.h 
	$(CXX) $(CXXFLAGS) $< -o $@

hal_core.o: src/hal_core.cpp src/hal_core.h
	$(CXX) $(CXXFLAGS) $< -o $@

pico_core.o: src/pico_core.cpp src/pico_core.h src/hal_core.h
	$(CXX) $(CXXFLAGS) $< -o $@

pico_data.o: src/pico_data.cpp src/pico_data.h src/pico_core.h src/pico_script.h
	$(CXX) $(CXXFLAGS) $< -o $@

pico_cart.o: src/pico_cart.cpp src/pico_cart.h src/pico_core.h
	$(CXX) $(CXXFLAGS) $< -o $@

pico_script.o: src/pico_script.cpp src/pico_script.h src/pico_cart.h
	$(CXX) $(CXXFLAGS) $< -o $@

utf8-util.o: $(UTF8_UTIL_BASE)/utf8-util.cpp
	$(CXX) $(CXXFLAGS) $< -o $@

clean:
	rm *.o && rm $(EXE)
	
run: all
	./$(EXE) ../pico-8/gridbug/gridbug.p8 

