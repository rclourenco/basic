#ifndef _GRAPHICS_H
	#define _GRAPHICS_H
	
typedef struct{
	unsigned w,h;
	char data[1];
}TPicture;

typedef struct{
	unsigned char B,G,R,U;
}Trgb;

//definiäes de desenho
#define LIFI 0
#define LINE 1
#define FILL 2
#define COPY_D 0
#define XOR_D 1
#define AND_D 2
#define OR_D 3

#define PLOT(X,Y,C) screen[(X)+(Y)*320]=C
#define GETPLOT(X,Y) screen[(X)+(Y)*320]
#define PI M_PI

#define DEFAULT 0
#define USER 1

void pchar(int x,int y, unsigned char c);
void writest(int x,int y,unsigned char *st);
void fillscreen(char a);
void swap(int *a,int *b);
extern unsigned char DRAW;
extern unsigned char Color;
extern unsigned char FillColor;
extern unsigned char TextColor;
extern unsigned char TextBackGround;
//unsigned char far *dscreen=(unsigned char far *)MK_FP(0xA000,0x0);
//unsigned char far *screen=(unsigned char far *)MK_FP(0xA000,0x0);

//instruäes graficas de interface com a bios
typedef unsigned char far *CPT;
void modo13h();//modo grafico
void modo3h();//modo texto
unsigned char far *getcpt();//retorna a posicao do mapa de caracteres
void SetOverScan(char c);
void SetDacRegs(unsigned a,unsigned char r,unsigned char g,unsigned char b);
void GetDacRegs(unsigned a,unsigned char *r,unsigned char *g,unsigned char *b);


//instruäes de desenho
void DrawPixel(int x,int y,char c);
void DrawRect(int x1,int y1,int x2,int y2,char op);
//void DrawCircle(int x,int y,int r,char op);
//void DrawLine(int x1,int y1,int x2,int y2);
#define DrawLine dline

void dline(int x1, int y1, int x2, int y2);
void dcircle(int x0, int y0, int r);

//instruäes para manipulaÆo de bitmaps
unsigned PicSize(int x1,int y1,int x2,int y2);
void GetPic(int x1,int y1,int x2,int y2,TPicture far *pic);
void DrawPic(int x1,int y1,TPicture far *pic);
void DrawPicB(int x1,int y1,TPicture far *pic);
void DrawPicF(int x1,int y1,TPicture far *pic);
int XPRes(TPicture far *pic);
int YPRes(TPicture far *pic);

//instruäes para ecran virtual
void DefaultScreen();
void UserScreen(unsigned char far *s);
char ScrnCpyUD();
char ScrnCpyDU();

unsigned char xgetch();

typedef struct {
	int c, r;
} TCursor;

extern TCursor Cursor;

void tt_locate(int c, int r);
void tt_color(int c);
void tt_bgcolor(int c);
void tt_clear();
int tt_getline(char *buffer, int max);
void tt_init();
void tt_putchar(int c);
int tt_getchar();
#endif

