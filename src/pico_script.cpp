#include "pico_script.h"

#include "hal_core.h"
#include "pico_audio.h"
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

	if (debug_coroutines) then 
		function cocreate(c)
			printh("cocreate")
			return coroutine.create(c)
		end

		function costatus(c) 
			printh("costatus")
			return coroutine.status(c)
		end

		function coresume(c) 
			printh("coresume")
			return coroutine.resume(c)
		end

		function yield(c) 
			printh("yield")
			return coroutine.yield(c)
		end
	else
		yield = coroutine.yield
		cocreate = coroutine.create
		coresume = coroutine.resume
		costatus = coroutine.status
	end

	-- constants for input/buttons
	⬅️ = 0
	➡️ = 1
	⬆️ = 2
	⬇️ = 3
	🅾 = 4
	❎ = 5

	-- TODO: implement these functions:

	function menuitem()
	end

	function flip() 
	end
)";

//#define API_TRACE

#ifdef API_TRACE

static void dump_func(lua_State* ls, const char* funcname) {
	std::cout << funcname + 5 << "(";
	int params = lua_gettop(ls);
	for (int n = 1; n <= params; n++) {
		auto s = luaL_tolstring(ls, n, nullptr);
		std::cout << s << ",";
		lua_remove(ls, -1);
	}
	std::cout << ")" << std::endl;
}

#define DEBUG_DUMP_FUNCTION         \
	pico_control::test_integrity(); \
	dump_func(ls, __FUNCTION__);
#else
#define DEBUG_DUMP_FUNCTION
#endif

static void init_scripting() {
	if (lstate) {
		lua_close(lstate);
	}

	lstate = luaL_newstate();
	luaL_openlibs(lstate);

	std::string fw = pico_cart::convert_emojis(firmware);

	lua_newtable(lstate);
	lua_setglobal(lstate, "__tac08__");

	throw_error(luaL_loadbuffer(lstate, fw.c_str(), fw.size(), "firmware"));
	throw_error(lua_pcall(lstate, 0, 0, 0));
}

static void register_cfunc(const char* name, lua_CFunction cf) {
	lua_pushcfunction(lstate, cf);
	lua_setglobal(lstate, name);
}

static void register_ext_cfunc(const char* name, lua_CFunction cf) {
	lua_getglobal(lstate, "__tac08__");
	lua_pushcfunction(lstate, cf);
	lua_setfield(lstate, -2, name);
	lua_pop(lstate, 1);
}

// ------------------------------------------------------------------
static int impl_cartdata(lua_State* ls) {
	DEBUG_DUMP_FUNCTION
	auto s = luaL_checkstring(ls, 1);
	if (s) {
		pico_api::cartdata(s);
	}
	return 0;
}

static int impl_cls(lua_State* ls) {
	DEBUG_DUMP_FUNCTION
	if (lua_gettop(ls) == 0) {
		pico_api::cls();
	} else {
		auto n = lua_tonumber(ls, 1);
		pico_api::cls(n);
	}
	return 0;
}

static int impl_poke(lua_State* ls) {
	DEBUG_DUMP_FUNCTION
	auto a = luaL_checknumber(ls, 1);
	auto v = luaL_checknumber(ls, 2);
	pico_api::poke(a, v);
	return 0;
}

static int impl_peek(lua_State* ls) {
	DEBUG_DUMP_FUNCTION
	auto a = luaL_checknumber(ls, 1);
	lua_pushnumber(ls, pico_api::peek(a));
	return 1;
}

static int impl_poke4(lua_State* ls) {
	DEBUG_DUMP_FUNCTION
	auto a = luaL_checknumber(ls, 1);
	auto v = luaL_checknumber(ls, 2);
	pico_api::poke4(a, v.bits());
	return 0;
}

static int impl_peek4(lua_State* ls) {
	DEBUG_DUMP_FUNCTION
	auto a = luaL_checknumber(ls, 1);
	uint32_t v = pico_api::peek4(a);
	lua_pushnumber(ls, z8::fix32::frombits(v));
	return 1;
}

