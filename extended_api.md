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
Read string from file. Returns string containing contents of file
or an empty string if file not present
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

## siminput(state)
Simulate joypad input. State is an 8 bit value containing the 
dpad/button states in the same order returned from btn() api call.
Used for simulating joypad input on touch screen devices such as phones.

## touchmask()
Returns 8 bit value with each bit representing an active multi touch on the screen.
Use indices of the set bits to pass to touchstate() to get the state for that touch point.
 
## touchstate(index)
For a given touch index returns the state of that touch point.
returns 3 parameters:
x - the x pos of the touch
y - the y pos of the touch
state - the state of the touch. 

## maps(page)
Select maps page to get map data from. 

## maps()
Select default map page 

## sprites(page)
Select sprites page to get map data from. 

## sprites()
Select default sprite page 

## open_url(url)
Opens the suplied url in the default system browser.

