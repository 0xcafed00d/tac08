#include <assert.h>
#include <cstring>
#include <deque>
#include <functional>
#include <iostream>
#include <set>

#include "pico_script.h"

#include "hal_audio.h"
#include "hal_core.h"
#include "log.h"
#include "pico_audio.h"
#include "pico_cart.h"
#include "pico_core.h"

#include "z8lua/lauxlib.h"
#include "z8lua/lua.h"
#include "z8lua/lualib.h"

#include "firmware.lua"

static lua_State* lstate = nullptr;

typedef std::function<void()> deferredAPICall_t;
static std::deque<deferredAPICall_t> deferredAPICalls;
static bool traceAPI = false;

static void throw_error(int err) {
	if (err) {
		std::string msg = lua_tostring(lstate, -1);
		logr << msg;
		auto errlnstart = msg.find(":");
		auto errlnend = msg.find(":", errlnstart + 1);
		int errline = std::stoi(msg.substr(errlnstart + 1, errlnend - errlnstart - 1)) - 1;

		auto li = pico_cart::getLineInfo(pico_cart::getCart(), errline);

		std::stringstream ss;
		ss << li.filename << ":" << li.localLineNum << ":" << li.sourceLine << msg.substr(errlnend);

		pico_script::error e(ss.str());
		lua_pop(lstate, 1);
		throw e;
	}
}

static void dump_func(lua_State* ls, const char* funcname) {
	std::stringstream str;

	str << funcname + 5 << "(";
	int params = lua_gettop(ls);
	for (int n = 1; n <= params; n++) {
		auto s = luaL_tolstring(ls, n, nullptr);
		str << s << ",";
		lua_remove(ls, -1);
	}
	str << ")";
	logr << str.str();
}

#define DEBUG_DUMP_FUNCTION                  \
	if (traceAPI | DEBUG_Trace()) {          \
		/* pico_control::test_integrity();*/ \
		dump_func(ls, __FUNCTION__);         \
	}

static void register_cfuncs();

static void init_scripting() {
	lstate = luaL_newstate();
	luaL_openlibs(lstate);
	luaopen_debug(lstate);
	luaopen_string(lstate);

	traceAPI = false;

	std::string fw = pico_cart::convert_emojis(firmware);

	throw_error(luaL_loadbuffer(lstate, fw.c_str(), fw.size(), "firmware"));
	throw_error(lua_pcall(lstate, 0, 0, 0));

	register_cfuncs();
	luaL_dostring(lstate, "__tac08__.make_api_list()");
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
// Lua accesable API
// ------------------------------------------------------------------

static int impl_load(lua_State* ls) {
	DEBUG_DUMP_FUNCTION
	auto s = luaL_checkstring(ls, 1);
	if (s) {
		deferredAPICalls.push_back([=]() { pico_api::load(s); });
	}
	return 0;
}

static int impl_run(lua_State* ls) {
	DEBUG_DUMP_FUNCTION
	deferredAPICalls.push_back([]() { pico_api::reload(); });
	return 0;
}

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
		auto n = lua_tonumber(ls, 1).toInt();
		pico_api::cls(n);
	}
	return 0;
}

static int impl_poke(lua_State* ls) {
	DEBUG_DUMP_FUNCTION
	auto a = luaL_checknumber(ls, 1).toInt();
	auto v = luaL_checknumber(ls, 2).toInt();
	pico_api::poke(a, v);
	return 0;
}

static int impl_peek(lua_State* ls) {
	DEBUG_DUMP_FUNCTION
	auto a = luaL_checknumber(ls, 1).toInt();
	lua_pushnumber(ls, pico_api::peek(a));
	return 1;
}

static int impl_poke2(lua_State* ls) {
	DEBUG_DUMP_FUNCTION
	auto a = luaL_checknumber(ls, 1).toInt();
	auto v = luaL_checknumber(ls, 2);
	pico_api::poke2(a, v.bits());
	return 0;
}

static int impl_peek2(lua_State* ls) {
	DEBUG_DUMP_FUNCTION
	auto a = luaL_checknumber(ls, 1).toInt();
	uint16_t v = pico_api::peek2(a);
	lua_pushnumber(ls, z8::fix32::frombits(v));
	return 1;
}

