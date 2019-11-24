#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include "basic.h"
#include "graphics.h"

BasNode *rootnode=NULL;
BasNode **current=&rootnode;

BasNode *BasNodeCreate(BasNode **link, int type, void *data, int ln)
{
	BasNode *x=(BasNode *)malloc(sizeof(BasNode));
	if(!x)
		return NULL;

	*link=x;
	x->data=data;
	x->type=type;
	x->next=NULL;
	x->ln = ln;
	return x;
}

BasNode *BasGotoNodeCreate(BasNode **link, int ln)
{
	BasGotoNode *n=(BasGotoNode *)malloc(sizeof(BasGotoNode));
	if (!n)
		return NULL;

	return BasNodeCreate(link, BAS_GOTO, n, ln);
}


BasNode *BasForNodeCreate(BasNode **link, int ln)
{
	BasForNode *n=(BasForNode *)malloc(sizeof(BasForNode));
	if(!n)
		return NULL;

	return BasNodeCreate(link, BAS_FOR, n, ln);
}

BasNode *BasIfNodeCreate(BasNode **link, int ln)
{
	BasIfNode *n=(BasIfNode *)malloc(sizeof(BasIfNode));
	if(!n)
		return NULL;

	return BasNodeCreate(link, BAS_IF, n, ln);
}

BasNode *BasPrintNodeCreate(BasNode **link, int ln)
{
	BasPrintNode *n=(BasPrintNode *)malloc(sizeof(BasPrintNode));
	if(!n)
		return NULL;

	return BasNodeCreate(link, BAS_PRINT, n, ln);
}

BasNode *BasInputNodeCreate(BasNode **link, int ln)
{
	BasInputNode *n=(BasInputNode *)malloc(sizeof(BasInputNode));
	if(!n)
		return NULL;
	n->prompt[0]='\0';
	n->list=NULL;
	return BasNodeCreate(link, BAS_INPUT, n, ln);
}

BasVarList *BasVarListAdd(BasVarList **link, char *var)
{
	BasVarList *n = (BasVarList *)malloc(sizeof(BasVarList));
	if (!n)
		return NULL;
	strncpy(n->var,var,MLEN-1);
	n->var[MLEN-1]='\0';
	n->next=NULL;
	*link = n;
	return n;
}

BasNode *BasAssignmentNodeCreate(BasNode **link, int ln)
{
	BasAssignmentNode *n=(BasAssignmentNode *)malloc(sizeof(BasAssignmentNode));
	if (!n)
		return NULL;

	return BasNodeCreate(link, BAS_ASSIGNMENT, n, ln);
}

BasExpression *BasExpressionCreate()
{
	BasExpression *n=(BasExpression *)malloc(sizeof(BasExpression));
	if(!n)
		return NULL;
	
	n->list = NULL;
	n->istk.max  = 0;
	n->istk.size = 0;
	n->istk.contents = NULL;
	n->tlist = NULL;
	n->size = 0;
	return n;
}

BasExpressionList *BasExpressionListAdd(BasExpressionList **link, BasExpression *e)
{
	BasExpressionList *n=(BasExpressionList *)malloc(sizeof(BasExpressionList));
	if(!n)
		return NULL;
	n->expr = e;
	n->next = NULL;
	n->separator = 0;
	*link = n;
	return n;
}

BasTokenItem *BasTokenItemAdd(BasTokenItem **link, BasToken *t)
{
	BasTokenItem *n=(BasTokenItem *)malloc(sizeof(BasTokenItem));
	if(!n)
		return NULL;
	n->token = *t;
	n->next = NULL;
	*link = n;
	return n;
}

