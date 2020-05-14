//
//  ZEPTO-8 — Fantasy console emulator
//
//  Copyright © 2016—2017 Sam Hocevar <sam@hocevar.net>
//
//  This program is free software. It comes without any warranty, to
//  the extent permitted by applicable law. You can redistribute it
//  and/or modify it under the terms of the Do What the Fuck You Want
//  to Public License, Version 2, as published by the WTFPL Task Force.
//  See http://www.wtfpl.net/ for more details.
//


#include <cmath>
#include <cctype>
#include <cstring>
#include <algorithm>

#define lpico8lib_c
#define LUA_LIB

#include "lua.h"

#include "lauxlib.h"
#include "llimits.h"
#include "lobject.h"
#include "lstate.h"

#define TAU 6.2831853071795864769252867665590057683936

static int pico8_max(lua_State *l) {
    lua_pushnumber(l, lua_Number::max(lua_tonumber(l, 1), lua_tonumber(l, 2)));
    return 1;
}

static int pico8_min(lua_State *l) {
    lua_pushnumber(l, lua_Number::min(lua_tonumber(l, 1), lua_tonumber(l, 2)));
    return 1;
}

static int pico8_mid(lua_State *l) {
    lua_Number x = lua_tonumber(l, 1);
    lua_Number y = lua_tonumber(l, 2);
    lua_Number z = lua_tonumber(l, 3);
    lua_pushnumber(l, x > y ? y > z ? y : lua_Number::min(x, z)
                            : x > z ? x : lua_Number::min(y, z));
    return 1;
}

static int pico8_ceil(lua_State *l) {
    lua_pushnumber(l, lua_Number::ceil(lua_tonumber(l, 1)));
    return 1;
}

static int pico8_flr(lua_State *l) {
    lua_pushnumber(l, lua_Number::floor(lua_tonumber(l, 1)));
    return 1;
}

static int pico8_cos(lua_State *l) {
    lua_pushnumber(l, cast_num(std::cos(-TAU * (double)lua_tonumber(l, 1))));
    return 1;
}

static int pico8_sin(lua_State *l) {
    lua_pushnumber(l, cast_num(std::sin(-TAU * (double)lua_tonumber(l, 1))));
    return 1;
}

static int pico8_atan2(lua_State *l) {
    lua_Number x = lua_tonumber(l, 1);
    lua_Number y = lua_tonumber(l, 2);
    // This could simply be atan2(-y,x) but since PICO-8 decided that
    // atan2(0,0) = 0.75 we need to do the same in our version.
    double a = 0.75 + std::atan2((double)x, (double)y) / TAU;
    lua_pushnumber(l, cast_num(a >= 1 ? a - 1 : a));
    return 1;
}

static int pico8_sqrt(lua_State *l) {
    lua_Number x = lua_tonumber(l, 1);
    /* FIXME PICO-8 actually returns stuff for negative values */
    lua_pushnumber(l, cast_num(x.bits() >= 0 ? std::sqrt((double)x) : 0));
    return 1;
}

static int pico8_abs(lua_State *l) {
    lua_pushnumber(l, lua_Number::abs(lua_tonumber(l, 1)));
    return 1;
}

static int pico8_sgn(lua_State *l) {
    lua_pushnumber(l, cast_num(lua_tonumber(l, 1).bits() >= 0 ? 1.0 : -1.0));
    return 1;
}

static int pico8_band(lua_State *l) {
    lua_pushnumber(l, lua_tonumber(l, 1) & lua_tonumber(l, 2));
    return 1;
}

static int pico8_bor(lua_State *l) {
    lua_pushnumber(l, lua_tonumber(l, 1) | lua_tonumber(l, 2));
    return 1;
}

static int pico8_bxor(lua_State *l) {
    lua_pushnumber(l, lua_tonumber(l, 1) ^ lua_tonumber(l, 2));
    return 1;
}

static int pico8_bnot(lua_State *l) {
    lua_pushnumber(l, ~lua_tonumber(l, 1));
    return 1;
}

static int pico8_shl(lua_State *l) {
    // If y is negative, it is interpreted modulo 32.
    // If y is >= 32, result is always zero.
    int32_t xbits = lua_tonumber(l, 1).bits();
    int y = (int)lua_tonumber(l, 2);
    lua_pushnumber(l, lua_Number::frombits(y >= 32 ? 0 : xbits << (y & 0x1f)));
    return 1;
}

static int pico8_lshr(lua_State *l) {
    int32_t xbits = lua_tonumber(l, 1).bits();
    int y = (int)lua_tonumber(l, 2);
    lua_pushnumber(l, lua_Number::frombits(y >= 32 ? 0 : (uint32_t)xbits >> (y & 0x1f)));
    return 1;
}