static int impl_poke4(lua_State* ls) {
	DEBUG_DUMP_FUNCTION
	auto a = luaL_checknumber(ls, 1).toInt();
	auto v = luaL_checknumber(ls, 2);
	pico_api::poke4(a, v.bits());
	return 0;
}

static int impl_peek4(lua_State* ls) {
	DEBUG_DUMP_FUNCTION
	auto a = luaL_checknumber(ls, 1).toInt();
	uint32_t v = pico_api::peek4(a);
	lua_pushnumber(ls, z8::fix32::frombits(v));
	return 1;
}

static int impl_dget(lua_State* ls) {
	DEBUG_DUMP_FUNCTION
	auto a = luaL_checknumber(ls, 1).toInt();
	uint32_t v = pico_api::dget(a);
	lua_pushnumber(ls, z8::fix32::frombits(v));
	return 1;
}

static int impl_dset(lua_State* ls) {
	DEBUG_DUMP_FUNCTION
	auto a = luaL_checknumber(ls, 1).toInt();
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

	auto n = luaL_checknumber(ls, 1).toInt();
	auto p = luaL_optnumber(ls, 2, 0).toInt();

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

	auto n = luaL_checknumber(ls, 1).toInt();
	auto p = luaL_optnumber(ls, 2, 0).toInt();

	auto val = pico_api::btnp(n, p);

	lua_pushboolean(ls, val);
	return 1;
}

static int impl_mget(lua_State* ls) {
	DEBUG_DUMP_FUNCTION
	auto x = lua_tonumber(ls, 1).toInt();
	auto y = lua_tonumber(ls, 2).toInt();

	lua_pushnumber(ls, pico_api::mget(x, y));
	return 1;
}

static int impl_mset(lua_State* ls) {
	DEBUG_DUMP_FUNCTION
	auto x = lua_tonumber(ls, 1).toInt();
	auto y = lua_tonumber(ls, 2).toInt();
	auto v = lua_tonumber(ls, 3).toInt();

	pico_api::mset(x, y, v);
	return 0;
}

static int impl_fget(lua_State* ls) {
	DEBUG_DUMP_FUNCTION
	auto n = lua_tonumber(ls, 1).toInt();
	if (lua_gettop(ls) == 1) {
		lua_pushnumber(ls, pico_api::fget(n));
	} else {
		auto index = lua_tonumber(ls, 2).toInt();
		lua_pushboolean(ls, pico_api::fget(n, index));
	}
	return 1;
}

static int impl_fset(lua_State* ls) {
	DEBUG_DUMP_FUNCTION
	auto n = lua_tonumber(ls, 1).toInt();
	if (lua_gettop(ls) > 2) {
		auto index = lua_tonumber(ls, 2).toInt();
		auto val = lua_toboolean(ls, 3);
		pico_api::fset(n, index, val);
	} else {
		auto val = lua_tonumber(ls, 2).toInt();
		pico_api::fset(n, val);
	}

	return 0;
}

static int impl_palt(lua_State* ls) {
	DEBUG_DUMP_FUNCTION
	if (lua_gettop(ls) == 0) {
		pico_api::palt();
	} else {
		auto a = lua_tonumber(ls, 1).toInt();
		auto f = lua_toboolean(ls, 2);
		pico_api::palt(a, f);
	}
	return 0;
}

static int impl_map(lua_State* ls) {
	DEBUG_DUMP_FUNCTION
	auto count = lua_gettop(ls);

	auto cell_x = lua_tonumber(ls, 1).toInt();
	auto cell_y = lua_tonumber(ls, 2).toInt();
	if (count <= 2) {
		pico_api::map(cell_x, cell_y);
		return 0;
	}

	auto screen_x = lua_tonumber(ls, 3).toInt();
	auto screen_y = lua_tonumber(ls, 4).toInt();
	if (count <= 4) {
		pico_api::map(cell_x, cell_y, screen_x, screen_y);
		return 0;
	}

	auto cell_w = lua_tonumber(ls, 5).toInt();
	auto cell_h = lua_tonumber(ls, 6).toInt();
	if (count <= 6) {
		pico_api::map(cell_x, cell_y, screen_x, screen_y, cell_w, cell_h);
		return 0;
	}

	auto layer = lua_tonumber(ls, 7).toInt();
	pico_api::map(cell_x, cell_y, screen_x, screen_y, cell_w, cell_h, layer);
	return 0;
}