static int impl_dget(lua_State* ls) {
	DEBUG_DUMP_FUNCTION
	auto a = luaL_checknumber(ls, 1);
	uint32_t v = pico_api::dget(a);
	lua_pushnumber(ls, z8::fix32::frombits(v));
	return 1;
}

static int impl_dset(lua_State* ls) {
	DEBUG_DUMP_FUNCTION
	auto a = luaL_checknumber(ls, 1);
	auto v = luaL_checknumber(ls, 2);
	pico_api::dset(a, v.bits());
	return 0;
}

static int impl_btn(lua_State* ls) {
	DEBUG_DUMP_FUNCTION
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
	DEBUG_DUMP_FUNCTION
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
	DEBUG_DUMP_FUNCTION
	auto x = lua_tonumber(ls, 1);
	auto y = lua_tonumber(ls, 2);

	lua_pushnumber(ls, pico_api::mget(x, y));
	return 1;
}

static int impl_mset(lua_State* ls) {
	DEBUG_DUMP_FUNCTION
	auto x = lua_tonumber(ls, 1);
	auto y = lua_tonumber(ls, 2);
	auto v = lua_tonumber(ls, 3);

	pico_api::mset(x, y, v);
	return 0;
}

static int impl_fget(lua_State* ls) {
	DEBUG_DUMP_FUNCTION
	auto n = lua_tonumber(ls, 1);
	if (lua_gettop(ls) == 1) {
		lua_pushnumber(ls, pico_api::fget(n));
	} else {
		auto index = lua_tonumber(ls, 2);
		lua_pushboolean(ls, pico_api::fget(n, index));
	}
	return 1;
}

static int impl_fset(lua_State* ls) {
	DEBUG_DUMP_FUNCTION
	auto n = lua_tonumber(ls, 1);
	if (lua_gettop(ls) > 2) {
		auto index = lua_tonumber(ls, 2);
		auto val = lua_toboolean(ls, 3);
		pico_api::fset(n, index, val);
	} else {
		auto val = lua_tonumber(ls, 2);
		pico_api::fset(n, val);
	}

	return 0;
}

static int impl_palt(lua_State* ls) {
	DEBUG_DUMP_FUNCTION
	if (lua_gettop(ls) == 0) {
		pico_api::palt();
	} else {
		auto a = lua_tonumber(ls, 1);
		auto f = lua_toboolean(ls, 2);
		pico_api::palt(a, f);
	}
	return 0;
}

static int impl_map(lua_State* ls) {
	DEBUG_DUMP_FUNCTION
	auto count = lua_gettop(ls);

	auto cell_x = lua_tonumber(ls, 1);
	auto cell_y = lua_tonumber(ls, 2);
	if (count <= 2) {
		pico_api::map(cell_x, cell_y);
		return 0;
	}

	auto screen_x = lua_tonumber(ls, 3);
	auto screen_y = lua_tonumber(ls, 4);
	if (count <= 4) {
		pico_api::map(cell_x, cell_y, screen_x, screen_y);
		return 0;
	}

	auto cell_w = lua_tonumber(ls, 5);
	auto cell_h = lua_tonumber(ls, 6);
	if (count <= 6) {
		pico_api::map(cell_x, cell_y, screen_x, screen_y, cell_w, cell_h);
		return 0;
	}

	auto layer = lua_tonumber(ls, 7);
	pico_api::map(cell_x, cell_y, screen_x, screen_y, cell_w, cell_h, layer);
	return 0;
}

static int impl_pal(lua_State* ls) {
	DEBUG_DUMP_FUNCTION
	if (lua_gettop(ls) == 0) {
		pico_api::pal();
	} else {
		auto a = lua_tonumber(ls, 1);
		auto b = lua_tonumber(ls, 2);
		pico_api::pal(a, b);
	}
	return 0;
}

