/*
	Biblioteca programada por Renato Lourenco.

	SDL2 port of graphics.c (VGA mode 13h emulation).

	The public API is the same as graphics.c / graphics.h: a 320x200
	256-colour linear frame buffer, the same drawing, picture, virtual
	screen and text-terminal (tt_*) routines.  Instead of writing to
	0xA000:0000 everything is drawn into an in-memory buffer that is
	converted through a VGA-style palette and shown in an SDL window.

	Because the DOS build relies on conio.h, this module also provides
	kbhit(), getch() and delay() with DOS semantics (extended keys return
	0 followed by the BIOS scan code, e.g. 0x48 for "up").

	Build with -DGRAPHICS_SDL and link with SDL2, for example:
		gcc -DGRAPHICS_SDL -c graphics_sdl.c `sdl2-config --cflags`
		gcc ... graphics_sdl.o `sdl2-config --libs`
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <SDL.h>

#ifndef GRAPHICS_SDL
	#define GRAPHICS_SDL
#endif
#include "graphics.h"

#include "fontc64.c"

#define SCR_W 320
#define SCR_H 200
#define SCR_SIZE (SCR_W*SCR_H)

/* width of the emulated overscan (border) area, in VGA pixels */
#define BORDER 8
/* initial window scale factor */
#define WIN_SCALE 3
/* minimum time between automatic screen refreshes, in ms (~60 fps) */
#define REFRESH_MS 16

/*
	The macros in graphics.h write without any bounds checking, which
	on DOS just scribbles over video memory but here would corrupt the
	heap.  Redefine them to clip.
*/
#undef PLOT
#undef GETPLOT
#define INSIDE(X,Y) ((unsigned)(X)<SCR_W && (unsigned)(Y)<SCR_H)
#define PLOT(X,Y,C) do { int _x=(X), _y=(Y); \
	if (INSIDE(_x,_y)) screen[_x+_y*SCR_W]=(C); } while(0)
#define GETPLOT(X,Y) (INSIDE((X),(Y)) ? screen[(X)+(Y)*SCR_W] : 0)

unsigned char DRAW=COPY_D;
unsigned char Color=15;
unsigned char FillColor=15;
unsigned char TextColor=7;
unsigned char TextBackGround=255;

static unsigned char vram[SCR_SIZE];
unsigned char *dscreen=vram;
unsigned char *screen=vram;

char ScrnT=DEFAULT;
unsigned char *cart=console_font_8x8;

/* ------------------------------------------------------------------ */
/* SDL state                                                          */
/* ------------------------------------------------------------------ */

static SDL_Window   *gfx_window   = NULL;
static SDL_Renderer *gfx_renderer = NULL;
static SDL_Texture  *gfx_texture  = NULL;
static Uint32        gfx_last_present = 0;
static int           gfx_dirty = 1;

static unsigned char dac[256][3];   /* 6-bit VGA DAC values */
static Uint32        lut[256];      /* ARGB8888 lookup built from dac */
static unsigned char overscan = 0;

static Uint32 rgb6(unsigned char r, unsigned char g, unsigned char b)
{
	r&=63; g&=63; b&=63;
	return 0xFF000000u
		| ((Uint32)((r<<2)|(r>>4))<<16)
		| ((Uint32)((g<<2)|(g>>4))<<8)
		|  (Uint32)((b<<2)|(b>>4));
}

