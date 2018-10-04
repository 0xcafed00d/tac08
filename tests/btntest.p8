pico-8 cartridge // http://www.pico-8.com
version 16
__lua__
function _init()
	cls()
	x = 0
end 

function _update()

end 

function _draw()

	pset (x-1, 8, 0)
	pset (x, 8, 7)

	for n=0,5 do
 	if (btn(n))	pset (x, 16 + n * 2, 7)
 	if (btnp(n))	pset (x,16 + n * 2, 8)
	end
	
	x+=1
	if (x > 127) then 
		x = 0
		cls()
	end

end 
