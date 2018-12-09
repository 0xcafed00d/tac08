pico-8 cartridge // http://www.pico-8.com
version 16
__lua__
function _init()
	cls()
	cstore(0,0,0x7fff,"test1.p8")
	poke(0x3200+67,0)
	dump_ram(0x3200, 256)
	export("sfx_%d.wav")
	cstore(0,0,0x7fff,"test2.p8")
end 

_hex_digits = "0123456789abcdef"

function tohexstr (val, digits)
	local s = ""
	for n = 1, digits do
		local v = band(val, 0xf)
		s = sub(_hex_digits, v+1, v+1)..s
		val = val/16
	end
	return s
end


function dump_ram(addr, len)
	local s = ""

	for n = 0, len-1 do
		if band(n, 0xf) == 0 then
			printh(s)
			s = ""
			s = s..tohexstr(addr+n, 4)..": "
		end
		local v = peek(addr+n)
		s = s..tohexstr(v, 2).." "
	end
	printh(s)
end

function _update()

end 

function _draw()

end 
__sfx__
0010000424350303502435030350003000030000300003003f3000030000300003000030000300003000030000300183000030000300003000030000300003000030000300003000030000300003000030000300
001000000c0500c0500c0500c0500c0500c0500c0500c0500c0500c0500c0500c0500c0500c0500c0500c0500c0500c0500c0500c0500c0500c0500c0500c0500c0500c0500c0500c0500c0500c0500c0500c050
__music__
00 00014344
04 01024344

