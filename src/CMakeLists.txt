# Edit following two lines to set component requirements (see docs)

set(COMPONENT_REQUIRES )
set(COMPONENT_PRIV_REQUIRES )

# TODO: Add other files to the next line:
#       set(COMPONENT_SRCS "main.cpp hal_core.cpp ...")
set(COMPONENT_SRCS "main.cpp")
set(COMPONENT_ADD_INCLUDEDIRS "")

register_component()

# Add compile flags AFTER component registration.

target_compile_options(${COMPONENT_LIB} PRIVATE -fexceptions)
