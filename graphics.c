/*
	Biblioteca programada por Renato Louren‡o.
*/
//biblioteca para o modo grafico 13h

#include <dos.h>
#include <stdlib.h>
#include <stdio.h>
#include <conio.h>
#include "graphics.h"


#include "fontc64.c"

unsigned char DRAW=COPY_D;
unsigned char Color=15;
unsigned char FillColor=15;
unsigned char TextColor=7;
unsigned char TextBackGround=255;
unsigned char far *dscreen=(unsigned char far *)MK_FP(0xA000,0x0);
unsigned char far *screen=(unsigned char far *)MK_FP(0xA000,0x0);

char ScrnT=DEFAULT;
unsigned char far *cart=console_font_8x8;


void _fxmemset(void far *a, int c, unsigned s)
{
	asm {
		mov ax, word ptr [a+2];
		mov es, ax;
		mov di, word ptr [a];
		mov cx, s;
		mov ax, c;
		cld;
		rep stosb;
	}
}

void _fxmemcpy(void far *a, void far *b, unsigned s)
{
	asm {
		push ds;
		les di, a;
		lds si, b;
		mov cx, s;
		cld;
		rep movsb;
		pop ds;
	}
}



//implementa‡Æo

void modo13h()
{
	asm {
		mov ax, 0x13;
		int 0x10;
	}
}

void modo3h()
{
	asm {
		mov ax, 0x3;
		int 0x10;
	}
}

unsigned char far *getcpt()
{
	unsigned s;
	unsigned o;

	asm {
		push es;
		push bp;
		
		mov ax, 0x1130;
		mov bx, 0x0300;
		int 0x10;
		mov ax, es;
		mov bx, bp;
		pop bp;
		pop es;

		mov s, ax;
		mov o, bx;
	}
	
	
	return ((unsigned char far *) MK_FP(s,o));
}

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
	_fxmemset(screen,a,64000);
}

//instru‡äes de desenho
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

/*
void DrawCircle(int x,int y,int r,char op)
{
	int c1,c2;
	int r2=r-1;
	double alfa;
	double inca;
	int px,py;
	char cor;
	if ((r==0)||(r>180))
	{
		DrawPixel(x,y,Color);
		return;
	}
	inca=PI/(PI*r);
	if(op!=1)
	{
		for(c1=x-r2;c1<=x+r2;c1++)
		for(c2=y-r2;c2<=y+r2;c2++)
		{
			if((long double)((x-c1)*(x-c1))+((y-c2)*(y-c2))<(long double)(r*r)-r)
				DrawPixel(c1,c2,FillColor);
		}
	}
	if(op==2)
		cor=FillColor;
	else
		cor=Color;
	for(alfa=0;alfa<PI/2;alfa+=inca)
	{
		px=cos(alfa)*r;
		py=sin(alfa)*r;
		DrawPixel(x+px,y+py,cor);
		DrawPixel(x-px,y-py,cor);
		DrawPixel(x-px,y+py,cor);
		DrawPixel(x+px,y-py,cor);
	}
}
*/

void DrawPixel(int x,int y,char c)
{
	if ((x>=0)&&(y>=0)&&(x<320)&&(y<200))
	{
		switch (DRAW)
		{
			case XOR_D:PLOT(x,y,c^GETPLOT(x,y));break;
			case AND_D:PLOT(x,y,c&GETPLOT(x,y));break;
			case OR_D:PLOT(x,y,c|GETPLOT(x,y));break;
			default:PLOT(x,y,c);
		}

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



/*
void DrawLineX(int x1,int y1,int x2,int y2)
{
	long vx,vy;
	int x,y;
	double a,nv;
	vx=x2-x1;
	vy=y2-y1;
	nv=(double)sqrt((vx*vx)+(vy*vy));
	if(nv!=0)
	for(a=0;a<=1;a+=(1/nv))
	{
		x=x1+a*vx+.5;
		y=y1+a*vy+.5;
		DrawPixel(x,y,Color);
	}
	DrawPixel(x2,y2,Color);
}
*/

unsigned PicSize(int x1,int y1,int x2,int y2)
{
	int x,y;
	if (x1>x2) swap(&x1,&x2);
	if (y1>y2) swap(&y1,&y2);
	x=x2-x1+1;
	y=y2-y1+1;
	if((x>320)&&(y>200))
		return 0;
	return x*y+10;
}

void swap(int *a,int *b)
{
	int temp;
	temp=*a;
	*a=*b;
	*b=temp;
}

void GetPic(int x1,int y1,int x2,int y2,TPicture far *pic)
{
	int x,y;
	int cy,cx;
	char far *d;
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

int XPRes(TPicture far *pic)
{
	return pic->w;
}

int YPRes(TPicture far *pic)
{
	return pic->h;
}

void DrawPicF(int x1,int y1,TPicture far *pic)
{
	char far *data=pic->data;
	int x2=pic->w;
	int y2=pic->h;
	int py,y;

	py=y1;
	for(y=0;y<y2;y++)
	{
		py=(y1+y)*320;
		_fxmemcpy(&screen[x1+py],data,x2);
		data+=x2;
	}
}


void DrawPic(int x1,int y1,TPicture far *pic)
{
	char far *data=pic->data;
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

void DrawPicB(int x1,int y1,TPicture far *pic)
{
	char far *data=pic->data;
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

void DefaultScreen()
{
	ScrnT=DEFAULT;
	screen=dscreen;
}

void UserScreen(unsigned char far *s)
{
	ScrnT=USER;
	screen=s;
}

char ScrnCpyUD()
{
	if (ScrnT!=USER) return 0;
	_fxmemcpy(dscreen,screen,64000);
	return 1;
}

char ScrnCpyDU()
{
	if (ScrnT!=USER) return 0;
	_fxmemcpy(screen,dscreen,64000);
	return 1;
}

void SetOverScan(char c)
{
	asm {
		mov ax, 0x1001;
		mov bh, c;
		xor bl, bl;
		int 0x10;
	}
}

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
	char *start = screentext+Cursor.r*40;
	if (Cursor.r>0 && screentext[(Cursor.r-1)*40+39]!=10)
		start = screentext+(Cursor.r-1)*40;

	if (max>80)
		max = 80;
	/*
	if (Cursor.r==24 && max>40)
		max = 40;
        */
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
	_fxmemset(screentext, ' ', 40*25);
}

void tt_clear()
{
	int i;
	_fxmemset(screentext, ' ', 40*25);
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
	//hide_cursor();
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
		_fxmemcpy(screen, screen+320*8, 192*320);
		_fxmemset(&screen[192*320], TextBackGround, 8*320);

		_fxmemcpy(screentext, screentext+40, 40*24);
		_fxmemset(screentext+24*40, ' ', 40);
		screentext[24*40+39] = 10;
		pchar(39*8, 24*8, 10);
	}
	//show_cursor();
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
/*
void SetDacRegs(unsigned a,unsigned char r,unsigned char g,unsigned char b)
{
	struct REGPACK regs;
	regs.r_ax=0x1010;
	regs.r_bx=a;
	regs.r_cx=(g<<8)+b;
	regs.r_dx=r<<8;
	intr(0x10,&regs);
}

void GetDacRegs(unsigned a,unsigned char *r,unsigned char *g,unsigned char *b)
{
	struct REGPACK regs;
	regs.r_ax=0x1015;
	regs.r_bx=a;
	intr(0x10,&regs);
	*r=regs.r_dx>>8;
	*g=regs.r_cx>>8;
	*b=(regs.r_cx<<8)>>8;
}
*/