int BuildForBlock(BasNode *n, BasTokenizer *tokenizer)
{
	BasForNode *f = (BasForNode *)n->data;
	BasToken *t;
	t=bas_token_expect(tokenizer, TOKEN_NAME);
	if(!t) {
		return -4;
	}

	strncpy(f->counter,t->content, MLEN-1);
	f->counter[MLEN-1]='\0';

	t=bas_token_expect_operator(tokenizer, "=");
	if(!t) {
		return -4;
	}

	t=bas_token_expect(tokenizer, TOKEN_NUM);
	if(!t) {
		return -4;
	}

	f->start=atof(t->content);

	t=bas_token_expect_name(tokenizer, "to");
	if(!t) {
		return -4;
	}

	t=bas_token_expect(tokenizer, TOKEN_NUM);
	if(!t) {
		return -4;
	}

	f->stop=atof(t->content);

	t=bas_token_get(tokenizer);
	if(!t) {
		printf("Unexpected end...\n");
		return -4;
	}

	if(t->type==TOKEN_LINE) {
		f->step=1.0;
		goto for_block;
	} else if(t->type!=TOKEN_NAME || strcmp(t->content,"step")) {
		printf("Unexpected token, expecting: step\n");
		return -4;
	}

	t=bas_token_expect(tokenizer, TOKEN_NUM);
	if(!t) {
		return -4;
	}
	f->step=atof(t->content);

	t=bas_token_expect(tokenizer, TOKEN_LINE);
	if(!t) {
		return -4;
	}

for_block:
	return BasBlock(&f->block, tokenizer, BAS_FOR);

}

int BuildIfBlock(BasNode *n, BasTokenizer *tokenizer)
{
	int r;
	BasIfNode *f = (BasIfNode *)n->data;
	BasToken *t;

	f->then_block = NULL;
	f->else_block = NULL;
	f->expr = BasExpressionCreate();
	if (!f->expr)
		return -4;

	r = BasBuildExpression(f->expr, tokenizer, BAS_EXPR_IF);
	if (r)
		return r;


//	f->start=atof(t->content);

	t=bas_token_expect_name(tokenizer, "then");
	if(!t) {
		return -4;
	}

	t=bas_token_expect(tokenizer, TOKEN_LINE);
	if(!t) {
		return -4;
	}

if_block:
	r = BasBlock (&f->then_block, tokenizer, BAS_IF);
	if (r==1) {
		r = BasBlock(&f->else_block, tokenizer, BAS_ELSE);
	}
	return r;

}


int BuildPrintBlock(BasNode *n, BasTokenizer *tokenizer)
{
	BasPrintNode *pn = (BasPrintNode *)n->data;
	BasToken *t;
	BasExpressionList **link;

	pn->list = NULL;
	link = &pn->list;	
	do {
		int r;
		BasExpression *e;
		BasExpressionList *l;
	        e = BasExpressionCreate();
		if (!e)
			return -4;
		l = BasExpressionListAdd(link, e);
		if (!l) {
			free(e);
			return -4;
		}
		link = &l->next;

		r = BasBuildExpression(e, tokenizer, BAS_EXPR_PRINT);
		if (r)
			return r;
		t=bas_token_get(tokenizer);
		if (!t)
			break;
		switch (t->type) {
			case TOKEN_COMMA:     l->separator = TOKEN_COMMA;     break;
			case TOKEN_SEMICOLON: l->separator = TOKEN_SEMICOLON; break;
			case TOKEN_LINE:      l->separator = TOKEN_LINE;      break;
			default: return -4;
		}
	} while (t && t->type != TOKEN_LINE);
	return 0;
}

int BuildInputBlock(BasNode *n, BasTokenizer *tokenizer)
{
	BasInputNode *f = (BasInputNode *)n->data;
	BasToken *t;
	BasVarList **link;
	BasVarList *c;

	t=bas_token_get(tokenizer);
	if (!t)
		return -4;

	link = &f->list;

	switch(t->type)
	{
		case tokenString:
			strncpy(f->prompt, t->content, 255);
			f->prompt[255]='\0';
		break;
		case tokenVariable:
			c = BasVarListAdd(link, t->content);
			if (!c)
				return -4;
			link = &c->next;
		break;
		default:
			return -4;
	}
	for(;;) {
		t = bas_token_get(tokenizer);
		if (!t)
			break;

		if (t->type == tokenLine)
		       	break;
	
		if (t->type != tokenComma && t->type!= tokenSemicolon)
			return -4;

		t = bas_token_get(tokenizer);
		if (!t)
			return -4;
		if (t->type!=tokenVariable)
			return -4;

		
		c = BasVarListAdd(link, t->content);
		if (!c)
			return -4;
		link = &c->next;
	}
	printf("Input Done!\n");
	return 0;
}

