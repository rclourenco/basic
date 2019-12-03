#include <stdio.h>
#include <stdlib.h>
#include <alloc.h>
#include "basic.h"
#include "graphics.h"

#define MLINES 1024

typedef struct {
	int index;
	char line[81];
} BasTextLine;

BasTextLine **editlines = NULL;
size_t editlen=0;

unsigned char basedit_text[1024];

void basedit_init()
{
	int i;
	for (i=0;i<1024;i++) {
		basedit_text[i]=' ';
	}
	editlines = (BasTextLine **)malloc(sizeof(BasTextLine *)*MLINES);
}

int basedit_store(int p,  char *line)
{
	int i=0;

	BasTextLine *new;

	if (p<0)
		return -1;

	for (i=0;i<editlen;i++) {
		if (editlines[i]->index == p) {
			memcpy(editlines[i]->line, line, 81);
			return p;
		}
	}

	if (editlen>=MLINES)
		return -1;

	new = (BasTextLine *)malloc(sizeof(BasTextLine));
	if (!new)
		return -2;

	for(i=0;i<editlen;i++) {
		if (editlines[i]->index > p)
			break;
	}

	if (i<editlen) {
		memmove(editlines+i, editlines+i+1, (editlen-i)*sizeof(BasTextLine *));
	}

	strcpy(new->line, line);
	new->index = p;

	editlines[i] = new;
	editlen++;

	return p;
}

void basedit_list(int start, int end)
{
	int i;
	if (end == -1) {
		end = 0x7FFF;
	}

	basPrintf("Total lines: %u\n", editlen);

	for (i=0;i<editlen;i++)
	{
		if (editlines[i]->index > end)
			break;

		if (editlines[i]->index < start)
			continue;
		basPrintf("%s\n", editlines[i]->line);
	}

//	basPrintf("Listing... from %u to %u\n", start, end);
}

typedef struct {
	int sindex;
	int index;
	int line;
	int pos;
}TRunData;

TRunData run_data;

int run_data_reader(void *data)
{
	TRunData *d = (TRunData *)data;
	int a;
	
	if (d->line>=editlen)
		return EOF;

	while (editlines[d->line]->index < d->sindex) {
		d->line++;
	}

	if (d->line>=editlen)
		return EOF;


	if ( editlines[d->line]->line[d->pos] == 0 ) {
		d->line++;
		d->pos=0;

		if (d->line < editlen) {
			d->index = editlines[d->line]->index;
			return '\n';
		}

		return EOF;
	}

	a = editlines[d->line]->line[d->pos];

	d->pos++;
	return a;
}


int basedit_run(int start)
{
	StreamReader reader;
	BasTokenizer tokenizer;
	BasNode *rootdirect = NULL;

	int i;
	run_data.pos = 0;
	run_data.line = 0;
	run_data.sindex = start;
	run_data.index = 0;
	stream_reader_init(&reader, run_data_reader, &run_data);
	bas_token_init(&tokenizer, &reader);
	i = BasBlock(&rootdirect, &tokenizer, 0);

	if (i==0 && rootdirect) {
		tt_putchar(10);
		basexec(rootdirect, 0);
		BasNodeFree(&rootdirect);
		return;
	}

	BasNodeFree(&rootdirect);
}

char read_buffer[81];

int basedit_load(char *filename)
{
	FILE *fp;

	if (filename==NULL) {
		basPrintf("load <filename>");
		return 0;
	}

	if (editlines==NULL)
		basedit_init();
	if (editlines==NULL)
		return 0;

    fp = fopen(filename, "rt");
	if (!fp) {
		return 0;
	}

	while(read_line(fp, read_buffer, 80)) {
		int ln=0;
		sscanf(read_buffer, "%d", &ln);
		basedit_store(ln, read_buffer);
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
	
	if (d->pos>=80)
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
	i = tt_getline(direct_data.buffer, 80);
	direct_data.buffer[i] = 0;

	direct_data.pos=0;
	stream_reader_init(&reader, direct_data_reader, &direct_data);
	bas_token_init(&tokenizer, &reader);
	i = BasBlock(&rootdirect, &tokenizer, BAS_DIRECT);
	if (i==BAS_STORE) {
/*
        int j=strlen(direct_data.buffer);
		while (j>0 && direct_data.buffer[j-1]==' ')
			j--;
		direct_data.buffer[j]=0;
*/
		{
			int v = basedit_store(atoi(tokenizer.last.content), direct_data.buffer);
			if (v<0)
				basPrintf("Out of memory");
		}
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
