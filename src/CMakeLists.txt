# Edit following two lines to set component requirements (see docs)


idf_component_register(SRCS "main.cpp" "hal_core.cpp"
                       INCLUDE_DIRS "."
                       REQUIRES SDL)

target_compile_options(${COMPONENT_LIB} PRIVATE -fexceptions)