/* Rebuild the default mode 13h palette */
static void default_palette()
{
	static const unsigned char ega[16][3]={
		{0,0,0},{0,0,42},{0,42,0},{0,42,42},
		{42,0,0},{42,0,42},{42,21,0},{42,42,42},
		{21,21,21},{21,21,63},{21,63,21},{21,63,63},
		{63,21,21},{63,21,63},{63,63,21},{63,63,63}
	};
	static const unsigned char gray[16]={
		0,5,8,11,14,17,20,24,28,32,36,40,45,50,56,63
	};
	/* 3 intensities x 3 saturations, 5 levels (low..high) each */
	static const unsigned char lv[9][5]={
		{ 0,16,31,47,63},{31,39,47,55,63},{45,49,54,58,63},
		{ 0, 7,14,21,28},{14,17,21,24,28},{20,22,24,26,28},
		{ 0, 4, 8,12,16},{ 8,10,12,14,16},{11,12,13,15,16}
	};
	int i, g, h;

	for (i=0;i<16;i++) {
		dac[i][0]=ega[i][0]; dac[i][1]=ega[i][1]; dac[i][2]=ega[i][2];
		dac[16+i][0]=dac[16+i][1]=dac[16+i][2]=gray[i];
	}

	i=32;
	for (g=0; g<9; g++) {
		const unsigned char *l=lv[g];
		unsigned char lo=l[0], hi=l[4];
		for (h=0; h<24; h++, i++) {
			unsigned char r, gr, b;
			int s=h%4;
			switch (h/4) {
			case 0: r=l[s];   gr=lo; b=hi;     break; /* blue -> magenta */
			case 1: r=hi;     gr=lo; b=l[4-s]; break; /* magenta -> red  */
			case 2: r=hi;     gr=l[s]; b=lo;   break; /* red -> yellow   */
			case 3: r=l[4-s]; gr=hi; b=lo;     break; /* yellow -> green */
			case 4: r=lo;     gr=hi; b=l[s];   break; /* green -> cyan   */
			default:r=lo;     gr=l[4-s]; b=hi; break; /* cyan -> blue    */
			}
			dac[i][0]=r; dac[i][1]=gr; dac[i][2]=b;
		}
	}
	for (; i<256; i++)
		dac[i][0]=dac[i][1]=dac[i][2]=0;

	for (i=0;i<256;i++)
		lut[i]=rgb6(dac[i][0],dac[i][1],dac[i][2]);
	gfx_dirty=1;
}

static void gfx_init()
{
	static int palette_ready=0;

	if (!palette_ready) {
		default_palette();
		palette_ready=1;
	}
	if (gfx_window)
		return;

	if (!SDL_WasInit(SDL_INIT_VIDEO) && SDL_Init(SDL_INIT_VIDEO)!=0) {
		fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
		exit(1);
	}

	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");

	gfx_window = SDL_CreateWindow("BASIC",
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		(SCR_W+2*BORDER)*WIN_SCALE, (SCR_H+2*BORDER)*WIN_SCALE,
		SDL_WINDOW_RESIZABLE);
	if (!gfx_window) {
		fprintf(stderr, "SDL_CreateWindow: %s\n", SDL_GetError());
		exit(1);
	}

	gfx_renderer = SDL_CreateRenderer(gfx_window, -1,
		SDL_RENDERER_ACCELERATED|SDL_RENDERER_PRESENTVSYNC);
	if (!gfx_renderer)
		gfx_renderer = SDL_CreateRenderer(gfx_window, -1,
			SDL_RENDERER_SOFTWARE);
	if (!gfx_renderer) {
		fprintf(stderr, "SDL_CreateRenderer: %s\n", SDL_GetError());
		exit(1);
	}
	SDL_RenderSetLogicalSize(gfx_renderer, SCR_W+2*BORDER, SCR_H+2*BORDER);

	gfx_texture = SDL_CreateTexture(gfx_renderer, SDL_PIXELFORMAT_ARGB8888,
		SDL_TEXTUREACCESS_STREAMING, SCR_W, SCR_H);
	if (!gfx_texture) {
		fprintf(stderr, "SDL_CreateTexture: %s\n", SDL_GetError());
		exit(1);
	}

	SDL_StartTextInput();
	gfx_dirty=1;
}

void gfx_shutdown()
{
	if (gfx_texture)  SDL_DestroyTexture(gfx_texture);
	if (gfx_renderer) SDL_DestroyRenderer(gfx_renderer);
	if (gfx_window)   SDL_DestroyWindow(gfx_window);
	gfx_texture=NULL;
	gfx_renderer=NULL;
	gfx_window=NULL;
	if (SDL_WasInit(SDL_INIT_VIDEO))
		SDL_Quit();
}

void gfx_present()
{
	void *pixels;
	int pitch;
	SDL_Rect dst = { BORDER, BORDER, SCR_W, SCR_H };
	Uint32 b;

	gfx_init();

	if (SDL_LockTexture(gfx_texture, NULL, &pixels, &pitch)==0) {
		int x, y;
		const unsigned char *src=dscreen;
		for (y=0; y<SCR_H; y++) {
			Uint32 *row=(Uint32 *)((Uint8 *)pixels + y*pitch);
			for (x=0; x<SCR_W; x++)
				row[x]=lut[*src++];
		}
		SDL_UnlockTexture(gfx_texture);
	}

	b=lut[overscan];
	SDL_SetRenderDrawColor(gfx_renderer,
		(b>>16)&0xFF, (b>>8)&0xFF, b&0xFF, 0xFF);
	SDL_RenderClear(gfx_renderer);
	SDL_RenderCopy(gfx_renderer, gfx_texture, NULL, &dst);
	SDL_RenderPresent(gfx_renderer);

	gfx_last_present=SDL_GetTicks();
	gfx_dirty=0;
}

