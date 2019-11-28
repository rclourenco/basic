#include <stdio.h>
#include <stdlib.h>
#include <alloc.h>
#include "basic.h"
#include "graphics.h"

#define MLINES 1024

char **editlines = NULL;
size_t editlen=0;

unsigned char basedit_text[1024];

void basedit_init()
{
	int i;
	for (i=0;i<1024;i++) {
		basedit_text[i]=' ';
	}
	editlines = (char **)malloc(sizeof(char *)*MLINES);
}


int basedit_load(char *filename)
{
	FILE *fp;

	if (editlines==NULL)
		basedit_init();
	if (editlines==NULL)
		return 0;

        fp = fopen(filename, "rt");
	if (!fp) {
		return 0;
	}
		
	fclose(fp);
	return 1;
}

int basedit_char(unsigned char ch)
{
	tt_putchar(ch);
	
	return 0;
}


typedef struct {
	unsigned char buffer[81];
	int pos;
}TDirectData;

TDirectData direct_data;

int direct_data_reader(void *data)
{
	TDirectData *d = (TDirectData *)data;
	int a;
	
	if (d->pos>=40)
		return EOF;

	a = d->buffer[d->pos];
	if (!a)
		return EOF;
	d->pos++;
	return a;
}

void basedit_line()
{
	StreamReader reader;
	BasTokenizer tokenizer;
	BasNode *rootdirect = NULL;

	int i;
	i = tt_getline(direct_data.buffer, 40);
	direct_data.buffer[i] = 0;

	direct_data.pos=0;
	stream_reader_init(&reader, direct_data_reader, &direct_data);
	bas_token_init(&tokenizer, &reader);
	i = BasBlock(&rootdirect, &tokenizer, BAS_DIRECT);
	if (i==BAS_STORE) {
		// BasNodeFree(&rootdirect);
		return;
	}

	if (i==0 && rootdirect) {
		tt_putchar(10);
		basexec(rootdirect, 0);
		if (Cursor.c!=0)
			tt_putchar(10);
		basPrintf("Ready.");
		BasNodeFree(&rootdirect);
		return;
	}

	BasNodeFree(&rootdirect);

	basPrintf("\nInvalid Token: %s\nReady.", tokenizer.last.content);
}

void basedit_loop()
{
	unsigned char key=0;

	basedit_init();
	basPrintf("***** BASIC *****\n");
	basPrintf("Ready.\n");

	while ( (key=xgetch())!=27 ) {
		
		switch(key) {
			case 0:  
				switch(xgetch())
				{
					case 0x4d: if (Cursor.c<39) Cursor.c++; break;
					case 0x4b: if (Cursor.c>0)  Cursor.c--; break;
					case 0x48: if (Cursor.r>0)  Cursor.r--; break;
					case 0x50: if (Cursor.r<24) Cursor.r++; break;
				}
				break;
			case 13: basedit_line(); tt_putchar(10); break;
			default:
				 basedit_char(key);
		}

		if (basexec_quit())
			break;
	}
}