int BuildAssignmentBlock(BasNode *n, BasToken *current, BasTokenizer *tokenizer)
{
	BasAssignmentNode *f = (BasAssignmentNode *)n->data;
	BasToken *t;
	int r;

	if (current != NULL && current->type==tokenVariable) {
		strncpy(f->var, current->content, MLEN-1);
		f->var[MLEN-1]='\0';
	}

	t=bas_token_expect_operator(tokenizer, "=");
	if(!t) {
		return -4;
	}

	f->expr = BasExpressionCreate();
	if (!f->expr)
		return -4;

	r = BasBuildExpression(f->expr, tokenizer, BAS_EXPR_ASSIGN);
	if (r)
		return r;

	/*
	do {
		t=bas_token_get(tokenizer);
	} while (t && t->type != TOKEN_LINE);
	*/
	return 0;
}

int BuildGotoBlock(BasNode *n, BasTokenizer *tokenizer)
{
	BasGotoNode *f = (BasGotoNode *)n->data;

	BasToken *t;


	t=bas_token_expect(tokenizer, tokenNumber);
	if (!t)
		return -4;
	f->goline = atoi(t->content);
	return 0;
}


int BasBlock(BasNode **link, BasTokenizer *tokenizer, int type)
{
	BasToken *token;

	int ln = -1;
	int cn = -1;
	while( (token=bas_token_get(tokenizer)) != NULL ) {
		if (token->type == tokenNumber) {
			printf("Number: %s\n", token->content);
			if (type==0)
				cn = atoi(token->content);
			continue;
		}
		ln = cn;
		cn = -1; 

		if (token->type == TOKEN_NAME) {
			if(!strcasecmp(token->content, "for")) {
				BasNode *n = BasForNodeCreate(link, ln);
				if(!n)
					return -2;
				link = &n->next;
				{
					int r=BuildForBlock(n, tokenizer);
					if(r) {
						return r;
					}
				}
			}
			else if(!strcasecmp(token->content, "if")) {
				BasNode *n = BasIfNodeCreate(link, ln);
				if(!n)
					return -2;
				link = &n->next;
				{
					int r=BuildIfBlock(n, tokenizer);
					if(r) {
						return r;
					}
				}
			}
			else if(!strcasecmp(token->content, "print")) {
				BasNode *n = BasPrintNodeCreate(link, ln);
				if(!n)
					return -2;
				link = &n->next;
				{
					int r=BuildPrintBlock(n, tokenizer);
					if(r) {
						return r;
					}
				}	
			}
			else if(!strcasecmp(token->content, "input")) {
				BasNode *n = BasInputNodeCreate(link, ln);
				if(!n)
					return -2;
				link = &n->next;
				{
					int r=BuildInputBlock(n, tokenizer);
					if(r) {
						return r;
					}
				}
			}
			else if(!strcasecmp(token->content, "goto")) {
				BasNode *n = BasGotoNodeCreate(link, ln);
				if(!n)
					return -2;
				link = &n->next;
				{
					int r=BuildGotoBlock(n, tokenizer);
					if(r) {
						return r;
					}
				}
			}
			else if(!strcasecmp(token->content, "end")) {
				if (type==BAS_IF || type==BAS_ELSE) {
					return 0;
				}
				return -4;
			}
			else if(!strcasecmp(token->content, "else")) {
				if (type==BAS_IF) {
					return 1;
				}
				return -4;
			}
			else if(!strcasecmp(token->content, "next")) {
				if(type==BAS_FOR) {
					return 0;
				}
				printf("Unexpected next\n");
				return -4;
			}
			else {
				BasNode *n = BasAssignmentNodeCreate(link, ln);
				if(!n)
					return -2;
				link = &n->next;
				{
					int r=BuildAssignmentBlock(n, token, tokenizer);
					if(r) {
						return r;
					}
				}	

			}
		}
	}
	return 0; /* success */
}

