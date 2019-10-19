pico-8 cartridge // http://www.pico-8.com
version 16
__lua__

local all_chars = 
[[▮■□⁙⁘‖◀▶「」¥•、。゛゜ !"#$%&'()*+,-./
0123456789:;<=>?@𝘢𝘣𝘤𝘥𝘦𝘧𝘨𝘩𝘪𝘫𝘬𝘭𝘮𝘯𝘰
𝘱𝘲𝘳𝘴𝘵𝘶𝘷𝘸𝘹𝘺𝘻[\]^_`abcdefghijklmno
pqrstuvwxyz{|}~○█▒🐱⬇️░✽●♥
☉웃⌂⬅️😐♪🅾️◆…➡️★⧗⬆️ˇ∧❎
▤▥あいうえおかきくけこさしすせ
そたちつてとなにぬねのはひふへほ
まみむめもやゆよらりるれろわをん
っゃゅょアイウエオカキクケコサシ
スセソタチツテトナニヌネノハヒフ
ヘホマミムメモヤユヨラリルレロワ
ヲンッャュョ◜◝]]


local s = "█▒🐱⬇️░✽●♥☉웃⌂⬅️😐♪🅾️◆…➡️★⧗⬆️ˇ∧❎▤▥"

local internal_str = ""

function _init()
	if __tac08__ then
		__tac08__.screen(128, 192)
	end
	
	for n = 0x10, 0xff do
		internal_str = internal_str..chr(n)
	end
end


function _update()
end


local c = 0x10
function _draw()
	cls()
	color(7)

	local x, y = 0, 0
	
	for n = 0, 12 do
		print(internal_str, x, y)
		x = x - 128
		y = y + 6
	end

	print(all_chars)

end
