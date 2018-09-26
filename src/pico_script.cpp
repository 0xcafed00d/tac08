#include "pico_script.h"

#include "z8lua/lauxlib.h"
#include "z8lua/lua.h"
#include "z8lua/lualib.h"

static lua_State* lstate = nullptr;

static void init_scripting() {
	if (lstate) {
		lua_close(lstate);
	}

	lstate = luaL_newstate();
}

static void throw_error(int err) {
	if (err) {
		pico_script::error e(lua_tostring(lstate, -1));
		lua_pop(lstate, 1);
		throw e;
	}
}

namespace pico_script {
	void load(std::string script) {
		init_scripting();
		throw_error(luaL_loadbuffer(lstate, script.c_str(), script.size(), "main"));
		throw_error(lua_pcall(lstate, 0, 0, 0));
	}

	void reset() {
	}

	void run(std::string function) {
		lua_getglobal(lstate, function.c_str());
		if (!lua_isfunction(lstate, -1)) {
			throw pico_script::error(function + " not found");
		}
		throw_error(lua_pcall(lstate, 0, 0, 0));
	}

}  // namespace pico_script