static int pico8_shr(lua_State *l) {
    // If y is negative, it is interpreted modulo 32.
    // If y is >= 32, only the sign is preserved, so it's
    // the same as for y == 31.
    int32_t xbits = lua_tonumber(l, 1).bits();
    int y = (int)lua_tonumber(l, 2);
    lua_pushnumber(l, lua_Number::frombits(xbits >> (std::min(y, 31) & 0x1f)));
    return 1;
}

static int pico8_rotl(lua_State *l) {
    int32_t xbits = lua_tonumber(l, 1).bits();
    int y = 0x1f & (int)lua_tonumber(l, 2);
    lua_pushnumber(l, lua_Number::frombits((xbits << y) | ((uint32_t)xbits >> (32 - y))));
    return 1;
}

static int pico8_rotr(lua_State *l) {
    int32_t xbits = lua_tonumber(l, 1).bits();
    int y = 0x1f & (int)lua_tonumber(l, 2);
    lua_pushnumber(l, lua_Number::frombits(((uint32_t)xbits >> y) | (xbits << (32 - y))));
    return 1;
}

static int pico8_tostr(lua_State *l) {
    char buffer[20];
    char const *s = buffer;
    switch (lua_type(l, 1))
    {
        case LUA_TNUMBER: {
            lua_Number x = lua_tonumber(l, 1);
            if (lua_toboolean(l, 2)) {
                uint32_t b = (uint32_t)x.bits();
                sprintf(buffer, "0x%04x.%04x", (b >> 16) & 0xffff, b & 0xffff);
            } else {
                lua_number2str(buffer, x);
            }
            break;
        }
        case LUA_TSTRING: s = lua_tostring(l, 1); break;
        case LUA_TBOOLEAN: s = lua_toboolean(l, 1) ? "true" : "false"; break;
        default: sprintf(buffer, "[%s]", luaL_typename(l, 1)); break;
    }
    lua_pushstring(l, s);
    return 1;
}

static int pico8_tonum(lua_State *l) {
    char const *s = lua_tostring(l, 1);
    lua_Number ret;
    // If parsing failed, PICO-8 returns nothing
    if (!luaO_str2d(s, strlen(s), &ret)) return 0;
    lua_pushnumber(l, ret);
    return 1;
}

static void update_prng(global_State *g) {
    g->prngseed2 = g->prngseed1 + ((g->prngseed2 >> 16) | (g->prngseed2 << 16));
    g->prngseed1 += g->prngseed2;
}

static int pico8_srand(lua_State *L) {
    uint32_t seed = lua_tonumber(L, 1).bits();
    G(L)->prngseed1 = seed ? seed : 0xdeadbeef;
    G(L)->prngseed2 = G(L)->prngseed1 ^ 0xbead29ba;
    for (int i = 0; i < 32; ++i)
        update_prng(G(L));
    return 0;
}

static int pico8_rnd(lua_State *L) {
    uint32_t range = lua_isnone(L, 1) ? 0x10000 : lua_tonumber(L, 1).bits();
    update_prng(G(L));
    lua_pushnumber(L, lua_Number::frombits(range ? G(L)->prngseed2 % range : 0));
    return 1;
}

static const luaL_Reg pico8lib[] = {
  {"max",   pico8_max},
  {"min",   pico8_min},
  {"mid",   pico8_mid},
  {"ceil",  pico8_ceil},
  {"flr",   pico8_flr},
  {"cos",   pico8_cos},
  {"sin",   pico8_sin},
  {"atan2", pico8_atan2},
  {"sqrt",  pico8_sqrt},
  {"abs",   pico8_abs},
  {"sgn",   pico8_sgn},
  {"band",  pico8_band},
  {"bor",   pico8_bor},
  {"bxor",  pico8_bxor},
  {"bnot",  pico8_bnot},
  {"shl",   pico8_shl},
  {"shr",   pico8_shr},
  {"lshr",  pico8_lshr},
  {"rotl",  pico8_rotl},
  {"rotr",  pico8_rotr},
  {"tostr", pico8_tostr},
  {"tonum", pico8_tonum},
  {"srand", pico8_srand},
  {"rnd",   pico8_rnd},
  {NULL, NULL}
};


/*
** Register PICO-8 functions in global table
*/
LUAMOD_API int luaopen_pico8 (lua_State *L) {
  lua_pushglobaltable(L);
  luaL_setfuncs(L, pico8lib, 0);
  return 1;
}

