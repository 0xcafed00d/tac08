pico-8 cartridge // http://www.pico-8.com
version 16
__lua__

function _init()
	apix = __tac08__
	printh(apix.cwd())
	printh(apix.cd(".."))
	
	for f, d in apix.allfiles() do 
		printh(f.." "..tostr(d))
	end
end 

function _update()
end 

function _draw()
end 
