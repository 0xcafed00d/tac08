#include "pico_script.h"
#include "hal_core.h"
#include "pico_cart.h"
#include "pico_core.h"

#include "z8lua/lauxlib.h"
#include "z8lua/lua.h"
#include "z8lua/lualib.h"

#include <iostream>

static lua_State* lstate = nullptr;

static void throw_error(int err) {
	if (err) {
		pico_script::error e(lua_tostring(lstate, -1));
		lua_pop(lstate, 1);
		throw e;
	}
}

static std::string firmware = R"(
	
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
	add = table.insert
	function del(a, val)
		for k, v in ipairs(a) do
			if val == v then
				table.remove(a, k)
				return
			end
		end
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


	-- TODO: implement these functions:

	function sfx(a, b)
	end

	function music()
	end


	function menuitem()
	end

	⬇️ = 3
	⬅️ = 0
	➡️ = 1
	⬆️ = 2

	function flip() 
	end

)";

static void init_scripting() {
	if (lstate) {
		lua_close(lstate);
	}

	lstate = luaL_newstate();
	luaL_openlibs(lstate);

	std::string fw = pico_cart::convert_emojis(firmware);

	throw_error(luaL_loadbuffer(lstate, fw.c_str(), fw.size(), "firmware"));
	throw_error(lua_pcall(lstate, 0, 0, 0));
}

static void register_cfunc(const char* name, lua_CFunction cf) {
	lua_pushcfunction(lstate, cf);
	lua_setglobal(lstate, name);
}

// ------------------------------------------------------------------
static int impl_cartdata(lua_State* ls) {
	auto s = luaL_checkstring(ls, 1);
	// TODO: implement
	return 0;
}

static int impl_cls(lua_State* ls) {
	if (lua_gettop(ls) == 0) {
		pico_api::cls();
	} else {
		auto n = luaL_checknumber(ls, 1);
		pico_api::cls(n);
	}
	return 0;
}

static int impl_poke(lua_State* ls) {
	auto a = luaL_checknumber(ls, 1);
	auto v = luaL_checknumber(ls, 2);
	// TODO: implement
	return 0;
}

static int impl_peek(lua_State* ls) {
	auto a = luaL_checknumber(ls, 1);
	lua_pushnumber(ls, 0);

	// TODO: implement
	return 1;
}

static int impl_dget(lua_State* ls) {
	auto a = luaL_checknumber(ls, 1);
	lua_pushnumber(ls, 0);
	// TODO: implement
	return 1;
}

static int impl_dset(lua_State* ls) {
	auto a = luaL_checknumber(ls, 1);
	// TODO: implement
	return 0;
}

static int impl_btn(lua_State* ls) {
	if (lua_gettop(ls) == 0) {
		auto val = pico_api::btn();
		lua_pushnumber(ls, val);
		return 1;
	}

	auto n = luaL_checknumber(ls, 1);
	auto p = luaL_optnumber(ls, 2, 0);

	auto val = pico_api::btn(n, p);

	lua_pushboolean(ls, val);
	return 1;
}

static int impl_btnp(lua_State* ls) {
	if (lua_gettop(ls) == 0) {
		auto val = pico_api::btnp();
		lua_pushnumber(ls, val);
		return 1;
	}

	auto n = luaL_checknumber(ls, 1);
	auto p = luaL_optnumber(ls, 2, 0);

	auto val = pico_api::btnp(n, p);

	lua_pushboolean(ls, val);
	return 1;
}

static int impl_mget(lua_State* ls) {
	auto x = luaL_checknumber(ls, 1);
	auto y = luaL_checknumber(ls, 2);

	lua_pushnumber(ls, pico_api::mget(x, y));
	return 1;
}

static int impl_mset(lua_State* ls) {
	auto x = luaL_checknumber(ls, 1);
	auto y = luaL_checknumber(ls, 2);
	auto v = luaL_checknumber(ls, 3);

	pico_api::mset(x, y, v);
	return 0;
}

