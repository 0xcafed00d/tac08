# Edit following two lines to set component requirements (see docs)


idf_component_register(SRCS "main.cpp" "hal_core.cpp" "hal_fs.cpp" "hal_audio.cpp" "hal_palette.cpp" "log.cpp" "pico_audio.cpp" "pico_cart.cpp" "pico_core.cpp" "pico_data.cpp" "pico_gfx.cpp" "pico_memory.cpp" "pico_script.cpp" "utils.cpp" "utf8-util/utf8-util/utf8-util.cpp" 
                       INCLUDE_DIRS "."
                       REQUIRES SDL)


# Lua libs:
# "z8lua/lapi.c" "z8lua/lgc.c" "z8lua/lstate.c" "z8lua/lauxlib.c" "z8lua/linit.c" "z8lua/lstring.c" "z8lua/lbaselib.c" "z8lua/liolib.c" "z8lua/lstrlib.c" "z8lua/lbitlib.c" "z8lua/llex.c" "z8lua/ltable.c" "z8lua/lcode.c" "z8lua/lmathlib.c" "z8lua/ltablib.c" "z8lua/lcorolib.c" "z8lua/lmem.c" "z8lua/ltests.c" "z8lua/lctype.c" "z8lua/loadlib.c" "z8lua/ltm.c" "z8lua/ldblib.c" "z8lua/lobject.c" "z8lua/lua.c" "z8lua/ldebug.c" "z8lua/lopcodes.c" "z8lua/lundump.c" "z8lua/ldo.c" "z8lua/loslib.c" "z8lua/lvm.c" "z8lua/ldump.c" "z8lua/lparser.c" "z8lua/lzio.c" "z8lua/lfunc.c" "z8lua/lpico8lib.c"

# Need to include the utf8-util submodule.
include_directories("utf8-util/utf8-util")

add_library(lua STATIC IMPORTED GLOBAL)
set_target_properties(lua PROPERTIES IMPORTED_LOCATION ../src/z8lua/libluaz8.a)
target_link_libraries(${COMPONENT_LIB} PUBLIC lua)

# This doesn't work either:
# add_prebuilt_library(lua /Users/aric/src/tac08/src/z8lua/libluaz8.a)
# target_link_libraries(${COMPONENT_LIB} PRIVATE lua)

target_compile_options(${COMPONENT_LIB} PRIVATE -fexceptions -Wno-unused-variable -Wno-unused-function)
