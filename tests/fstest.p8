pico-8 cartridge // http://www.pico-8.com
version 16
__lua__

function _init()
	apix = __tac08__
	printh(apix.cwd())
	
	f,d,s = apix.files()
	while f do
		printh(f.." "..tostr(d).." "..tostr(s))
		if f == nil then 
			break
		end
		f,d,s = apix.files()
	end
end 

function _update()
end 

function _draw()
end 
