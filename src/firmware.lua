static std::string firmware = R"(--"

-- namespace for extension & and internal api
__tac08__ = {}

printh = print

function all(a)
	if (a == nil) return function() end

	local i = 0
	local n = #a
	return function()
		if(i <= n) then
			i = i+1
			return a[i]
		end 
	end	
end

sub = string.sub

function add(a, val)
	if a != nil then 
		table.insert(a, val)
	end
	return val
end

function del(a, val)
	if a != nil then
		for k, v in ipairs(a) do
			if val == v then
				table.remove(a, k)
				return
			end
		end
	end
	return
end

function count(a)
	return #a
end

function foreach(a, f)
	for v in all(a) do
		f(v)
	end
end

__assert = assert
function assert(cond, msg) 
	if not cond then
		printh("assertion failed:")
		__assert (false, msg)
	end
end

yield = coroutine.yield
cocreate = coroutine.create
coresume = coroutine.resume
costatus = coroutine.status

-- constants for input/buttons
⬅️ = 0
➡️ = 1
⬆️ = 2
⬇️ = 3
🅾 = 4
❎ = 5

 -- api implemented in lua
function menuitem(idx, label, func)
	__tac08__.menu:menuitem(idx, label, func)
end

function flip() 
end

__tac08__.resetgame = function ()
	run()
end


-- pause menu 
__tac08__.menu = {
	items = {
				{"continue", nil}, 
				{nil, nil}, 
				{nil, nil}, 
				{nil, nil}, 
				{nil, nil}, 
				{nil, nil}, 
				{"reset", __tac08__.resetgame}
			},
	current = 0,
	bg = 0,
	t1 = 7,
	t2 = 6,

	resetmenu = function(m)
		for i = 1, 5 do
			m:menuitem(i, nil, nil)
			m.current = 0
		end
		m.color(0, 7, 6)
	end,

	color = function(m, bg, t1, t2)
		m.bg = bg
		m.t1 = t1
		m.t2 = t2
	end,

	menuitem = function(m, idx, label, func)
		if (idx < 1 or idx > 5) return
		m.items[idx+1][1] = label
		m.items[idx+1][2] = func
	end,

	-- returns true if menu is to dismissed
	update = function(m)
		if btnp(2) then
			repeat
				m.current = (m.current-1) % #m.items 
			until m.items[m.current+1][1]~=nil
		end 
		if btnp(3) then 
			repeat
				m.current = (m.current+1) % #m.items 
			until m.items[m.current+1][1]~=nil
		end
		if btnp(4) or btnp(5) or btnp(6) then
			local f = m.items[m.current+1][2]
			printh(m.items[m.current+1][1])
			if (f) f()
			return true; 
		end 
		if btnp(7) then
			return true; 
		end
		return false; 
	end, 

	draw = function(m)
		pal()
		local w = 0
		local h = 0 
		for i in all(m.items) do
			if i[1] ~= nil then
				w = max(w, #i[1])
				h+=1
			end
		end
		w = w*4+18
		h = (h+2)*8-4

		local x, y = 64 - w / 2, 64 - h / 2

		rectfill(x, y, x+w, y+h, m.bg)
		rect(x+1, y+1, x+w-1, y+h-1, m.t1)
		y += 8

		for i = 1, #m.items do
			if m.items[i][1] ~= nil then
				if i == m.current+1 then
					print (">", x+6, y, m.t1)
					print (m.items[i][1], x+12, y, m.t1)
				else
					print (m.items[i][1], x+10, y, m.t2)
				end
				y += 8
			end
		end
	end
}

__tac08__.do_menu = function()
	if __tac08__.menu:update() then
		return true
	else
		__tac08__.menu:draw()
		return false
	end
end


--" )";