static int impl_pal(lua_State* ls) {
	DEBUG_DUMP_FUNCTION
	if (lua_gettop(ls) == 0) {
		pico_api::pal();
	} else {
		auto a = lua_tonumber(ls, 1).toInt();
		auto b = lua_tonumber(ls, 2).toInt();
		pico_api::pal(a, b);
	}
	return 0;
}

static int impl_spr(lua_State* ls) {
	DEBUG_DUMP_FUNCTION
	auto n = lua_tonumber(ls, 1).toInt();
	auto x = lua_tonumber(ls, 2).toInt();
	auto y = lua_tonumber(ls, 3).toInt();

	if (lua_gettop(ls) <= 3) {
		pico_api::spr(n, x, y);
		return 0;
	}

	auto w = lua_tonumber(ls, 4).toInt();
	auto h = lua_tonumber(ls, 5).toInt();

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
	auto sx = lua_tonumber(ls, 1).toInt();
	auto sy = lua_tonumber(ls, 2).toInt();
	auto sw = lua_tonumber(ls, 3).toInt();
	auto sh = lua_tonumber(ls, 4).toInt();
	auto dx = lua_tonumber(ls, 5).toInt();
	auto dy = lua_tonumber(ls, 6).toInt();

	if (lua_gettop(ls) <= 6) {
		pico_api::sspr(sx, sy, sw, sh, dx, dy);
		return 0;
	}

	auto dw = lua_tonumber(ls, 7).toInt();
	auto dh = lua_tonumber(ls, 8).toInt();
	auto flip_x = lua_toboolean(ls, 9);
	auto flip_y = lua_toboolean(ls, 10);
	pico_api::sspr(sx, sy, sw, sh, dx, dy, dw, dh, flip_x, flip_y);

	return 0;
}

static int impl_sset(lua_State* ls) {
	DEBUG_DUMP_FUNCTION
	auto x = lua_tonumber(ls, 1).toInt();
	auto y = lua_tonumber(ls, 2).toInt();

	if (lua_gettop(ls) <= 2) {
		pico_api::sset(x, y);
		return 0;
	}

	auto c = lua_tonumber(ls, 3).toInt();
	pico_api::sset(x, y, c);

	return 0;
}

static int impl_sget(lua_State* ls) {
	DEBUG_DUMP_FUNCTION
	auto x = lua_tonumber(ls, 1).toInt();
	auto y = lua_tonumber(ls, 2).toInt();

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

	auto x = lua_tonumber(ls, 2).toInt();
	auto y = lua_tonumber(ls, 3).toInt();
	if (lua_gettop(ls) <= 3) {
		auto s = luaL_tolstring(ls, 1, nullptr);
		pico_api::print(s, x, y);
		lua_remove(ls, -1);
		return 0;
	}
	auto c = lua_tonumber(ls, 4).toInt();
	auto s = luaL_tolstring(ls, 1, nullptr);
	pico_api::print(s, x, y, c);
	lua_remove(ls, -1);
	return 0;
}

static int impl_cursor(lua_State* ls) {
	DEBUG_DUMP_FUNCTION

	auto x = lua_tonumber(ls, 1).toInt();
	auto y = lua_tonumber(ls, 2).toInt();
	if (lua_gettop(ls) <= 2) {
		pico_api::cursor(x, y);
	} else {
		auto c = lua_tonumber(ls, 3).toInt();
		pico_api::cursor(x, y, c);
	}
	return 0;
}

static int impl_pget(lua_State* ls) {
	DEBUG_DUMP_FUNCTION
	auto x = lua_tonumber(ls, 1).toInt();
	auto y = lua_tonumber(ls, 2).toInt();

	pico_api::colour_t p = pico_api::pget(x, y);
	lua_pushnumber(ls, p);
	return 1;
}

static int impl_pset(lua_State* ls) {
	DEBUG_DUMP_FUNCTION
	auto x = lua_tonumber(ls, 1).toInt();
	auto y = lua_tonumber(ls, 2).toInt();

	if (lua_gettop(ls) <= 2) {
		pico_api::pset(x, y);
		return 0;
	}

	auto c = lua_tonumber(ls, 3).toInt();
	pico_api::pset(x, y, c);
	return 0;
}

