idf_component_register(SRCS "lapi.cpp" "lgc.cpp" "lstate.cpp" "lauxlib.cpp" "linit.cpp" "lstring.cpp" "lbaselib.cpp" "liolib.cpp" "lstrlib.cpp" "lbitlib.cpp" "llex.cpp" "ltable.cpp" "lcode.cpp" "lmathlib.cpp" "ltablib.cpp" "lcorolib.cpp" "lmem.cpp" "ltests.cpp" "lctype.cpp" "loadlib.cpp" "ltm.cpp" "ldblib.cpp" "lobject.cpp" "lua.cpp" "ldebug.cpp" "lopcodes.cpp" "lundump.cpp" "ldo.cpp" "loslib.cpp" "lvm.cpp" "ldump.cpp" "lparser.cpp" "lzio.cpp" "lfunc.cpp" "lpico8lib.cpp"
				       INCLUDE_DIRS "."
					   REQUIRES)

target_compile_options(${COMPONENT_LIB} PRIVATE -fexceptions -pedantic -Wcast-align -Wpointer-arith -Wshadow
        -Wsign-compare -Wundef -Wwrite-strings -Wno-implicit-fallthrough -Wno-cast-align -Wno-format -Wno-unused-function -std=c++11)