int BasBuildExpression(BasExpression *e, BasTokenizer *tokenizer, int type)
{
	BasToken *t;
	BasTokenItem **link;
	BasTokenItem *i;

	e->list = NULL;
	link = &e->list;
	do {
		t=bas_token_get(tokenizer);
		if (!t)
			break;
		if (t->type==TOKEN_LINE) {
			bas_token_back(tokenizer);
			break;
		}

		
		switch (type)
		{
		case BAS_EXPR_IF:
			if (t->type == TOKEN_NAME && !strcasecmp(t->content, "then")) {
				bas_token_back(tokenizer);
				goto next;
			}
			break;
		case BAS_EXPR_PRINT:
			if (t->type == TOKEN_COMMA || t->type == TOKEN_SEMICOLON) {
				bas_token_back(tokenizer);
				goto next;
			}
			break;
		}

		i = BasTokenItemAdd(link, t);
		if (!i)
			return -4;
		link = &i->next;
	} while (t && t->type != TOKEN_LINE);
next:
	BasRpnExpression(e);
	return 0;
}

void dump(BasNode *, int);
void dump_token(BasToken *t);


void dump_expr(BasExpression *expr)
{
	BasTokenItem *ti;
	if (!expr)
		return;

	ti = expr->list;
	while(ti) {
		dump_token(&ti->token);
		ti=ti->next;
		if (ti)
			printf(" ");
	}
}

void dump_goto(BasGotoNode *x, int tab)
{
	int i;
	for (i=0;i<tab;i++) putchar('\t');
	
	printf("Goto: @%d\n", x->goline);
}


void dump_for(BasForNode *x, int tab)
{
	int i;
	for (i=0;i<tab;i++) putchar('\t');
	
	printf("for (%s=%g; %s<=%g; %s+=%g) {\n", x->counter, x->start, x->counter, x->stop, x->counter, x->step);
	dump(x->block, tab+1);
	for (i=0;i<tab;i++) putchar('\t');
	printf("}\n");
}

void dump_if(BasIfNode *x, int tab)
{
	int i;
	for (i=0;i<tab;i++) putchar('\t');
	
	printf("if (");
	dump_expr(x->expr);
	printf(") {\n");
	dump(x->then_block, tab+1);
	for (i=0;i<tab;i++) putchar('\t');
	printf("}\n");

	if (! x->else_block)
		return;

	for (i=0;i<tab;i++) putchar('\t');
	printf("else {\n");
	dump(x->else_block, tab+1);
	for (i=0;i<tab;i++) putchar('\t');
	printf("}\n");
}

void dump_token(BasToken *t) 
{
	switch(t->type)
	{
		case TOKEN_OP:     printf("%s", t->content);   break;
		case TOKEN_NAME:   printf("%s", t->content); break;
		case TOKEN_NUM:    printf("%s", t->content);  break;
		case TOKEN_STRING: printf("\"%s\"",  t->content);  break;
		case tokenFunction: printf("F[%s]", t->content); break;
		default: printf("TOKEN[%s]", t->content);        break;
	}
}

void dump_print(BasPrintNode *x, int tab)
{
	int i;
	int nl = 1;
	BasExpressionList *l;

	for (i=0;i<tab;i++) putchar('\t');
	l = x->list;
	printf("@print");

	while(l) {
		BasTokenItem *ti = l->expr->list;
		if (!ti) break;

		while(ti) {
			printf(" ");
			dump_token(&ti->token);
			ti=ti->next;
		}
		nl = 0;
		switch(l->separator) {
			case TOKEN_COMMA:     printf(","); break;
			case TOKEN_SEMICOLON: printf(";"); break;
			default: nl = 1;
		}
		l=l->next;
	}
	if (nl)
		printf(" <nl>");
	printf("\n");
}