static int impl_clip(lua_State* ls) {
	DEBUG_DUMP_FUNCTION
	if (lua_gettop(ls) == 0) {
		pico_api::clip();
	} else {
		auto x = lua_tonumber(ls, 1).toInt();
		auto y = lua_tonumber(ls, 2).toInt();
		auto w = lua_tonumber(ls, 3).toInt();
		auto h = lua_tonumber(ls, 4).toInt();

		pico_api::clip(x, y, w, h);
	}

	return 0;
}

static int impl_rectfill(lua_State* ls) {
	DEBUG_DUMP_FUNCTION
	auto x0 = lua_tonumber(ls, 1).toInt();
	auto y0 = lua_tonumber(ls, 2).toInt();
	auto x1 = lua_tonumber(ls, 3).toInt();
	auto y1 = lua_tonumber(ls, 4).toInt();

	if (lua_gettop(ls) <= 4) {
		pico_api::rectfill(x0, y0, x1, y1);
		return 0;
	}

	auto c = lua_tonumber(ls, 5).toInt();
	pico_api::rectfill(x0, y0, x1, y1, c);

	return 0;
}

static int impl_rect(lua_State* ls) {
	DEBUG_DUMP_FUNCTION
	auto x0 = lua_tonumber(ls, 1).toInt();
	auto y0 = lua_tonumber(ls, 2).toInt();
	auto x1 = lua_tonumber(ls, 3).toInt();
	auto y1 = lua_tonumber(ls, 4).toInt();

	if (lua_gettop(ls) <= 4) {
		pico_api::rect(x0, y0, x1, y1);
		return 0;
	}
	auto c = lua_tonumber(ls, 5).toInt();
	pico_api::rect(x0, y0, x1, y1, c);

	return 0;
}

static int impl_circfill(lua_State* ls) {
	DEBUG_DUMP_FUNCTION
	auto x = lua_tonumber(ls, 1).toInt();
	auto y = lua_tonumber(ls, 2).toInt();
	auto r = lua_tonumber(ls, 3).toInt();

	if (lua_gettop(ls) <= 3) {
		pico_api::circfill(x, y, r);
		return 0;
	}

	auto c = lua_tonumber(ls, 4).toInt();
	pico_api::circfill(x, y, r, c);

	return 0;
}

static int impl_circ(lua_State* ls) {
	DEBUG_DUMP_FUNCTION
	auto x = lua_tonumber(ls, 1).toInt();
	auto y = lua_tonumber(ls, 2).toInt();
	auto r = lua_tonumber(ls, 3).toInt();

	if (lua_gettop(ls) <= 3) {
		pico_api::circ(x, y, r);
		return 0;
	}
	auto c = lua_tonumber(ls, 4).toInt();
	pico_api::circ(x, y, r, c);

	return 0;
}

static int impl_line(lua_State* ls) {
	DEBUG_DUMP_FUNCTION
	auto x0 = lua_tonumber(ls, 1).toInt();
	auto y0 = lua_tonumber(ls, 2).toInt();

	if (lua_gettop(ls) <= 2) {
		pico_api::line(x0, y0);
		return 0;
	}

	auto x1 = lua_tonumber(ls, 3).toInt();
	auto y1 = lua_tonumber(ls, 4).toInt();

	if (lua_gettop(ls) <= 4) {
		pico_api::line(x0, y0, x1, y1);
		return 0;
	}

	auto c = lua_tonumber(ls, 5).toInt();
	pico_api::line(x0, y0, x1, y1, c);
	return 0;
}

static int impl_fillp(lua_State* ls) {
	DEBUG_DUMP_FUNCTION
	if (lua_gettop(ls) == 0) {
		pico_api::fillp();
	} else {
		auto n = lua_tonumber(ls, 1);
		auto bits = n.bits();
		int pattern = (bits >> 16) & 0xffff;
		bool transparent = (bits & 0xffff) != 0;
		pico_api::fillp(pattern, transparent);
	}
	return 0;
}

static int impl_time(lua_State* ls) {
	DEBUG_DUMP_FUNCTION
	uint64_t t = TIME_GetTime_ms();
	t = (t << 16) / 1000;
	lua_pushnumber(ls, z8::fix32::frombits((uint32_t)t));
	return 1;
}

