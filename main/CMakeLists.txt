# Edit following two lines to set component requirements (see docs)

# TODO: Add other files to the next line:
#       set(COMPONENT_SRCS "main.cpp hal_core.cpp ...")
# set(COMPONENT_SRCS "main.cpp hal_core.cpp hal_fs.cpp hal_palette.cpp hal_audio.cpp pico_core.cpp pico_gfx.cpp pico_audio.cpp pico_memory.cpp pico_data.cpp pico_script.cpp pico_cart.cpp utf8-util/utf8-util/utf8-util.cpp utils.cpp log.cpp crypt.cpp")

# set(COMPONENT_SRCS "app_main.cpp")

# # set(COMPONENT_ADD_INCLUDEDIRS "utf8-util/utf8-util")



# register_component()

# Add compile flags AFTER component registration.

# add_definitions(-DSDL_DISABLE_IMMINTRIN_H)

# target_compile_options(${COMPONENT_LIB} PRIVATE -fexceptions -Lz8lua -lluaz8)


idf_component_register(SRCS "app_main.cpp"
                       # INCLUDE_DIRS "include"
                       REQUIRES tac08)
