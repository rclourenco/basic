#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include "basic.h"

BasNode *rootnode=NULL;
BasNode **current=&rootnode;

BasNode *BasNodeCreate(BasNode **link, int type, void *data)
{
	BasNode *x=(BasNode *)malloc(sizeof(BasNode));
	if(!x)
		return NULL;

	*link=x;
	x->data=data;
	x->type=type;
	x->next=NULL;

	return x;
}

BasNode *BasForNodeCreate(BasNode **link)
{
	BasForNode *n=(BasForNode *)malloc(sizeof(BasForNode));
	if(!n)
		return NULL;

	return BasNodeCreate(link, BAS_FOR, n);
}

BasNode *BasIfNodeCreate(BasNode **link)
{
	BasIfNode *n=(BasIfNode *)malloc(sizeof(BasIfNode));
	if(!n)
		return NULL;

	return BasNodeCreate(link, BAS_IF, n);
}

BasNode *BasPrintNodeCreate(BasNode **link)
{
	BasPrintNode *n=(BasPrintNode *)malloc(sizeof(BasPrintNode));
	if(!n)
		return NULL;

	return BasNodeCreate(link, BAS_PRINT, n);
}

BasNode *BasInputNodeCreate(BasNode **link)
{
	BasInputNode *n=(BasInputNode *)malloc(sizeof(BasInputNode));
	if(!n)
		return NULL;

	return BasNodeCreate(link, BAS_INPUT, n);
}

BasNode *BasAssignmentNodeCreate(BasNode **link)
{
	BasAssignmentNode *n=(BasAssignmentNode *)malloc(sizeof(BasAssignmentNode));
	if (!n)
		return NULL;

	return BasNodeCreate(link, BAS_ASSIGNMENT, n);
}

