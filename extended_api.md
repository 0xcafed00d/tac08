# tac08 extended api

## wrclip(str)
Writes string to system clipboard. 
* str - string to write to clipboard

## rdclip()
Reads string from system clipboard. Returns empty string if clipboard in empty or cannot be read

## wrstr(name, str)
Write string to file.
* name - filename to write string to
* str - string to write to file

## rdstr(name)
Read string from file.
* name - filename to read file from 

## xpal(enable)
Enable extended (> 16 colours) palette mode.
* enable - boolean value, true enables extended palette mode, false returns back to 16 colour mode

## setpal(index, r, g, b)
Set a palette index to a new rgb value.
* index - palette entry index (0-15 in pico-8 mode  or 0-255 in extended palette mode)
* r - new red value (0-255) 
* g - new green value (0-255) 
* b - new blue value (0-255) 

## resetpal()
Reset whole palette to default state.

## resetpal(i)
Reset 1 palette entry to default state.
* index - palette entry index (0-15 in pico-8 mode  or 0-255 in extended palette mode)

## screen(width, height)
Set screen resolution. 
* width - new screen width in pixels (min: 64, max 256)
* height - new screen height in pixels (min: 64, max 256)

## cursor(enable)
Enables the system hardware cursor.
* enable - boolean value, true shows cursor, false hides.

## showmenu()
Programatically shows the pause menu. 


## wavload(filename)
## wavplay(id, chan, loop)
## wavplay(id, chan, loop_start, loop_end)
## wavstop(chan)
## wavstoploop(chan)
## wavplaying(chan)