static int impl_spr(lua_State* ls) {
	DEBUG_DUMP_FUNCTION
	auto n = lua_tonumber(ls, 1);
	auto x = lua_tonumber(ls, 2);
	auto y = lua_tonumber(ls, 3);

	if (lua_gettop(ls) <= 3) {
		pico_api::spr(n, x, y);
		return 0;
	}

	auto w = lua_tonumber(ls, 4);
	auto h = lua_tonumber(ls, 5);

	if (lua_gettop(ls) <= 5) {
		pico_api::spr(n, x, y, w, h);
		return 0;
	}

	auto flip_x = lua_toboolean(ls, 6);
	auto flip_y = lua_toboolean(ls, 7);

	pico_api::spr(n, x, y, w, h, flip_x, flip_y);

	return 0;
}

static int impl_sspr(lua_State* ls) {
	DEBUG_DUMP_FUNCTION
	auto sx = lua_tonumber(ls, 1);
	auto sy = lua_tonumber(ls, 2);
	auto sw = lua_tonumber(ls, 3);
	auto sh = lua_tonumber(ls, 4);
	auto dx = lua_tonumber(ls, 5);
	auto dy = lua_tonumber(ls, 6);

	if (lua_gettop(ls) <= 6) {
		pico_api::sspr(sx, sy, sw, sh, dx, dy);
		return 0;
	}

	auto dw = lua_tonumber(ls, 7);
	auto dh = lua_tonumber(ls, 8);
	auto flip_x = lua_toboolean(ls, 9);
	auto flip_y = lua_toboolean(ls, 10);
	pico_api::sspr(sx, sy, sw, sh, dx, dy, dw, dh, flip_x, flip_y);

	return 0;
}

static int impl_sset(lua_State* ls) {
	DEBUG_DUMP_FUNCTION
	auto x = lua_tonumber(ls, 1);
	auto y = lua_tonumber(ls, 2);

	if (lua_gettop(ls) <= 2) {
		pico_api::sset(x, y);
		return 0;
	}

	auto c = lua_tonumber(ls, 3);
	pico_api::sset(x, y, c);

	return 0;
}

static int impl_sget(lua_State* ls) {
	DEBUG_DUMP_FUNCTION
	auto x = lua_tonumber(ls, 1);
	auto y = lua_tonumber(ls, 2);

	lua_pushnumber(ls, pico_api::sget(x, y));
	return 1;
}

static int impl_print(lua_State* ls) {
	DEBUG_DUMP_FUNCTION

	if (lua_gettop(ls) == 1) {
		auto s = luaL_tolstring(ls, 1, nullptr);
		pico_api::print(s);
		lua_remove(ls, -1);
		return 0;
	}

	auto x = lua_tonumber(ls, 2);
	auto y = lua_tonumber(ls, 3);
	if (lua_gettop(ls) <= 3) {
		auto s = luaL_tolstring(ls, 1, nullptr);
		pico_api::print(s, x, y);
		lua_remove(ls, -1);
		return 0;
	}
	auto c = lua_tonumber(ls, 4);
	auto s = luaL_tolstring(ls, 1, nullptr);
	pico_api::print(s, x, y, c);
	lua_remove(ls, -1);
	return 0;
}

static int impl_cursor(lua_State* ls) {
	DEBUG_DUMP_FUNCTION

	auto x = lua_tonumber(ls, 1);
	auto y = lua_tonumber(ls, 2);
	pico_api::cursor(x, y);
	return 0;
}

static int impl_pget(lua_State* ls) {
	DEBUG_DUMP_FUNCTION
	auto x = lua_tonumber(ls, 1);
	auto y = lua_tonumber(ls, 2);

	pico_api::colour_t p = pico_api::pget(x, y);
	lua_pushnumber(ls, p);
	return 1;
}

static int impl_pset(lua_State* ls) {
	DEBUG_DUMP_FUNCTION
	auto x = lua_tonumber(ls, 1);
	auto y = lua_tonumber(ls, 2);

	if (lua_gettop(ls) <= 2) {
		pico_api::pset(x, y);
		return 0;
	}

	auto c = lua_tonumber(ls, 3);
	pico_api::pset(x, y, c);
	return 0;
}

static int impl_clip(lua_State* ls) {
	DEBUG_DUMP_FUNCTION
	if (lua_gettop(ls) == 0) {
		pico_api::clip();
	} else {
		auto x = lua_tonumber(ls, 1);
		auto y = lua_tonumber(ls, 2);
		auto w = lua_tonumber(ls, 3);
		auto h = lua_tonumber(ls, 4);

		pico_api::clip(x, y, w, h);
	}

	return 0;
}

