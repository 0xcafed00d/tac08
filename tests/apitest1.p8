pico-8 cartridge // http://www.pico-8.com
version 16
__lua__
function _init()
	test1()
	test2()
	print ("tests finished")
end 

function test1()
	print ("test 1")

	for n = 0, 255 do
		fset(n, n)
	end
	for n = 0, 255 do
		local expect = n
		local got = fget(n)

		assert(expect == got, "expected: "..expect.." got: "..got)
	end
end

function test2()
	print ("test 2")

	for n = 0, 255 do
		fset(n, n)
		fset(n, band(n, 7), false)
		fset(n, band(n+1, 7), true)
	end
	for n = 0, 255 do
		local expect = false
		local got = fget(n, band(n, 7))

		assert(expect == got, "expected: "..tostr(expect).." got: "..tostr(got))

		local expect1 = true
		local got1 = fget(n, band(n+1, 7))

		assert(expect == got, "expected: "..tostr(expect1).." got: "..tostr(got1))

	end
end


function _update()
end 

function _draw()
end 
