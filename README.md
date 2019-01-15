# tac08

## what is tac08?
tac08 is an an emulation of the runtime part of the Pico-8 fantasy console. It takes a .p8 (text format) pico-8 cart file and runs it as closely posible

## what isn't tac08?
tac08 is not a replacement for Pico-8, it provides none of the content creation components of Pico-8, such as code editing, sprite and map creation and music tools. You will still require a copy 

## why was tac08 written? 

## is it a 100% emulation?
no.

## how do I get sound working then?

## how do I build tac08

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
After this you *should* be left with a tac08 executable with you can run with:
```
./tac08 mygame.p8
```