static int impl_rectfill(lua_State* ls) {
	DEBUG_DUMP_FUNCTION
	auto x0 = lua_tonumber(ls, 1);
	auto y0 = lua_tonumber(ls, 2);
	auto x1 = lua_tonumber(ls, 3);
	auto y1 = lua_tonumber(ls, 4);

	if (lua_gettop(ls) <= 4) {
		pico_api::rectfill(x0, y0, x1, y1);
		return 0;
	}

	auto c = lua_tonumber(ls, 5);
	pico_api::rectfill(x0, y0, x1, y1, c);

	return 0;
}

static int impl_rect(lua_State* ls) {
	DEBUG_DUMP_FUNCTION
	auto x0 = lua_tonumber(ls, 1);
	auto y0 = lua_tonumber(ls, 2);
	auto x1 = lua_tonumber(ls, 3);
	auto y1 = lua_tonumber(ls, 4);

	if (lua_gettop(ls) <= 4) {
		pico_api::rect(x0, y0, x1, y1);
		return 0;
	}
	auto c = lua_tonumber(ls, 5);
	pico_api::rect(x0, y0, x1, y1, c);

	return 0;
}

static int impl_circfill(lua_State* ls) {
	DEBUG_DUMP_FUNCTION
	auto x = lua_tonumber(ls, 1);
	auto y = lua_tonumber(ls, 2);
	auto r = lua_tonumber(ls, 3);

	if (lua_gettop(ls) <= 3) {
		pico_api::circfill(x, y, r);
		return 0;
	}

	auto c = lua_tonumber(ls, 4);
	pico_api::circfill(x, y, r, c);

	return 0;
}

static int impl_circ(lua_State* ls) {
	DEBUG_DUMP_FUNCTION
	auto x = lua_tonumber(ls, 1);
	auto y = lua_tonumber(ls, 2);
	auto r = lua_tonumber(ls, 3);

	if (lua_gettop(ls) <= 3) {
		pico_api::circ(x, y, r);
		return 0;
	}
	auto c = lua_tonumber(ls, 4);
	pico_api::circ(x, y, r, c);

	return 0;
}

static int impl_line(lua_State* ls) {
	DEBUG_DUMP_FUNCTION
	auto x0 = lua_tonumber(ls, 1);
	auto y0 = lua_tonumber(ls, 2);
	auto x1 = lua_tonumber(ls, 3);
	auto y1 = lua_tonumber(ls, 4);

	if (lua_gettop(ls) <= 4) {
		pico_api::line(x0, y0, x1, y1);
		return 0;
	}

	auto c = lua_tonumber(ls, 5);
	pico_api::line(x0, y0, x1, y1, c);
	return 0;
}

static int impl_fillp(lua_State* ls) {
	DEBUG_DUMP_FUNCTION
	if (lua_gettop(ls) == 0) {
		pico_api::fillp();
	} else {
		auto n = lua_tonumber(ls, 1);
		pico_api::fillp(n);
	}
	return 0;
}

static int impl_time(lua_State* ls) {
	DEBUG_DUMP_FUNCTION
	uint64_t t = TIME_GetTime_ms();
	t = (t << 16) / 1000;
	lua_pushnumber(ls, z8::fix32::frombits(t));
	return 1;
}

static int impl_color(lua_State* ls) {
	DEBUG_DUMP_FUNCTION
	auto c = luaL_optnumber(ls, 1, 0);
	pico_api::color(c);
	return 0;
}

static int impl_camera(lua_State* ls) {
	DEBUG_DUMP_FUNCTION
	if (lua_gettop(ls) == 0) {
		pico_api::camera();
	} else {
		auto x = lua_tonumber(ls, 1);
		auto y = lua_tonumber(ls, 2);
		pico_api::camera(x, y);
	}
	return 0;
}

