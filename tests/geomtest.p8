pico-8 cartridge // http://www.pico-8.com
version 16
__lua__
function _init() 
	tests = {
		test1,
		test2,
		test3,
		test4
	}
	
	current_test = 1
end

function _update() 
	if btnp(❎) then
		current_test += 1
		if current_test > #tests then
			current_test = 1
		end
	end	
end

function _draw() 
	camera()
	clip()
	pal()
	tests[current_test]()
end


function test1()
	cls(7)
	circfill(64, 64, 63, 9)
end

function test2()
	cls(7)
	circfill(64, 64, 70, 9)
end

function test3()
	cls(7)
	clip(1,1,127,127)
	circfill(64, 64, 70, 9)
end


function test4()
	cls(7)
	clip(4,4,120,120)
	circfill(64, 64, 70, 9)
end

