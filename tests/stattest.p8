pico-8 cartridge // http://www.pico-8.com
version 18
__lua__

stats = {}

function _init()
end

function _update60()
	for n = 0, 9 do 
		stats[n] = stat(n)
	end
end

function _draw()
	cls()
	for n = 0, 9 do 
		print("stat("..tostr(n)..") = "..tostr(stats[n]))
	end
end
