pico-8 cartridge // http://www.pico-8.com
version 16
__lua__

function _init()
	to_load = "../../pico-8/flower/flower.p8"
	time = 30
end


function _update()
	time = time - 1
	if time == 0 then
		load(to_load)
	end
end

function _draw()
	cls()
	print(time)	
	print(to_load)
end
