#include <stdio.h>
#include <ctype.h>

#define MLEN 255

#define TOKEN_NONE 0
#define TOKEN_EOL  1
#define TOKEN_OP   2
#define TOKEN_NUM  3
#define TOKEN_NAME 4
#define TOKEN_STRING 5
#define TOKEN_ERROR 999

int read_line(FILE *fp, char *buffer, size_t len);

typedef struct {
	char *buffer;
	int pos;
	int eot;
} StrReader;

void line_reader_init(StrReader *r, char *buffer);
char line_reader_get(StrReader *r);
void line_reader_back(StrReader *r);

typedef struct {
	int type;
	char content[256];
}BasToken;

typedef struct {
	StrReader *reader;
	BasToken before;
	BasToken last;
} BasTokenizer;

void bas_token_init(BasTokenizer *bt, StrReader *reader);
BasToken *bas_token_get(BasTokenizer *bt);

int main(int argc, char **argv)
{	
	FILE *fp;
	unsigned long counter;
	char buffer[MLEN+1];
	StrReader reader;
	BasTokenizer tokenizer;

	if (argc<2)
		return 0;
	fp = fopen(argv[1],"r");
	if(!fp)
	{
		return 1;
	}

	while(read_line(fp,buffer,MLEN))
	{
		char c;
		BasToken *token;
		line_reader_init(&reader, buffer);
		bas_token_init(&tokenizer, &reader);
		while( (token=bas_token_get(&tokenizer)) ) {
			printf("%d -> %s\n", token->type, token->content);
		}
	}
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

void line_reader_init(StrReader *r, char *buffer)
{
	r->buffer = buffer;
	r->pos = 0;
	r->eot = 0;
}

char line_reader_get(StrReader *r)
{
	if(r->buffer[r->pos]!= '\0')
	{
		return r->buffer[r->pos++];
	}
	return '\0';
}

void line_reader_back(StrReader *r)
{
	if(r->pos>0)
		r->pos--;
}

void bas_token_init(BasTokenizer *bt, StrReader *reader)
{
	bt->reader=reader;
	bt->last.type = TOKEN_NONE;
	bt->last.content[0]='\0';
}

BasToken *bas_token_string(BasTokenizer *bt)
{
	char c;
	int i = 0;
	c = line_reader_get(bt->reader);
	while(c!='\0' && c!='\n') {
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
		c = line_reader_get(bt->reader);
	}
	bt->last.type = TOKEN_ERROR;
	return &bt->last;
}

BasToken *bas_token_name(BasTokenizer *bt, char c)
{
	bt->last.type = TOKEN_NAME;
	int i = 0;
	bt->last.content[i++]=c;
	c = line_reader_get(bt->reader);
	while(isalnum(c) || c=='_') {
		bt->last.content[i++]=c;
		c = line_reader_get(bt->reader);
	}
	bt->last.content[i]='\0';
	return &bt->last;
}

BasToken *bas_token_number(BasTokenizer *bt, char c)
{
	int i = 0;
	bt->last.type = TOKEN_NUM;
	bt->last.content[i++]=c;

	if (c=='.')
		goto decimal_part;

	c = line_reader_get(bt->reader);
	while(isdigit(c)) {
		bt->last.content[i++]=c;
		c = line_reader_get(bt->reader);
	}

	if (c=='e' || c=='E') {
		bt->last.content[i++]=c;
		c = line_reader_get(bt->reader);
		printf("jump to expoent");
		goto expoent;
	}

	if (c!='.')
		goto end_number;
	bt->last.content[i++]='.';

decimal_part:
	c = line_reader_get(bt->reader);
	while(isdigit(c)) {
		bt->last.content[i++]=c;
		c = line_reader_get(bt->reader);
	}
	if (c!='e' && c!='E') {
		goto end_number;
	}
	c = line_reader_get(bt->reader);
expoent:
	if(c=='-' || c=='+' || isdigit(c)) {
		bt->last.content[i++]=c;
		c = line_reader_get(bt->reader);
	} else {
		goto end_number;
	}

	while(isdigit(c)) {
		bt->last.content[i++]=c;
		c = line_reader_get(bt->reader);
	}

end_number:
//	if (c!='\0')
//		line_reader_back(bt->reader);
	bt->last.content[i]='\0';
	return &bt->last;
}

BasToken *bas_token_get(BasTokenizer *bt)
{
	char c;

	bt->before = bt->last;
	c=line_reader_get(bt->reader);
	while (isblank(c)) 
		c=line_reader_get(bt->reader);
	switch(c) {
		case '\0':
		case '\n':
	       		return NULL;	       
		case '?':
		case ':':
		case '\'':
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
			bt->last.type = TOKEN_OP;
			bt->last.content[0]=c;
			bt->last.content[1]='\0';
		return &bt->last;
		case '"': 
			return bas_token_string(bt);
		default:
		if (isalpha(c) || c=='_') {
			return bas_token_name(bt, c);
		}

		if (isdigit(c) || c == '.') {
			return bas_token_number(bt, c);
		}
	}

	return NULL;
}