static int impl_stat(lua_State* ls) {
	DEBUG_DUMP_FUNCTION
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

static int impl_music(lua_State* ls) {
	DEBUG_DUMP_FUNCTION
	auto nargs = lua_gettop(ls);
	auto n = lua_tonumber(ls, 1);
	auto fadems = lua_tonumber(ls, 2);
	auto channelmask = lua_tonumber(ls, 3);

	if (nargs == 1) {
		pico_api::music(n);
	} else if (nargs == 2) {
		pico_api::music(n, fadems);
	} else {
		pico_api::music(n, fadems, channelmask);
	}

	return 0;
}

static int impl_sfx(lua_State* ls) {
	DEBUG_DUMP_FUNCTION
	auto nargs = lua_gettop(ls);
	auto n = lua_tonumber(ls, 1);
	auto channel = lua_tonumber(ls, 2);
	auto offset = lua_tonumber(ls, 3);
	auto length = lua_tonumber(ls, 4);

	if (nargs == 1) {
		pico_api::sfx(n);
	} else if (nargs == 2) {
		pico_api::sfx(n, channel);
	} else if (nargs == 3) {
		pico_api::sfx(n, channel, offset);
	} else {
		pico_api::sfx(n, channel, offset, length);
	}
	return 0;
}

static int impl_memcpy(lua_State* ls) {
	DEBUG_DUMP_FUNCTION
	auto src_a = lua_tonumber(ls, 1);
	auto dest_a = lua_tonumber(ls, 2);
	auto len = lua_tonumber(ls, 3);
	pico_api::memory_cpy(src_a, dest_a, len);
	return 0;
}

static int impl_memset(lua_State* ls) {
	DEBUG_DUMP_FUNCTION
	auto a = lua_tonumber(ls, 1);
	auto val = lua_tonumber(ls, 2);
	auto len = lua_tonumber(ls, 3);
	pico_api::memory_set(a, val, len);
	return 0;
}

static int implx_wrclip(lua_State* ls) {
	DEBUG_DUMP_FUNCTION
	auto s = luaL_checkstring(ls, 1);
	if (s) {
		pico_apix::wrclip(s);
	}
	return 0;
}

static int implx_rdclip(lua_State* ls) {
	DEBUG_DUMP_FUNCTION
	auto s = pico_apix::rdclip();
	lua_pushstring(ls, s.c_str());
	return 1;
}

static int implx_rdstr(lua_State* ls) {
	DEBUG_DUMP_FUNCTION
	auto name = luaL_checkstring(ls, 1);
	std::string res;
	if (name) {
		res = pico_apix::rdstr(name);
	}
	lua_pushstring(ls, res.c_str());
	return 1;
}

static int implx_wrstr(lua_State* ls) {
	DEBUG_DUMP_FUNCTION
	auto name = luaL_checkstring(ls, 1);
	auto s = luaL_checkstring(ls, 2);
	if (name && s) {
		pico_apix::wrstr(name, s);
	}
	return 0;
}

// ------------------------------------------------------------------

static void register_cfuncs() {
	register_cfunc("cartdata", impl_cartdata);
	register_cfunc("cls", impl_cls);
	register_cfunc("poke", impl_poke);
	register_cfunc("peek", impl_peek);
	register_cfunc("poke4", impl_poke4);
	register_cfunc("peek4", impl_peek4);
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
	register_cfunc("cursor", impl_cursor);
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
	register_cfunc("music", impl_music);
	register_cfunc("sfx", impl_sfx);
	register_cfunc("memcpy", impl_memcpy);
	register_cfunc("memset", impl_memset);
	register_ext_cfunc("wrclip", implx_wrclip);
	register_ext_cfunc("rdclip", implx_rdclip);
	register_ext_cfunc("wrstr", implx_wrstr);
	register_ext_cfunc("rdstr", implx_rdstr);
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

	bool run(std::string function, bool optional) {
		lua_getglobal(lstate, function.c_str());
		if (!lua_isfunction(lstate, -1)) {
			if (optional)
				return false;
			else
				throw pico_script::error(function + " not found");
		}
		throw_error(lua_pcall(lstate, 0, 0, 0));
		return true;
	}

}  // namespace pico_script
