pico-8 cartridge // http://www.pico-8.com
version 16
__lua__

function _init()
	__tac08__.cursor(true)
end


function _update()
end

function _draw()
	cls()
	print ("mouse:  x:"..stat(32).." y:"..stat(33).." btns:"..stat(34), 0, 0, 7)
	for n = 0,7 do
		x, y, state = __tac08__.touchstate(n)
		print ("touch: "..n.." x:"..x.." y:"..y.." state:"..state, 0, n * 8+16, state)
	end
end