static int impl_fget(lua_State* ls) {
	auto n = luaL_checknumber(ls, 1);
	if (lua_gettop(ls) == 1) {
		lua_pushnumber(ls, pico_api::fget(n));
	} else {
		auto index = luaL_checknumber(ls, 2);
		lua_pushboolean(ls, pico_api::fget(n, index));
	}
	return 1;
}

static int impl_fset(lua_State* ls) {
	auto n = luaL_checknumber(ls, 1);
	if (lua_gettop(ls) > 2) {
		auto index = luaL_checknumber(ls, 2);
		auto val = lua_toboolean(ls, 3);
		pico_api::fset(n, index, val);
	} else {
		auto val = luaL_checknumber(ls, 2);
		pico_api::fset(n, val);
	}

	return 0;
}

static int impl_palt(lua_State* ls) {
	if (lua_gettop(ls) == 0) {
		pico_api::palt();
	} else {
		auto a = luaL_checknumber(ls, 1);
		auto f = lua_toboolean(ls, 2);  // TODO: check boolean conversion
		pico_api::palt(a, f);
	}
	return 0;
}

static int impl_map(lua_State* ls) {
	auto count = lua_gettop(ls);

	auto cell_x = luaL_checknumber(ls, 1);
	auto cell_y = luaL_checknumber(ls, 2);
	if (count == 2) {
		pico_api::map(cell_x, cell_y);
		return 0;
	}

	auto screen_x = luaL_checknumber(ls, 3);
	auto screen_y = luaL_checknumber(ls, 4);
	if (count == 4) {
		pico_api::map(cell_x, cell_y, screen_x, screen_y);
		return 0;
	}

	auto cell_w = luaL_checknumber(ls, 5);
	auto cell_h = luaL_checknumber(ls, 6);
	if (count == 6) {
		pico_api::map(cell_x, cell_y, screen_x, screen_y, cell_w, cell_y);
		return 0;
	}

	auto layer = luaL_checknumber(ls, 7);
	pico_api::map(cell_x, cell_y, screen_x, screen_y, cell_w, cell_h, layer);
	return 0;
}

static int impl_pal(lua_State* ls) {
	if (lua_gettop(ls) == 2) {
		auto a = luaL_checknumber(ls, 1);
		auto b = luaL_checknumber(ls, 2);
		pico_api::pal(a, b);
	} else {
		pico_api::pal();
	}
	return 0;
}

static int impl_spr(lua_State* ls) {
	auto n = luaL_checknumber(ls, 1);
	auto x = luaL_checknumber(ls, 2);
	auto y = luaL_checknumber(ls, 3);

	if (lua_gettop(ls) == 3) {
		pico_api::spr(n, x, y);
		return 0;
	}

	auto w = luaL_checknumber(ls, 4);
	auto h = luaL_checknumber(ls, 5);

	if (lua_gettop(ls) == 5) {
		pico_api::spr(n, x, y, w, h);
		return 0;
	}

	auto flip_x = lua_toboolean(ls, 6);
	auto flip_y = lua_toboolean(ls, 7);

	pico_api::spr(n, x, y, w, h, flip_x, flip_y);

	return 0;
}

static int impl_sspr(lua_State* ls) {
	auto sx = luaL_checknumber(ls, 1);
	auto sy = luaL_checknumber(ls, 2);
	auto sw = luaL_checknumber(ls, 3);
	auto sh = luaL_checknumber(ls, 4);
	auto dx = luaL_checknumber(ls, 5);
	auto dy = luaL_checknumber(ls, 6);

	pico_api::sspr(sx, sy, sw, sh, dx, dy);

	return 0;
}

static int impl_sset(lua_State* ls) {
	auto x = luaL_checknumber(ls, 1);
	auto y = luaL_checknumber(ls, 2);

	if (lua_gettop(ls) == 2) {
		pico_api::sset(x, y);
		return 0;
	}

	auto c = luaL_checknumber(ls, 3);
	pico_api::sset(x, y, c);

	return 0;
}

static int impl_sget(lua_State* ls) {
	auto x = luaL_checknumber(ls, 1);
	auto y = luaL_checknumber(ls, 2);

	lua_pushnumber(ls, pico_api::sget(x, y));
	return 1;
}