void dump_input(BasInputNode *x, int tab)
{
	BasVarList *v;
	int i;
	for (i=0;i<tab;i++) putchar('\t');
	printf("Input [%s] ", x->prompt);
	v = x->list;
	while(v) {
		printf(", @[%s]", v->var);
		v = v->next;
	}
	printf("\n");
}

void dump_assignment(BasAssignmentNode *x, int tab)
{
	BasTokenItem *ti;
	int i;
	for (i=0;i<tab;i++) putchar('\t');
	printf("Let %s =", x->var);
	ti = x->expr->list;

	while(ti) {
		printf(" ");
		dump_token(&ti->token);
		ti=ti->next;
	}
	printf("\n");

}

void dump(BasNode *root, int tab)
{
	if (root!=NULL) {
		BasNode *cur = root;
		while(cur) {
			if (cur->ln != -1)
				printf("@%d ", cur->ln);
			switch(cur->type)
			{
				case BAS_FOR:
					dump_for((BasForNode *)cur->data, tab);
				break;
				case BAS_IF:
					dump_if((BasIfNode *)cur->data, tab);
				break;
				case BAS_INPUT:
					dump_input((BasInputNode *)cur->data, tab);
				break;
				case BAS_PRINT:
					dump_print((BasPrintNode *)cur->data, tab);
				break;
				case BAS_ASSIGNMENT:
					dump_assignment((BasAssignmentNode *)cur->data, tab);
				break;
				case BAS_GOTO:
					dump_goto((BasGotoNode *)cur->data, tab);
				break;
			}
			cur = cur->next;
		}
	}
}

void run()
{
	/*
	int i;
	for(i=0;i<256;i++)
		syswritechar(i);
	*/
	syswritechar(10);
	basexec(rootnode, 0);
}


void list(const char *filename) 
{
	FILE *fp;

	fp = fopen(filename,"r");
	if(!fp)
	{
		return;
	}

	while(!feof(fp)) {
		int ch = fgetc(fp);
		syswritechar(ch);
	}

	fclose(fp);

}
int parse(const char *filename)
{
	FILE *fp;
	unsigned long counter;
	char buffer[MLEN+1];
	int r;
	StreamReader reader;
	BasTokenizer tokenizer;
	BasToken *token;

	fp = fopen(filename,"r");
	if(!fp)
	{
		return 1;
	}

	stream_reader_init(&reader, fp);
	bas_token_init(&tokenizer, &reader);

	r = BasBlock(&rootnode, &tokenizer, 0);

	fclose(fp);
	return r;

}

/*
 * Init Graphic system
 * Move this to another place
 *
 *
 */

void sysgetxy(int *x, int *y)
{
	unsigned char col, row;
	asm {
		mov ah, 0x3;
		mov bh, 0;
		int 10h;
		mov row, dh;
		mov col, dl;
	}
	*x = col;
	*y = row;
}

void syssetxy(int x, int y)
{
	asm {
		mov ah, 0x2;
		mov bh, 0;
		mov dh, byte ptr y;
		mov dl, byte ptr x;
		int 10h;
	}
}

#define USE_GRAPHICS

int syswritechar(int c) 
{
#ifdef USE_GRAPHICS 
	TextBackGround=1;
	TextColor=9;

	tt_putchar(c);
	return 1;
#else
	if (c==10) {
		int x, y;
		sysgetxy(&x,&y);
		x = 0;
		syssetxy(x, y);
	}

	asm {
		mov ah, 0xe;
		mov al, byte ptr c;
		mov bh, 0;
		mov bl, 0x9;
	       	int 0x10;	
	}

	return 1;
#endif
}

void init_system()
{
	asm {
		mov ax, 13h;
		int 10h;

		mov ah, 0xB;
		mov bh, 0;
		mov bl, 2;
		int 10h;
	}
	basSetPutchar(syswritechar);
	basSetGetchar(tt_getchar);
	SetOverScan(9);
	TextBackGround=1;
	TextColor=9;
	FillColor=1;
	fillscreen(1);
}