static void gfx_maybe_present()
{
	if (!gfx_window || SDL_GetTicks()-gfx_last_present >= REFRESH_MS)
		gfx_present();
}

/* ------------------------------------------------------------------ */
/* Keyboard (conio emulation)                                         */
/* ------------------------------------------------------------------ */

#define KQ_SIZE 256
static int kq[KQ_SIZE];
static int kq_head=0, kq_tail=0;

static void kq_put(int c)
{
	int n=(kq_head+1)%KQ_SIZE;
	if (n==kq_tail) return;   /* full: drop key */
	kq[kq_head]=c;
	kq_head=n;
}

static void kq_put_ext(int scan)
{
	kq_put(0);
	kq_put(scan);
}

static void handle_keydown(const SDL_KeyboardEvent *k)
{
	SDL_Keycode sym=k->keysym.sym;
	Uint16 mod=k->keysym.mod;

	switch (sym) {
	case SDLK_RETURN:
	case SDLK_KP_ENTER:  kq_put(13); return;
	case SDLK_BACKSPACE: kq_put(8);  return;
	case SDLK_ESCAPE:    kq_put(27); return;
	case SDLK_TAB:       kq_put(9);  return;
	case SDLK_UP:        kq_put_ext(0x48); return;
	case SDLK_DOWN:      kq_put_ext(0x50); return;
	case SDLK_LEFT:      kq_put_ext(0x4B); return;
	case SDLK_RIGHT:     kq_put_ext(0x4D); return;
	case SDLK_HOME:      kq_put_ext(0x47); return;
	case SDLK_END:       kq_put_ext(0x4F); return;
	case SDLK_PAGEUP:    kq_put_ext(0x49); return;
	case SDLK_PAGEDOWN:  kq_put_ext(0x51); return;
	case SDLK_INSERT:    kq_put_ext(0x52); return;
	case SDLK_DELETE:    kq_put_ext(0x53); return;
	default: break;
	}

	if (sym>=SDLK_F1 && sym<=SDLK_F10) {
		kq_put_ext(0x3B + (sym-SDLK_F1));
		return;
	}

	/* Ctrl+letter does not generate text input: map to ^A..^Z */
	if ((mod & KMOD_CTRL) && sym>='a' && sym<='z')
		kq_put(sym-'a'+1);
}

static void handle_text(const char *t)
{
	/* decode UTF-8, keep code points that fit in one byte (Latin-1) */
	const unsigned char *s=(const unsigned char *)t;
	while (*s) {
		unsigned cp;
		if (*s<0x80) {
			cp=*s++;
		} else if ((*s&0xE0)==0xC0 && (s[1]&0xC0)==0x80) {
			cp=((s[0]&0x1F)<<6)|(s[1]&0x3F);
			s+=2;
		} else {
			/* longer sequence: skip it */
			s++;
			while ((*s&0xC0)==0x80) s++;
			continue;
		}
		if (cp<256)
			kq_put((int)cp);
	}
}

static void pump_events()
{
	SDL_Event e;

	gfx_init();
	while (SDL_PollEvent(&e)) {
		switch (e.type) {
		case SDL_QUIT:
			gfx_shutdown();
			exit(0);
		case SDL_KEYDOWN:
			handle_keydown(&e.key);
			break;
		case SDL_TEXTINPUT:
			handle_text(e.text.text);
			break;
		case SDL_WINDOWEVENT:
			gfx_dirty=1;
			break;
		}
	}
	if (gfx_dirty)
		gfx_present();
	else
		gfx_maybe_present();
}

int kbhit()
{
	pump_events();
	return kq_head!=kq_tail;
}

int getch()
{
	int c;
	while (!kbhit())
		SDL_Delay(5);
	c=kq[kq_tail];
	kq_tail=(kq_tail+1)%KQ_SIZE;
	return c;
}

void delay(unsigned ms)
{
	Uint32 start;
	gfx_init();
	start=SDL_GetTicks();
	for (;;) {
		Uint32 el;
		pump_events();
		el=SDL_GetTicks()-start;
		if (el>=ms) break;
		SDL_Delay((ms-el)>5 ? 5 : (ms-el));
	}
}

/* ------------------------------------------------------------------ */
/* "BIOS" functions                                                   */
/* ------------------------------------------------------------------ */

