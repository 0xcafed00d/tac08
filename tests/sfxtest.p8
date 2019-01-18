pico-8 cartridge // http://www.pico-8.com
version 16
__lua__

tests = {
	function () 
		sfx(0, 0)
	end,
	function () 
		sfx(0)
	end,
	function ()
		sfx(0, 0, 2) 
	end,
	function () 
		sfx(0, 0, 2, 1) 
	end,
}

currentTest = 1

function _init()
end

function _update()
	if btnp(3) then
		currentTest-=1
		if (currentTest == 0) currentTest = #tests
	end
	if btnp(2) then
		currentTest+=1
		if (currentTest>#tests) currentTest = 1
	end

	if (btnp(5)) then
		tests[currentTest]()
	end
end

function _draw()
	cls()
	print(currentTest)
end

__sfx__
002800001a0551c055180550c05513055000050000500005000050000500005000050000500005000050000500005000050000500005000050000500005000050000500005000050000500005000050000500005