static int impl_color(lua_State* ls) {
	DEBUG_DUMP_FUNCTION
	auto c = luaL_optnumber(ls, 1, 0).toInt();
	pico_api::color(c);
	return 0;
}

static int impl_camera(lua_State* ls) {
	DEBUG_DUMP_FUNCTION
	if (lua_gettop(ls) == 0) {
		pico_api::camera();
	} else {
		auto x = lua_tonumber(ls, 1).toInt();
		auto y = lua_tonumber(ls, 2).toInt();
		pico_api::camera(x, y);
	}
	return 0;
}

static int impl_stat(lua_State* ls) {
	DEBUG_DUMP_FUNCTION
	auto k = luaL_checknumber(ls, 1).toInt();

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
	auto n = lua_tonumber(ls, 1).toInt();
	auto fadems = lua_tonumber(ls, 2).toInt();
	auto channelmask = lua_tonumber(ls, 3).toInt();

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
	auto n = lua_tonumber(ls, 1).toInt();
	auto channel = lua_tonumber(ls, 2).toInt();
	auto offset = lua_tonumber(ls, 3).toInt();
	auto length = lua_tonumber(ls, 4).toInt();

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
	auto src_a = lua_tonumber(ls, 1).toInt();
	auto dest_a = lua_tonumber(ls, 2).toInt();
	auto len = lua_tonumber(ls, 3).toInt();
	pico_api::memory_cpy(src_a, dest_a, len);
	return 0;
}