void modo13h()
{
	gfx_init();
	memset(dscreen, 0, SCR_SIZE);
	default_palette();
	gfx_present();
}

void modo3h()
{
	gfx_shutdown();
}

unsigned char *getcpt()
{
	return console_font_8x8;
}

void SetOverScan(char c)
{
	overscan=(unsigned char)c;
	gfx_dirty=1;
}

void SetDacRegs(unsigned a,unsigned char r,unsigned char g,unsigned char b)
{
	if (a>255) return;
	dac[a][0]=r&63; dac[a][1]=g&63; dac[a][2]=b&63;
	lut[a]=rgb6(r,g,b);
	gfx_dirty=1;
}

void GetDacRegs(unsigned a,unsigned char *r,unsigned char *g,unsigned char *b)
{
	if (a>255) return;
	*r=dac[a][0]; *g=dac[a][1]; *b=dac[a][2];
}

/* ------------------------------------------------------------------ */
/* Drawing                                                            */
/* ------------------------------------------------------------------ */

void pchar(int x,int y, unsigned char c)
{
	int cx,cy;
	unsigned char v;
	if(!cart)
		cart=getcpt();
	for(cy=0;cy<8;cy++)
	{
		v=cart[8*c+cy];
		for(cx=0;cx<8;cx++)
		{
			if((v&128)==128)
				PLOT(x+cx,y+cy,TextColor);
			else
			if(TextBackGround!=255)
				PLOT(x+cx,y+cy,TextBackGround);
			v=v<<1;
		}
	}
}

void writest(int x,int y,unsigned char *st)
{
	while(*st)
	{
		pchar(x,y,*st);
		st++;
		x+=8;
	}
}

void fillscreen(char a)
{
	memset(screen,(unsigned char)a,SCR_SIZE);
}

void swap(int *a,int *b)
{
	int temp;
	temp=*a;
	*a=*b;
	*b=temp;
}

void DrawPixel(int x,int y,char c)
{
	if ((x>=0)&&(y>=0)&&(x<SCR_W)&&(y<SCR_H))
	{
		unsigned char uc=(unsigned char)c;
		switch (DRAW)
		{
			case XOR_D:PLOT(x,y,uc^GETPLOT(x,y));break;
			case AND_D:PLOT(x,y,uc&GETPLOT(x,y));break;
			case OR_D:PLOT(x,y,uc|GETPLOT(x,y));break;
			default:PLOT(x,y,uc);
		}
	}
}

void DrawRect(int x1,int y1,int x2,int y2,char op)
{
	int x,y;
	int cor;
	if(x1>x2) swap(&x1,&x2);
	if(y1>y2) swap(&y1,&y2);
	if(op!=1)
	{
		for(x=x1+1;x<x2;x++)
		for(y=y1+1;y<y2;y++)
			DrawPixel(x,y,FillColor);
	}
	if(op==2)
		cor=FillColor;
	else
		cor=Color;
	for(x=x1;x<=x2;x++)
	{
		DrawPixel(x,y1,cor);
		DrawPixel(x,y2,cor);
	}
	for(y=y1+1;y<y2;y++)
	{
		DrawPixel(x1,y,cor);
		DrawPixel(x2,y,cor);
	}
}

void dline(int x1, int y1, int x2, int y2)
{
	int dx,dy,d,y,x;
	int ax,ay;
	int sx,sy;

	dx = x2-x1;
	dy = y2-y1;

	sx = (x2<x1?-1:1);
	sy = (y2<y1?-1:1);
	ax = 2*(dx<0?-dx:dx);
	ay = 2*(dy<0?-dy:dy);

	x=x1;
	y=y1;
	if( ax >= ay ) {
		d=ay-ax/2;
		while(1)
		{
			PLOT(x,y,Color);
			if(x==x2) break;
			if(d>=0) {
				y=y+sy;
				d=d-ax;
			}
			d=d+ay;
			x=x+sx;
		}
	}
	else {
		d=ax-ay/2;
		while(1)
		{
			PLOT(x,y,Color);
			if(y==y2) break;
			if(d>=0) {
				x=x+sx;
				d=d-ay;
			}
			d=d+ax;
			y=y+sy;
		}
	}
}