static int impl_print(lua_State* ls) {
	auto s = luaL_tolstring(ls, 1, nullptr);
	lua_remove(ls, -1);

	if (lua_gettop(ls) == 1) {
		pico_api::print(s);
		return 0;
	}

	auto x = luaL_checknumber(ls, 2);
	auto y = luaL_checknumber(ls, 3);
	if (lua_gettop(ls) == 3) {
		pico_api::print(s, x, y);
		return 0;
	}
	auto c = luaL_checknumber(ls, 4);
	pico_api::print(s, x, y, c);
	return 0;
}

static int impl_pget(lua_State* ls) {
	auto a = luaL_checknumber(ls, 1);
	lua_pushnumber(ls, 0);
	// TODO: implement
	return 1;
}

static int impl_pset(lua_State* ls) {
	auto x = luaL_checknumber(ls, 1);
	auto y = luaL_checknumber(ls, 2);

	if (lua_gettop(ls) == 2) {
		pico_api::pset(x, y);
		return 0;
	}

	auto c = luaL_checknumber(ls, 3);
	pico_api::pset(x, y, c);
	return 0;
}

static int impl_clip(lua_State* ls) {
	if (lua_gettop(ls) == 0) {
		pico_api::clip();
	} else {
		auto x = luaL_checknumber(ls, 1);
		auto y = luaL_checknumber(ls, 2);
		auto w = luaL_checknumber(ls, 3);
		auto h = luaL_checknumber(ls, 4);

		pico_api::clip(x, y, w, h);
	}

	return 0;
}

static int impl_rectfill(lua_State* ls) {
	auto x0 = luaL_checknumber(ls, 1);
	auto y0 = luaL_checknumber(ls, 2);
	auto x1 = luaL_checknumber(ls, 3);
	auto y1 = luaL_checknumber(ls, 4);

	if (lua_gettop(ls) == 4) {
		pico_api::rectfill(x0, y0, x1, y1);
		return 0;
	}

	if (lua_isnil(ls, 5)) {
		pico_api::rectfill(x0, y0, x1, y1, 0);
	} else {
		auto c = luaL_checknumber(ls, 5);
		pico_api::rectfill(x0, y0, x1, y1, c);
	}

	return 0;
}

static int impl_rect(lua_State* ls) {
	auto x0 = luaL_checknumber(ls, 1);
	auto y0 = luaL_checknumber(ls, 2);
	auto x1 = luaL_checknumber(ls, 3);
	auto y1 = luaL_checknumber(ls, 4);

	if (lua_gettop(ls) == 4) {
		pico_api::rect(x0, y0, x1, y1);
		return 0;
	}

	if (lua_isnil(ls, 5)) {
		pico_api::rect(x0, y0, x1, y1, 0);
	} else {
		auto c = luaL_checknumber(ls, 5);
		pico_api::rect(x0, y0, x1, y1, c);
	}

	return 0;
}

static int impl_circfill(lua_State* ls) {
	auto x = luaL_checknumber(ls, 1);
	auto y = luaL_checknumber(ls, 2);
	auto r = luaL_checknumber(ls, 3);

	if (lua_gettop(ls) == 3) {
		pico_api::circfill(x, y, r);
		return 0;
	}

	if (lua_isnil(ls, 4)) {
		pico_api::circfill(x, y, r, 0);
	} else {
		auto c = luaL_checknumber(ls, 4);
		pico_api::circfill(x, y, r, c);
	}

	return 0;
}

static int impl_circ(lua_State* ls) {
	auto x = luaL_checknumber(ls, 1);
	auto y = luaL_checknumber(ls, 2);
	auto r = luaL_checknumber(ls, 3);

	if (lua_gettop(ls) == 3) {
		pico_api::circ(x, y, r);
		return 0;
	}

	if (lua_isnil(ls, 4)) {
		pico_api::circ(x, y, r, 0);
	} else {
		auto c = luaL_checknumber(ls, 4);
		pico_api::circ(x, y, r, c);
	}

	return 0;
}

