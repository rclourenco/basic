build: basicsdl

basic: basic.c basic.h basrpn.c basexec.c
	gcc -o basic basic.c basrpn.c basexec.c

basicd: basic.c basic.h basrpn.c basexec.c graphics.h graphics.c basedit.c
	tcc basic.c basrpn.c basexec.c graphics.c basedit.c

# SDL2 port of graphics.c (same API as graphics.h)
SDL_CFLAGS = `sdl2-config --cflags`
SDL_LIBS   = `sdl2-config --libs`

# uses $(CC) (cc by default): gcc or clang on Linux, Apple clang on macOS.
# On macOS install SDL2 first, e.g. with Homebrew: brew install sdl2
graphics_sdl.o: graphics_sdl.c graphics.h fontc64.c
	$(CC) -DGRAPHICS_SDL -c graphics_sdl.c $(SDL_CFLAGS)

# full interpreter + editor using the SDL2 graphics module
basicsdl: basic.c basic.h basrpn.c basexec.c graphics.h graphics_sdl.c fontc64.c basedit.c
	$(CC) -DGRAPHICS_SDL -o basicsdl basic.c basrpn.c basexec.c graphics_sdl.c basedit.c $(SDL_CFLAGS) $(SDL_LIBS) -lm
