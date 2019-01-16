# tac08

## what is tac08?
tac08 is an emulation of the runtime part of the Pico-8 fantasy console. It takes a .p8 (text format) Pico-8 cart file and runs it as closely posible as the real Pico-8 software.

## what isn't tac08?
tac08 is not a replacement for Pico-8, it provides none of the content creation components of Pico-8, such as code editing, sprite and map creation and music tools. You will still require a copy of Pico-8 to make games. 

## why was tac08 written? 
1. To enable Pico-8 games to be run on platforms that Pico-8 itself does not run on.
2. To make it possible to extent the Pico-8 api and allow games to use features not currently supported by Pico-8

## is it a 100% emulation?
no.

## how do I get sound working then?

## how do I build tac08
A makefile is supplied that will build tac08 for linux systems. It has been tested on Ubuntu 16.04 & 18.04, Rasbian on a Raspberry Pi 3, and a Crostini hosted debian install on a chromebook. It will probably work with little modification on a Apple Mac too, but I am not able to test that.

Build scripts for other environments are not currently provided. tac08 is written in standard c++11. The lua interpreter is a version of lua 5.2 modified to support the syntax changes Pico-8 requires, while the lua interpreter is written in C, the modifications require it to be compiled as C++. This modification was performed by Sam Hocevar https://github.com/samhocevar/z8lua

The only external depenancy tac08 is the core SDL2.0 library, so it should be possible to build tac08 from the provided code for any platform that has SDL2.0 ported to it. 


1. first clone tac08 from github: 
```
git clone --recurse-submodules https://github.com/simulatedsimian/tac08.git
```

2. install sdl2.0 development libraries:
```
sudo apt install libsdl2-dev
```

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
