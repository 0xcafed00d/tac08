# Edit following two lines to set component requirements (see docs)


idf_component_register(SRCS "main.cpp" "hal_core.cpp" "hal_fs.cpp" "hal_audio.cpp" "hal_palette.cpp" "log.cpp" "pico_audio.cpp" "pico_cart.cpp" "pico_core.cpp" "pico_data.cpp" "pico_gfx.cpp" "pico_memory.cpp" "pico_script.cpp" "utils.cpp" "utf8-util/utf8-util/utf8-util.cpp" 
                       INCLUDE_DIRS "."
                       REQUIRES sdl z8lua)

# Need to include the utf8-util submodule.
include_directories("utf8-util/utf8-util")

target_compile_options(${COMPONENT_LIB} PRIVATE -fexceptions -Wno-unused-variable -Wno-unused-function)