void close_system() 
{
	asm {
		mov ax, 3h;
		int 10h;
	}
	basSetPutchar(putchar);
	basSetGetchar(getchar);
}
/*
 * 
 */

int main(int argc, char **argv)
{	
	int r;
	if (argc<2)
		return 0;

	r = parse(argv[1]);
	if (r)
		return r;
	init_system();
	list(argv[1]);
	run();
	getchar();
	close_system();

	return 0;
}

int read_line(FILE *fp, char *buffer, size_t len)
{
	size_t count=0;
	while(!feof(fp))
	{
		int ch = fgetc(fp);
		if(ch==EOF || ch=='\n')
			break;
		else
		if(count<len)
			buffer[count++]=ch;
	}
	buffer[count]='\0';
	return count;
}

void stream_reader_init(StreamReader *r, FILE *fp)
{
	r->fp = fp;
	r->pos = 0;
}

int stream_reader_get(StreamReader *r)
{
	if(r->pos) {
		return r->buffer[--r->pos];
	}
	return fgetc(r->fp);
}

void stream_reader_unget(StreamReader *r, char c)
{
	if(r->pos < MAXBACK) {
		r->buffer[r->pos++]=c;
	}
}

BasToken *bas_token_expect(BasTokenizer *bt, int type)
{
	BasToken *t=bas_token_get(bt);
	if(!t){
		printf("Unexpected eof\n");
		return NULL;
	}

	if(t->type!=type)
	{
		printf("Unexpected token type\n");
		return NULL;
	}
	return t;
}

BasToken *bas_token_expect_name(BasTokenizer *bt, char *name)
{
	BasToken *t=bas_token_get(bt);
	if(!t) {
		printf("Unexpected eof\n");
		return NULL;
	}

	if(t->type!=TOKEN_NAME) {
		printf("Unexpected token type\n");
		return NULL;
	}

	if(strcmp(t->content, name)) {
		printf("Unexpected name: %s\n", t->content);
		return NULL;
	}

	return t;
}

BasToken *bas_token_expect_operator(BasTokenizer *bt, char *op)
{
	BasToken *t=bas_token_get(bt);
	if(!t) {
		printf("Unexpected eof\n");
		return NULL;
	}

	if(t->type!=TOKEN_OP) {
		printf("Unexpected token type\n");
		return NULL;
	}

	if(strcmp(t->content, op)) {
		printf("Unexpected operator: %s\n", t->content);
		return NULL;
	}

	return t;
}



void bas_token_init(BasTokenizer *bt, StreamReader *reader)
{
	bt->reader=reader;
	bt->last.type = TOKEN_NONE;
	bt->last.content[0]='\0';
	bt->before = bt->last;
	bt->on_hold = 0;
}

BasToken *bas_token_string(BasTokenizer *bt)
{
	int c;
	int i = 0;
	c = stream_reader_get(bt->reader);
	while(c!='\0' && c!='\n' && c!=EOF) {
		int q_esc = 0;
		if (!q_esc) {
			if (c=='"') {
				bt->last.type = TOKEN_STRING;
				bt->last.content[i]='\0';
				return &bt->last;
			} else if(c=='\\') {
				q_esc = 1;
			} else {
				bt->last.content[i++]=c;
			}
		} else {
			bt->last.content[i++]=c;
			q_esc = 0;
		}
		c = stream_reader_get(bt->reader);
	}
	bt->last.type = TOKEN_ERROR;
	return &bt->last;
}

BasToken *bas_token_name(BasTokenizer *bt, int c)
{
	int i = 0;

	bt->last.type = TOKEN_NAME;
	bt->last.content[i++]=c;
	c = stream_reader_get(bt->reader);
	while(isalnum(c) || c=='_') {
		bt->last.content[i++]=c;
		c = stream_reader_get(bt->reader);
	}

	while (isblank(c)) {
		c = stream_reader_get(bt->reader);
	}

	if(c=='(') 
		bt->last.type = TOKEN_FUNCTION;
	if(c!=EOF)
		stream_reader_unget(bt->reader, c);
	bt->last.content[i]='\0';
	return &bt->last;
}

