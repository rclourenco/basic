basic: basic.c basic.h basrpn.c basexec.c
	gcc -o basic basic.c basrpn.c basexec.c

basicd: basic.c basic.h basrpn.c basexec.c graphics.h graphics.c basedit.c
	tcc basic.c basrpn.c basexec.c graphics.c basedit.c

# SDL2 port of graphics.c (same API as graphics.h)
SDL_CFLAGS = `sdl2-config --cflags`
SDL_LIBS   = `sdl2-config --libs`

graphics_sdl.o: graphics_sdl.c graphics.h fontc64.c
	gcc -DGRAPHICS_SDL -c graphics_sdl.c $(SDL_CFLAGS)