static int impl_memset(lua_State* ls) {
	DEBUG_DUMP_FUNCTION
	auto a = lua_tonumber(ls, 1).toInt();
	auto val = lua_tonumber(ls, 2).toInt();
	auto len = lua_tonumber(ls, 3).toInt();
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

// returns nil if sound could not be loaded.
static int implx_wavload(lua_State* ls) {
	DEBUG_DUMP_FUNCTION
	auto name = luaL_checkstring(ls, 1);
	int id = pico_apix::wavload(name);
	if (id < 0)
		lua_pushnil(ls);
	else
		lua_pushnumber(ls, id);

	return 1;
}

static int implx_wavplay(lua_State* ls) {
	DEBUG_DUMP_FUNCTION
	auto id = luaL_checknumber(ls, 1).toInt();
	auto chan = luaL_checknumber(ls, 2).toInt();
	if (lua_gettop(ls) == 3) {
		bool loop = lua_toboolean(ls, 3);
		AUDIO_Play(id, chan, loop);
		return 0;
	}
	auto loop_start = luaL_checknumber(ls, 3).toInt();
	auto loop_end = luaL_checknumber(ls, 4).toInt();
	AUDIO_Play(id, chan, loop_start, loop_end);
	return 0;
}

static int implx_wavstop(lua_State* ls) {
	DEBUG_DUMP_FUNCTION
	auto chan = luaL_checknumber(ls, 1).toInt();
	AUDIO_Stop(chan);
	return 0;
}

static int implx_wavstoploop(lua_State* ls) {
	DEBUG_DUMP_FUNCTION
	auto chan = luaL_checknumber(ls, 1).toInt();
	AUDIO_StopLoop(chan);
	return 0;
}

static int implx_wavplaying(lua_State* ls) {
	DEBUG_DUMP_FUNCTION
	auto chan = luaL_checknumber(ls, 1).toInt();
	bool b = AUDIO_isPlaying(chan);
	lua_pushboolean(ls, b);
	return 1;
}

static int implx_setpal(lua_State* ls) {
	DEBUG_DUMP_FUNCTION
	auto i = luaL_checknumber(ls, 1).toInt();
	auto r = luaL_checknumber(ls, 2).toInt();
	auto g = luaL_checknumber(ls, 3).toInt();
	auto b = luaL_checknumber(ls, 4).toInt();
	pico_apix::setpal(i, r, g, b);
	return 0;
}

static int implx_selpal(lua_State* ls) {
	DEBUG_DUMP_FUNCTION
	auto name = luaL_checkstring(ls, 1);
	pico_apix::selpal(name);
	return 0;
}

static int implx_resetpal(lua_State* ls) {
	DEBUG_DUMP_FUNCTION
	if (lua_gettop(ls) == 1) {
		auto i = luaL_checknumber(ls, 1).toInt();
		pico_apix::resetpal(i);
	} else {
		pico_apix::resetpal();
	}

	return 0;
}

static int implx_screen(lua_State* ls) {
	DEBUG_DUMP_FUNCTION
	auto w = luaL_checknumber(ls, 1).toInt();
	auto h = luaL_checknumber(ls, 2).toInt();
	pico_apix::screen(w, h);
	return 0;
}

static int implx_xpal(lua_State* ls) {
	DEBUG_DUMP_FUNCTION
	auto enable = lua_toboolean(ls, 1);
	pico_apix::xpal(enable);
	return 0;
}

static int implx_cursor(lua_State* ls) {
	DEBUG_DUMP_FUNCTION
	auto enable = lua_toboolean(ls, 1);
	pico_apix::cursor(enable);
	return 0;
}

static int implx_showmenu(lua_State* ls) {
	DEBUG_DUMP_FUNCTION
	pico_apix::menu();
	return 0;
}

static int implx_touchmask(lua_State* ls) {
	DEBUG_DUMP_FUNCTION
	uint8_t m = INP_GetTouchMask();
	lua_pushnumber(ls, m);
	return 1;
}

static int implx_touchavail(lua_State* ls) {
	DEBUG_DUMP_FUNCTION
	bool avail = INP_TouchAvailable();
	lua_pushboolean(ls, avail);
	return 1;
}

static int implx_touchstate(lua_State* ls) {
	DEBUG_DUMP_FUNCTION
	auto idx = luaL_checknumber(ls, 1).toInt();
	TouchInfo ti = INP_GetTouchInfo(idx);
	lua_pushnumber(ls, ti.x);
	lua_pushnumber(ls, ti.y);
	lua_pushnumber(ls, ti.state);
	return 3;
}

static int implx_siminput(lua_State* ls) {
	DEBUG_DUMP_FUNCTION
	auto state = luaL_checknumber(ls, 1).toInt();
	pico_apix::siminput((uint8_t)state);
	return 0;
}

static int implx_sprites(lua_State* ls) {
	DEBUG_DUMP_FUNCTION
	if (lua_gettop(ls) == 0) {
		pico_apix::sprites();
	}
	auto page = luaL_checknumber(ls, 1).toInt();
	pico_apix::sprites(page);
	return 0;
}

static int implx_maps(lua_State* ls) {
	DEBUG_DUMP_FUNCTION
	if (lua_gettop(ls) == 0) {
		pico_apix::maps();
	}
	auto page = luaL_checknumber(ls, 1).toInt();
	pico_apix::maps(page);
	return 0;
}

static int implx_fonts(lua_State* ls) {
	DEBUG_DUMP_FUNCTION
	if (lua_gettop(ls) == 0) {
		pico_apix::fonts();
	}
	auto page = luaL_checknumber(ls, 1).toInt();
	pico_apix::fonts(page);
	return 0;
}

static int implx_open_url(lua_State* ls) {
	DEBUG_DUMP_FUNCTION
	auto s = luaL_checkstring(ls, 1);
	if (s) {
		PLATFORM_OpenURL(s);
	}
	return 0;
}

static int implx_tron(lua_State* ls) {
	pico_script::tron();
	DEBUG_DUMP_FUNCTION
	return 0;
}

static int implx_troff(lua_State* ls) {
	DEBUG_DUMP_FUNCTION
	pico_script::troff();
	return 0;
}

static int implx_fullscreen(lua_State* ls) {
	DEBUG_DUMP_FUNCTION
	auto enable = lua_toboolean(ls, 1);
	pico_apix::fullscreen(enable);
	return 0;
}

static int implx_window(lua_State* ls) {
	DEBUG_DUMP_FUNCTION
	return 0;
}

static int implx_assetload(lua_State* ls) {
	DEBUG_DUMP_FUNCTION
	auto s = luaL_checkstring(ls, 1);
	if (s) {
		pico_apix::assetload(s);
	}
	return 0;
}

static int implx_gfxstate(lua_State* ls) {
	DEBUG_DUMP_FUNCTION
	int index = lua_tonumber(ls, 1).toInt();
	pico_apix::gfxstate(index);
	return 0;
}

// dbg_getsrc (source, line)
static int implx_dbg_getsrc(lua_State* ls) {
	DEBUG_DUMP_FUNCTION
	auto src = luaL_checkstring(ls, 1);
	auto line = luaL_checknumber(ls, 2).toInt();

	auto l = pico_apix::dbg_getsrc(src, line);
	if (l.second) {
		lua_pushstring(ls, l.first.c_str());
		return 1;
	}
	return 0;
}

static std::set<int> debug_breakpoints;
static bool debug_singlestep = false;
static int break_line_number = -1;

static void dbg_hookfunc(lua_State* ls, lua_Debug* ar) {
	//	logr << "dbg_hookfunc " << ar->currentline << ":"
	//<< pico_apix::dbg_getsrc("main", ar->currentline).first;

	break_line_number = -1;

	lua_Debug info;
	lua_getstack(ls, 0, &info);
	lua_getinfo(ls, "S", &info);

	if (info.source && strcmp(info.source, "main") != 0) {
		return;
	}

	if (debug_breakpoints.count(ar->currentline) || debug_singlestep) {
		debug_singlestep = false;
		break_line_number = ar->currentline;
		luaL_dostring(ls, "__tac08__.dbg.locals = __tac08__.dbg.dumplocals(3)");
		lua_yield(ls, 0);
	}
}

// dbg_cocreate (thread_func) -> coroutine
static int implx_dbg_cocreate(lua_State* ls) {
	DEBUG_DUMP_FUNCTION
	luaL_checktype(ls, 1, LUA_TFUNCTION);
	lua_State* co = lua_newthread(ls);
	lua_sethook(co, dbg_hookfunc, LUA_MASKLINE, 0);
	lua_pushvalue(ls, 1); /* move function to top */
	lua_xmove(ls, co, 1); /* move function from L to NL */

	return 1;
}

void dumpstack(lua_State* ls, const char* name) {
	TraceFunction();
	logr << "dumpstack: " << name << " items: " << lua_gettop(ls);
	for (int n = 1; n <= lua_gettop(ls); n++) {
		logr << lua_typename(ls, lua_type(ls, n)) << ": " << lua_tostring(ls, n);
	}
}

// dbg_coresume (thread, mode) -> status_str, (error_str|line_number)
// mode = "run" - runs till breakpoint hit
// 		  "step" - executes single line of code.
// status_str = "break" - code hit breakpoint/line stop/function stop
// 				"done" - code completed execution normally
//				"error" - code generated an error.
static int implx_dbg_coresume(lua_State* ls) {
	DEBUG_DUMP_FUNCTION
	luaL_checktype(ls, -2, LUA_TTHREAD);
	lua_State* co = lua_tothread(ls, -2);
	std::string mode = lua_tostring(ls, -1);

	debug_singlestep = (mode == "step");

	int status = lua_status(co);
	if (status == LUA_OK || status == LUA_YIELD) {
		status = lua_resume(co, 0, 0);
	}

	switch (status) {
		case LUA_OK:
			lua_pushstring(ls, "done");
			return 1;

		case LUA_YIELD:
			lua_pushstring(ls, "break");
			lua_pushnumber(ls, break_line_number);
			return 2;

		default:
			lua_pushstring(ls, "error");
			lua_pushstring(ls, lua_tostring(co, -1));

			lua_Debug info;
			lua_getstack(co, 0, &info);
			lua_getinfo(co, "l", &info);

			lua_pushnumber(ls, info.currentline);
			luaL_dostring(co, "__tac08__.dbg.locals = __tac08__.dbg.dumplocals(3)");
			return 3;
	}

	return 0;
}

// dbg_bpline (line, enabled)
static int implx_dbg_bpline(lua_State* ls) {
	DEBUG_DUMP_FUNCTION
	int line = luaL_checknumber(ls, 1).toInt();
	bool enabled = lua_toboolean(ls, 2);

	if (enabled) {
		debug_breakpoints.insert(line);
	} else {
		debug_breakpoints.erase(line);
	}
	return 0;
}

static int implx_getkey(lua_State* ls) {
	DEBUG_DUMP_FUNCTION
	auto s = pico_apix::getkey();
	if (s.length()) {
		lua_pushstring(ls, s.c_str());
		return 1;
	}
	return 0;
}

// ------------------------------------------------------------------

static void register_cfuncs() {
	register_cfunc("load", impl_load);
	register_cfunc("run", impl_run);
	register_cfunc("cartdata", impl_cartdata);
	register_cfunc("cls", impl_cls);
	register_cfunc("poke", impl_poke);
	register_cfunc("peek", impl_peek);
	register_cfunc("poke2", impl_poke2);
	register_cfunc("peek2", impl_peek2);
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
	register_ext_cfunc("wavload", implx_wavload);
	register_ext_cfunc("wavplay", implx_wavplay);
	register_ext_cfunc("wavstop", implx_wavstop);
	register_ext_cfunc("wavstoploop", implx_wavstoploop);
	register_ext_cfunc("wavplaying", implx_wavplaying);
	register_ext_cfunc("setpal", implx_setpal);
	register_ext_cfunc("selpal", implx_selpal);
	register_ext_cfunc("resetpal", implx_resetpal);
	register_ext_cfunc("screen", implx_screen);
	register_ext_cfunc("xpal", implx_xpal);
	register_ext_cfunc("cursor", implx_cursor);
	register_ext_cfunc("showmenu", implx_showmenu);
	register_ext_cfunc("touchmask", implx_touchmask);
	register_ext_cfunc("touchstate", implx_touchstate);
	register_ext_cfunc("touchavail", implx_touchavail);
	register_ext_cfunc("siminput", implx_siminput);
	register_ext_cfunc("sprites", implx_sprites);
	register_ext_cfunc("maps", implx_maps);
	register_ext_cfunc("fonts", implx_fonts);
	register_ext_cfunc("open_url", implx_open_url);
	register_ext_cfunc("tron", implx_tron);
	register_ext_cfunc("troff", implx_troff);
	register_ext_cfunc("fullscreen", implx_fullscreen);
	register_ext_cfunc("window", implx_window);
	register_ext_cfunc("assetload", implx_assetload);
	register_ext_cfunc("gfxstate", implx_gfxstate);
	register_ext_cfunc("dbg_getsrc", implx_dbg_getsrc);
	register_ext_cfunc("dbg_cocreate", implx_dbg_cocreate);
	register_ext_cfunc("dbg_coresume", implx_dbg_coresume);
	register_ext_cfunc("dbg_bpline", implx_dbg_bpline);
	register_ext_cfunc("getkey", implx_getkey);
}

namespace pico_script {

	void load(const pico_cart::Cart& cart) {
		unload_scripting();
		init_scripting();

		std::string code;

		for (size_t i = 0; i < cart.source.size(); i++) {
			code += pico_cart::convert_emojis(cart.source[i].line) + "\n";
		}
		throw_error(luaL_loadbuffer(lstate, code.c_str(), code.size(), "main"));
		throw_error(lua_pcall(lstate, 0, 0, 0));
	}

	void unload_scripting() {
		if (lstate) {
			lua_close(lstate);
			lstate = nullptr;
		}
		deferredAPICalls.clear();
	}

	bool simpleCall(std::string function, bool optional) {
		lua_getglobal(lstate, function.c_str());

		if (!lua_isfunction(lstate, -1)) {
			if (optional) {
				lua_pop(lstate, 1);
				return false;
			} else
				throw pico_script::error(function + " not found");
		}
		throw_error(lua_pcall(lstate, 0, 0, 0));
		return true;
	}

	bool run(std::string function, bool optional, bool& restarted) {
		if (restarted) {
			return true;
		}

		auto ret = simpleCall(function, optional);

		while (!deferredAPICalls.empty()) {
			deferredAPICall_t apicall = deferredAPICalls.front();
			deferredAPICalls.pop_front();
			apicall();
			restarted = true;
		}
		return ret;
	}

	// returns true when menu finished
	bool do_menu() {
		lua_getglobal(lstate, "__tac08__");
		lua_getfield(lstate, -1, "do_menu");
		lua_remove(lstate, -2);
		throw_error(lua_pcall(lstate, 0, 1, 0));
		bool res = lua_toboolean(lstate, -1);
		lua_pop(lstate, 1);

		return res;
	}

	void tron() {
		traceAPI = true;
	}

	void troff() {
		traceAPI = false;
	}

}  // namespace pico_script