static int impl_line(lua_State* ls) {
	auto x0 = luaL_checknumber(ls, 1);
	auto y0 = luaL_checknumber(ls, 2);
	auto x1 = luaL_checknumber(ls, 3);
	auto y1 = luaL_checknumber(ls, 4);

	if (lua_gettop(ls) == 4) {
		pico_api::line(x0, y0, x1, y1);
		return 0;
	}

	if (lua_isnil(ls, 5)) {
		pico_api::line(x0, y0, x1, y1, 0);
	} else {
		auto c = luaL_checknumber(ls, 5);
		pico_api::line(x0, y0, x1, y1, c);
	}
	return 0;
}

static int impl_fillp(lua_State* ls) {
	if (lua_gettop(ls) == 0) {
		pico_api::fillp();
	} else {
		auto n = luaL_checknumber(ls, 1);
		pico_api::fillp(n);
	}
	return 0;
}

static int impl_time(lua_State* ls) {
	float t = TIME_GetTicks();
	t = t / 1000.0f;
	lua_pushnumber(ls, t);
	return 1;
}

static int impl_color(lua_State* ls) {
	auto c = luaL_checknumber(ls, 1);
	pico_api::color(c);
	return 0;
}

static int impl_camera(lua_State* ls) {
	if (lua_gettop(ls) == 0) {
		pico_api::camera();
	} else {
		auto x = luaL_checknumber(ls, 1);
		auto y = luaL_checknumber(ls, 2);
		pico_api::camera(x, y);
	}
	return 0;
}

static int impl_stat(lua_State* ls) {
	auto k = luaL_checknumber(ls, 1);

	std::string s;
	int i;

	auto v = pico_api::stat(k, s, i);

	if (v == 1)
		lua_pushstring(ls, s.c_str());
	else if (v == 2)
		lua_pushnumber(ls, i);
	else
		lua_pushnil(ls);

	return 1;
}

// ------------------------------------------------------------------

static void register_cfuncs() {
	register_cfunc("cartdata", impl_cartdata);
	register_cfunc("cls", impl_cls);
	register_cfunc("poke", impl_poke);
	register_cfunc("peek", impl_peek);
	register_cfunc("dget", impl_dget);
	register_cfunc("dset", impl_dset);
	register_cfunc("btn", impl_btn);
	register_cfunc("btnp", impl_btnp);
	register_cfunc("mget", impl_mget);
	register_cfunc("mset", impl_mset);
	register_cfunc("fget", impl_fget);
	register_cfunc("fset", impl_fset);
	register_cfunc("palt", impl_palt);
	register_cfunc("map", impl_map);
	register_cfunc("pal", impl_pal);
	register_cfunc("sget", impl_sget);
	register_cfunc("sset", impl_sset);
	register_cfunc("spr", impl_spr);
	register_cfunc("sspr", impl_sspr);
	register_cfunc("print", impl_print);
	register_cfunc("pget", impl_pget);
	register_cfunc("pset", impl_pset);
	register_cfunc("clip", impl_clip);
	register_cfunc("rectfill", impl_rectfill);
	register_cfunc("rect", impl_rect);
	register_cfunc("circfill", impl_circfill);
	register_cfunc("circ", impl_circ);
	register_cfunc("line", impl_line);
	register_cfunc("fillp", impl_fillp);
	register_cfunc("time", impl_time);
	register_cfunc("t", impl_time);
	register_cfunc("color", impl_color);
	register_cfunc("camera", impl_camera);
	register_cfunc("stat", impl_stat);
}

namespace pico_script {

	void load(std::string script) {
		init_scripting();
		register_cfuncs();

		throw_error(luaL_loadbuffer(lstate, script.c_str(), script.size(), "main"));
		throw_error(lua_pcall(lstate, 0, 0, 0));
	}

	void reset() {
	}

	void run(std::string function, bool optional) {
		lua_getglobal(lstate, function.c_str());
		if (!lua_isfunction(lstate, -1)) {
			if (optional)
				return;
			else
				throw pico_script::error(function + " not found");
		}
		throw_error(lua_pcall(lstate, 0, 0, 0));
	}

}  // namespace pico_script