BasToken *bas_token_number(BasTokenizer *bt, int c)
{
	int i = 0;
	bt->last.type = TOKEN_NUM;
	bt->last.content[i++]=c;

	if (c=='.')
		goto decimal_part;

	c = stream_reader_get(bt->reader);
	while(isdigit(c)) {
		bt->last.content[i++]=c;
		c = stream_reader_get(bt->reader);
	}

	if (c=='e' || c=='E') {
		bt->last.content[i++]=c;
		c = stream_reader_get(bt->reader);
		printf("jump to expoent");
		goto expoent;
	}

	if (c!='.')
		goto end_number;
	bt->last.content[i++]='.';

decimal_part:
	c = stream_reader_get(bt->reader);
	while(isdigit(c)) {
		bt->last.content[i++]=c;
		c = stream_reader_get(bt->reader);
	}
	if (c!='e' && c!='E') {
		goto end_number;
	}
	c = stream_reader_get(bt->reader);
expoent:
	if(c=='-' || c=='+' || isdigit(c)) {
		bt->last.content[i++]=c;
		c = stream_reader_get(bt->reader);
	} else {
		goto end_number;
	}

	while(isdigit(c)) {
		bt->last.content[i++]=c;
		c = stream_reader_get(bt->reader);
	}

end_number:
	if (c!=EOF)
		stream_reader_unget(bt->reader, c);
	bt->last.content[i]='\0';
	return &bt->last;
}

BasToken *bas_token_comment(BasTokenizer *bt)
{
	int c;
	int i=0;

	bt->last.type = TOKEN_COMMENT;

	c = stream_reader_get(bt->reader);
	while(c!='\n' && c!=EOF) {
		bt->last.content[i++]=c;
		c = stream_reader_get(bt->reader);
	}
	bt->last.content[i]='\0';
	return &bt->last;
}

void bas_token_back(BasTokenizer *bt)
{
	bt->on_hold=1;
}


BasToken *bas_token_get(BasTokenizer *bt)
{
	int c;

	if (bt->on_hold) {
		bt->on_hold = 0;
		return &bt->last;
	}

	bt->before = bt->last;
	c=stream_reader_get(bt->reader);
	while (c==' ' || c=='\t')
		c=stream_reader_get(bt->reader);
	if(c==EOF) {
		return NULL;
	}

	switch(c) {
		case '\0':
	       		return NULL;	       
		case '?':
		case ':':
		case '[':
		case ']':
		case '{':
		case '}':	
		case '(':
		case ')':
		case '/':
		case '\\':
		case '*':
		case '-':
		case '+':
		case '^':
		case '=':
		case '<':
		case '>':
			bt->last.type = TOKEN_OP;
			bt->last.content[0]=c;
			bt->last.content[1]='\0';
			return &bt->last;
		case ',':
			bt->last.type = TOKEN_COMMA;
			bt->last.content[0]=c;
			bt->last.content[1]='\0';
			return &bt->last;
		case ';':
			bt->last.type = TOKEN_SEMICOLON;
			bt->last.content[0]=c;
			bt->last.content[1]='\0';
			return &bt->last;
		case '\n':
			bt->last.type = TOKEN_LINE;
			sprintf(bt->last.content, "<nl>");
			return &bt->last;
		case '"': 
			return bas_token_string(bt);
		case '\'':
			return bas_token_comment(bt);
	default:
		if (isalpha(c) || c=='_') {
			return bas_token_name(bt, c);
		}

		if (isdigit(c) || c == '.') {
			return bas_token_number(bt, c);
		}

		bt->last.type = TOKEN_UNKNOWN;
		sprintf( bt->last.content, "<#%02X>", c);
		return &bt->last;
	}

	return NULL;
}

