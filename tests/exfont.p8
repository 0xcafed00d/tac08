pico-8 cartridge // http://www.pico-8.com
version 16
__lua__

local internal_str = ""

function _init()

	for n = 0x10, 0xff do
		internal_str = internal_str..chr(n)
	end
end


function _update()
end


local c = 0x10
function _draw()
	cls()
	color(7)

	local x, y = 0, 0
	
	for n = 0, 12 do
		print(internal_str, x, y)
		x = x - 128
		y = y + 6
	end

	print(chr(c))
	c+=1

	if c > 0xff then 
		c = 0x10
	end

end