void dcircle(int x0, int y0, int r)
{
	int x = r - 1;
	int y = 0;
	int dx = 1;
	int dy = 1;
	int err = dx - (r<<1);

	while(x>=y)
	{
		DrawPixel(x0+x,y0+y,Color);
		DrawPixel(x0+x,y0-y,Color);
		DrawPixel(x0-x,y0+y,Color);
		DrawPixel(x0-x,y0-y,Color);

		DrawPixel(x0+y,y0+x,Color);
		DrawPixel(x0+y,y0-x,Color);
		DrawPixel(x0-y,y0+x,Color);
		DrawPixel(x0-y,y0-x,Color);

		if(err<=0)
		{
			y++;
			err+=dy;
			dy+=2;
		}
		if(err>0)
		{
			x--;
			dx+=2;
			err += dx  - (r<<1);
		}
	}
}

/* ------------------------------------------------------------------ */
/* Pictures                                                           */
/* ------------------------------------------------------------------ */

unsigned PicSize(int x1,int y1,int x2,int y2)
{
	int x,y;
	if (x1>x2) swap(&x1,&x2);
	if (y1>y2) swap(&y1,&y2);
	x=x2-x1+1;
	y=y2-y1+1;
	if((x>SCR_W)&&(y>SCR_H))
		return 0;
	return x*y+sizeof(TPicture);
}

void GetPic(int x1,int y1,int x2,int y2,TPicture *pic)
{
	int x,y;
	int cy,cx;
	char *d;
	if (x1>x2) swap(&x1,&x2);
	if (y1>y2) swap(&y1,&y2);
	x=x2-x1+1;
	y=y2-y1+1;

	pic->w=x;
	pic->h=y;

	d=pic->data;
	for(cy=y1;cy<=y2;cy++)
	for(cx=x1;cx<=x2;cx++)
	{
		*d=GETPLOT(cx,cy);
		d++;
	}
}

int XPRes(TPicture *pic)
{
	return pic->w;
}

int YPRes(TPicture *pic)
{
	return pic->h;
}

/* fast opaque copy (clipped) */
void DrawPicF(int x1,int y1,TPicture *pic)
{
	char *data=pic->data;
	int w=pic->w;
	int h=pic->h;
	int y;

	for(y=0;y<h;y++,data+=w)
	{
		int sy=y1+y, sx=x1, off=0, n=w;
		if (sy<0 || sy>=SCR_H) continue;
		if (sx<0) { off=-sx; n-=off; sx=0; }
		if (sx+n>SCR_W) n=SCR_W-sx;
		if (n>0)
			memcpy(&screen[sx+sy*SCR_W],data+off,n);
	}
}

/* colour 0 is transparent */
void DrawPic(int x1,int y1,TPicture *pic)
{
	char *data=pic->data;
	int x2=x1+pic->w-1;
	int y2=y1+pic->h-1;
	int x,y;

	for(y=y1;y<=y2;y++)
	for(x=x1;x<=x2;x++)
	{
		if (*data!=0) PLOT(x,y,*data);
		data++;
	}
}

/* colour 255 is transparent, honours DRAW mode */
void DrawPicB(int x1,int y1,TPicture *pic)
{
	signed char *data=(signed char *)pic->data;
	int x2=x1+pic->w-1;
	int y2=y1+pic->h-1;
	int x,y;

	for(y=y1;y<=y2;y++)
	for(x=x1;x<=x2;x++)
	{
		if (*data!=-1) DrawPixel(x,y,*data);
		data++;
	}
}

/* ------------------------------------------------------------------ */
/* Virtual screens                                                    */
/* ------------------------------------------------------------------ */

void DefaultScreen()
{
	ScrnT=DEFAULT;
	screen=dscreen;
}

void UserScreen(unsigned char *s)
{
	ScrnT=USER;
	screen=s;
}

char ScrnCpyUD()
{
	if (ScrnT!=USER) return 0;
	memcpy(dscreen,screen,SCR_SIZE);
	gfx_dirty=1;
	return 1;
}

char ScrnCpyDU()
{
	if (ScrnT!=USER) return 0;
	memcpy(screen,dscreen,SCR_SIZE);
	return 1;
}

/* ------------------------------------------------------------------ */
/* Text terminal (40x25)                                              */
/* ------------------------------------------------------------------ */

TCursor Cursor = {0, 0};

void show_cursor()
{
	unsigned char bdraw = DRAW;
	unsigned char bfillcolor = FillColor;
	DRAW = XOR_D;
	FillColor = 8;
	DrawRect(Cursor.c*8,Cursor.r*8+1,Cursor.c*8+7,Cursor.r*8+7,FILL);
	DRAW = bdraw;
	FillColor = bfillcolor;
}

