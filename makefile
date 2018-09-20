CXX = g++
# Update these paths to match your installation
# You may also need to update the linker option rpath, which sets where to look for
# the SDL2 libraries at runtime to match your install
SDL_LIB = -L/usr/local/lib -lSDL2 -Wl,-rpath=/usr/local/lib
SDL_INCLUDE = -I/usr/local/include
# You may need to change -std=c++11 to -std=c++0x if your compiler is a bit older
CXXFLAGS = -Wall -c -std=c++11 $(SDL_INCLUDE)
LDFLAGS = $(SDL_LIB)
EXE = thing

all: $(EXE)

$(EXE): main.o gfx_core.o pico_core.o pico_data.o
	$(CXX) $^ $(LDFLAGS) -o $@
	cowsay "Built All The Things!!!"

main.o: src/main.cpp src/gfx_core.h src/pico_core.h src/pico_data.h
	$(CXX) $(CXXFLAGS) $< -o $@

gfx_core.o: src/gfx_core.cpp src/gfx_core.h
	$(CXX) $(CXXFLAGS) $< -o $@

pico_core.o: src/pico_core.cpp src/pico_core.h src/gfx_core.h
	$(CXX) $(CXXFLAGS) $< -o $@

pico_data.o: src/pico_data.cpp src/pico_data.h src/pico_core.h
	$(CXX) $(CXXFLAGS) $< -o $@

clean:
	rm *.o && rm $(EXE)
	
run: all
	./$(EXE)

