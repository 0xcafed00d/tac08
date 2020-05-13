idf_component_register(SRCS SDL_audio.c SDL_event.c SDL_input.c SDL_system.c SDL_video.c spi_lcd.c
				       INCLUDE_DIRS "."
					   REQUIRES fatfs)

target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-deprecated-declarations -Wno-incompatible-pointer-types -Wno-unused-variable -Wno-return-local-addr)