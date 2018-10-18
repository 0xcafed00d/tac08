pico-8 cartridge // http://www.pico-8.com
version 16
__lua__

tests = {}
function _init() 
	current_test = 1
end

x = 64
y = 64
r = 64

f = 0;

function _update() 
	if btnp(❎) then
		current_test += 1
		if current_test > #tests then
			current_test = 1
		end
	end

	f += 1

	if btnp(0) then
		if btn(4) then 
			r -= 1
		else
			x -= 1
		end
	end
	if btnp(1) then
		if btn(4) then 
			r += 1
		else
			x += 1
		end
	end
	if btnp(2) then
		y -= 1
	end
	if btnp(3) then
		y += 1
	end

end

function _draw() 
	camera()
	clip()
	pal()
	fillp(f)
	tests[current_test]()
	print (current_test, 120, 1, 0)
end

function draw_circ()
	circfill(x, y, r, 9)
	clip()
	print ("x:"..x.." y:"..y.." r:"..r, 1,1,0);
end

function draw_circl()
	circ(x, y, r, 9)
	clip()
	print ("x:"..x.." y:"..y.." r:"..r, 1,1,0);
end

function draw_rect()
	rectfill(x-r, y-r, x+r, y+r, 9)
	clip()
	print ("x:"..x.." y:"..y.." r:"..r, 1,1,0);
end

function draw_rectl()
	rect(x-r, y-r, x+r, y+r, 9)
	clip()
	print ("x:"..x.." y:"..y.." r:"..r, 1,1,0);
end

add(tests, function ()
	cls(7)
	draw_circ()
end)

add(tests, 
function()
	cls(7)
	clip(1,1,127,127)
	draw_circ()
end)


add(tests, 
function()
	cls(7)
	clip(4,4,120,120)
	draw_circ()
end)

add(tests, function ()
	cls(7)
	draw_circl()
end)

add(tests, 
function()
	cls(7)
	clip(1,1,127,127)
	draw_circl()
end)


add(tests, 
function()
	cls(7)
	clip(4,4,120,120)
	draw_circl()
end)

add(tests, 
function()
	cls(7)
	draw_rect()
end)

add(tests, 
function()
	cls(7)
	draw_rectl()
end)

add(tests, 
function()
	cls(7)
	clip(1,1,126,126)
	draw_rect()
end)

add(tests, 
function()
	cls(7)
	clip(1,1,126,126)
	draw_rectl()
end)