BasExpression *BasExpressionCreate()
{
	BasExpression *n=(BasExpression *)malloc(sizeof(BasExpression));
	if(!n)
		return NULL;
	
	n->list = NULL;
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

	pn->list = NULL;
	BasExpressionList **link = &pn->list;	
	do {
		BasExpression *e = BasExpressionCreate();
		if (!e)
			return -4;
		BasExpressionList *l = BasExpressionListAdd(link, e);
		if (!l) {
			free(e);
			return -4;
		}
		link = &l->next;

		int r = BasBuildExpression(e, tokenizer, BAS_EXPR_PRINT);
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
	BasToken *t;

	do {
		t=bas_token_get(tokenizer);
	} while (t && t->type != TOKEN_LINE);
	return 0;
}

int BuildAssignmentBlock(BasNode *n, BasToken *current, BasTokenizer *tokenizer)
{
	BasAssignmentNode *f = (BasAssignmentNode *)n->data;

	BasToken *t;

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

	int r = BasBuildExpression(f->expr, tokenizer, BAS_EXPR_ASSIGN);
	if (r)
		return r;

	/*
	do {
		t=bas_token_get(tokenizer);
	} while (t && t->type != TOKEN_LINE);
	*/
	return 0;
}


int BasBlock(BasNode **link, BasTokenizer *tokenizer, int type)
{
	BasToken *token;

	while( (token=bas_token_get(tokenizer)) ) {
		if (type!=0)
			printf(">>>> ");

		printf("A %d -> %s\n", token->type, token->content);

		if (token->type == TOKEN_NAME) {
			if(!strcasecmp(token->content, "for")) {
				BasNode *n = BasForNodeCreate(link);
				if(!n)
					return -2;
				link = &n->next;
				{
					int r=BuildForBlock(n, tokenizer);
					if(r) {
						return r;
					}
				}
				printf("For cycle: ");
			}
			else if(!strcasecmp(token->content, "if")) {
				BasNode *n = BasIfNodeCreate(link);
				if(!n)
					return -2;
				link = &n->next;
				{
					int r=BuildIfBlock(n, tokenizer);
					if(r) {
						return r;
					}
				}
				printf("If block: ");
			}
			else if(!strcasecmp(token->content, "print")) {
				BasNode *n = BasPrintNodeCreate(link);
				if(!n)
					return -2;
				link = &n->next;
				{
					int r=BuildPrintBlock(n, tokenizer);
					if(r) {
						return r;
					}
				}	
				printf("Print: ");
			}
			else if(!strcasecmp(token->content, "input")) {
				BasNode *n = BasInputNodeCreate(link);
				if(!n)
					return -2;
				link = &n->next;
				{
					int r=BuildInputBlock(n, tokenizer);
					if(r) {
						return r;
					}
				}
				printf("Input: ");
			}
			else if(!strcasecmp(token->content, "end")) {
				if (type==BAS_IF || type==BAS_ELSE) {
					return 0;
				}
				printf("Unexpected end\n");
				return -4;
			}
			else if(!strcasecmp(token->content, "else")) {
				if (type==BAS_IF) {
					return 1;
				}
				printf("Unexpected end\n");
				return -4;
			}
			else if(!strcasecmp(token->content, "next")) {
				if(type==BAS_FOR) {
					printf("</FOR>\n");
//					bas_token_unget(tokenizer);
					return 0;
				}
				printf("Unexpected next\n");
				return -4;
			}
			else {
				BasNode *n = BasAssignmentNodeCreate(link);
				if(!n)
					return -2;
				link = &n->next;
				{
					int r=BuildAssignmentBlock(n, token, tokenizer);
					if(r) {
						return r;
					}
				}	
				printf("Assign: ");

				/*
				BasToken *nt=bas_token_get(tokenizer);
				if(!nt) {
					printf("Unexpected end... expecting ’=’\n");
					return -4;
				}

				printf(">>>>>> %s\n", nt->content);

				if(nt->type == TOKEN_OP && !strcasecmp(nt->content, "=")) {
					printf("Assignment: %s <= ...\n", nt->content);
				}
				else {
					printf("Unexpected token, expecting ’=’\n");
				}
				*/
			}
		}
	}
	printf("<BLOCK RETURN>\n");
	return 0; /* success */
}

int BasBuildExpression(BasExpression *e, BasTokenizer *tokenizer, int type)
{
	BasToken *t;
	e->list = NULL;

	BasTokenItem **link = &e->list;
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

		BasTokenItem *i = BasTokenItemAdd(link, t);
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
	if (!expr)
		return;
	BasTokenItem *ti = expr->list;
	while(ti) {
		dump_token(&ti->token);
		ti=ti->next;
		if (ti)
			printf(" ");
	}
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
	for (i=0;i<tab;i++) putchar('\t');
	BasExpressionList *l = x->list;
	printf("@print");
	int nl = 1;
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
	int i;
	for (i=0;i<tab;i++) putchar('\t');
	printf("INPUT <TODO>\n");
}

void dump_assignment(BasAssignmentNode *x, int tab)
{
	int i;
	for (i=0;i<tab;i++) putchar('\t');
	printf("Let %s =", x->var);
	BasTokenItem *ti = x->expr->list;

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
			//printf("Cur Type: %d\n", cur->type);
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
			}
			cur = cur->next;
		}
	}
}

int main(int argc, char **argv)
{	
	FILE *fp;
	unsigned long counter;
	char buffer[MLEN+1];
	StreamReader reader;
	BasTokenizer tokenizer;
	BasToken *token;

	if (argc<2)
		return 0;
	fp = fopen(argv[1],"r");
	if(!fp)
	{
		return 1;
	}

	stream_reader_init(&reader, fp);
	bas_token_init(&tokenizer, &reader);

	BasBlock(&rootnode, &tokenizer, 0);
	
	printf("\n===================================[BEGIN]==========================================\n");
	dump(rootnode, 0);
	printf("\n===================================[END]============================================\n");

	fclose(fp);
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
	bt->last.type = TOKEN_NAME;
	int i = 0;
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

