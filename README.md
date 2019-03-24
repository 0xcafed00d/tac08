# tac08

## What is tac08?
tac08 is an emulation of the runtime part of the Pico-8 fantasy console. It takes a .p8 (text format) Pico-8 cart file and runs it closely as possible to the real Pico-8 software.

## What isn't tac08?
tac08 is not a replacement for Pico-8, it provides none of the content creation components of Pico-8, such as code editing, sprite and map creation and music tools. You will still require a copy of Pico-8 to make games. Also if you just want to run Pico-8 games you will have a much better experience with Pico-8 than tac08

## Why was tac08 written? 
tac08's target audience are developers that want to do one or more of the following: 
1. To enable Pico-8 games to be run on platforms that Pico-8 itself does not run on.
2. To embed Pico-8 games within other game engines. 
3. To make it possible to extend the Pico-8 api and allow games to use features not currently supported by Pico-8

tac08 was written for my own personal use (specifically for items 1 & 3 above) but I have decided to open source it as others may find it useful. 

## Is it a 100% emulation?
No. tac08 is still in development, however in my testing, I have found that a large number of the most popular games work correctly. 

This is a list of the most significant compatibility issues:
1. Not all peek and poke addresses are implemented, notably the current draw state values
2. Only one joystick is currently supported and it cannot be configured.
3. Saving screen shots and recording gif videos are not implemented.  
4. The flip() api function is not implemented. So no tweet carts and such will work. Only games that use _init, _update or _update60, _draw will work correctly.
5. Pico-8's sound synthesizer is not implemented, however you can still play sound effects (see below)
6. The music() api function is not currently implemented (but I plan to implement it). 
7. There are probably more things i can add to this list and will update as needed. 

## How do I get sound working then?
The way sound is implemented in tac08 is not ideal, but given current my current work loads and complexity of the Pico-8 sound system I feel it is a reasonable compromise.
In order to have tac08 play sound you need to export your sound effects from Pico-8 as wav files. I have found that sound effects do not export completely if they have loops within them. 

Exporting the sound effects is a two stage process. First paste the following code into the Pico-8 command prompt: 
```
for a=0x3200,0x42ff,68 do poke(a+66,0) poke(a+67,0) end cstore(0x3100,0x3100,0x1200,"audio.p8")
```
This will save out a cart called "audio.p8" containing only the unlooped sfx data. 

Next load the audio.p8 cart and export the actual sfx as wav files:
```
load "audio.p8"
export "cart%d.wav"
```

where "cart" is the name of your original cartridge file. You need to have these wav files in the same folder as you cart. You can delete any wav files that your cart does not need. 

**note that if the game creates sound at runtime then it is not currently possible to play these**


## How do I build tac08

### Windows

A Visual Studio 2017 solution is provided in the tac08\win-tac08 directory. A copy of Visual Studio 2017 Community with C++ components installed is sufficient to build tac08.

A copy of the libSDL2 header files, libraries and DLLs are included with the solution and do not need to be downloaded separately. 

1. first clone tac08 from github: 
```
git clone --recurse-submodules https://github.com/0xcafed00d/tac08.git
```

2. Simply open the solution file (tac08\win-tac08\win-tac08.sln) with VS2017, and select Build Solution. Binaries will be place in either the Release or Debug directories, depending on which build mode is selected. 

### Linux & Mac

A makefile is supplied that will build tac08 for linux systems & Mac OSX. It has been tested on Ubuntu 16.04 & 18.04, Rasbian on a Raspberry Pi 3, a Crostini hosted debian install on a chromebook, and a Mac. 

Build scripts for other environments are not currently provided (because I have not written them yet :-). tac08 is written in standard c++11. The lua interpreter is a version of lua 5.2 modified to support the syntax changes Pico-8 requires, while the lua interpreter is written in C, the modifications require it to be compiled as C++. This modification was performed by Sam Hocevar https://github.com/samhocevar/z8lua

The only external dependency tac08 is the core SDL2.0 library, so it should be possible to build tac08 from the provided code for any platform that has SDL2.0 ported to it. 

1. first clone tac08 from github: 
```
git clone --recurse-submodules https://github.com/0xcafed00d/tac08.git
```

2. install sdl2.0 development libraries:

For Debian based Linux distributions use apt. 
```
sudo apt install libsdl2-dev
```
Other distributions and the Mac will have other mechanisms for installing libraries.

You may need to modify the SDL_PATH_LIB & SDL_PATH_INC variables in the makefile if your SDL libraries are not installed in a standard location. 

3. build lua interpreter:
```
cd tac08/src/z8lua
make a
cd ../..
```

4. build tac08:
``` 
make
```
After this you **should** be left with a tac08 executable with you can run with:
```
./tac08 mygame.p8
```