void hide_cursor()
{
	unsigned char bdraw = DRAW;
	unsigned char bfillcolor = FillColor;
	DRAW = XOR_D;
	FillColor = 8;
	DrawRect(Cursor.c*8,Cursor.r*8+1,Cursor.c*8+7,Cursor.r*8+7,FILL);
	DRAW = bdraw;
	FillColor = bfillcolor;
}

void tt_locate(int c, int r)
{
	if (c<0)  c=0;
	if (c>39) c=39;

	if (r<0)  r=0;
	if (r>24) r=24;
	Cursor.c = c;
	Cursor.r = r;
}

unsigned char screentext[1024];

int tt_getline(char *buffer, int max)
{
	int i=0;
	unsigned char *start = screentext+Cursor.r*40;
	if (Cursor.r>0 && screentext[(Cursor.r-1)*40+39]!=10)
		start = screentext+(Cursor.r-1)*40;

	if (max>80)
		max = 80;

	for( i=0; i<max; i++) {
		if (start[i]==10)
			break;
		buffer[i]=start[i];
	}

	while (i>0 && (buffer[i-1]==10 || buffer[i-1]==' '))
		i--;
	buffer[i]=0;
	return i;
}

void tt_init()
{
	memset(screentext, ' ', 40*25);
}

void tt_clear()
{
	int i;
	memset(screentext, ' ', 40*25);
	fillscreen(FillColor);
	Cursor.c = 0;
	Cursor.r = 0;
	for (i=0;i<25;i++)
	{
		screentext[i*40+39] = 10;
		pchar(39*8, i*8, 10);
	}
}

void tt_color(int c)
{
	TextColor = c;
	Color = c;
}

void tt_bgcolor(int c)
{
	TextBackGround = c;
	FillColor = c;
}

void tt_putchar(int c)
{
	switch(c) {
	case 10:
		screentext[Cursor.r*40+39] = 10;
		pchar(39*8, Cursor.r*8, 10);
		Cursor.c = 0;
		Cursor.r++;
	break;
	case 8:
		Cursor.c--;
		if (Cursor.c<0) {
			Cursor.c=39;
			Cursor.r--;
			if (Cursor.r<0) {
				Cursor.r=0;
				Cursor.c=0;
			}
		}
		c = (Cursor.c==39) ? 10: ' ';
		screentext[Cursor.r*40+Cursor.c] = c;
		pchar(Cursor.c*8, Cursor.r*8, c);
	break;
	default:
		screentext[Cursor.r*40+Cursor.c] = c;
		pchar(Cursor.c*8, Cursor.r*8, c);
		Cursor.c++;
		if (Cursor.c>39) {
			Cursor.c=0;
			Cursor.r++;
		}
	}

	if (Cursor.r > 24) {
		Cursor.r = 24;
		memmove(screen, screen+SCR_W*8, 192*SCR_W);
		memset(&screen[192*SCR_W], TextBackGround, 8*SCR_W);

		memmove(screentext, screentext+40, 40*24);
		memset(screentext+24*40, ' ', 40);
		screentext[24*40+39] = 10;
		pchar(39*8, 24*8, 10);
	}

	if (gfx_window)
		gfx_maybe_present();
}

char linebuffer[1024];
int lbpos=0;
int lbmode=0;
int lbflush=0;

unsigned char xgetch()
{
	int s=0;
	for(;;) {
		int i;

		if (kbhit())
			break;

		hide_cursor();
		s = !s;
		for(i=0; i<20; i++) {
			delay(10);
			if (kbhit())
				break;
		}
	}
	if (s)
		hide_cursor();
	return getch();
}

int tt_getchar()
{
	int ch;
	if (lbflush<lbpos) {
		ch = linebuffer[lbflush++];
		if (lbflush==lbpos) {
			lbpos=0;
			lbflush=0;
			lbmode=0;
		}
		return ch;
	}

	lbpos=0;
	lbflush=0;
	lbmode=0;

	do {
		ch = xgetch();
		switch(ch) {
		case 0: xgetch(); break;
		case 8:
			if (lbpos>0) {
				lbpos--;
				tt_putchar(ch);
			}
			break;
		case 10: break;
		case 13:
			if (lbpos < 1024) {
				linebuffer[lbpos++]='\n';
			}
			else {
				linebuffer[1023]='\n';
			}
			tt_putchar('\n');
			break;
		default:
			if (lbpos<1024) {
				linebuffer[lbpos++]=ch;
				tt_putchar(ch);
			}
		}
	} while (ch!=13 && ch!=10);

	if (lbpos>0) {
		return linebuffer[lbflush++];
	}
	return EOF;
}
