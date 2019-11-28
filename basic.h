#ifndef _BASIC_H
	#define _BASIC_H

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

#define strcasecmp strcmpi
#define isblank(c) ((c)==' ' || (c)=='\t')

typedef int TNumber;

#define QUEUEMAX 100
#define MAXTOKENS 100

typedef struct {
  enum { enone, evar, enumber, estring, echar } type;
  union {
    TNumber number;
	int chr;
    int varp;
    char *vstring;
  } t;
} Entry;


typedef struct {
  char name[128];
  TNumber value;
} Variable;


typedef struct {
  size_t *contents;
  size_t size;
  size_t max;
} IndexQueue;

typedef enum {
	tokenNone,
	tokenNumber,
	tokenVariable,
	tokenOperator,
	tokenFunction,
	tokenComma, 
	tokenLeft, 
	tokenRight,
	tokenString,
	tokenLine,
	tokenComment,
	tokenSemicolon,
	tokenUnknown,
	tokenError
} TokenType;

typedef struct {
  TokenType type;
  const char *content;
  unsigned char extra;
  size_t size;
} RpnToken;


#define MLEN 255

#define TOKEN_NONE tokenNone
#define TOKEN_EOL  tokenEndOfLine
#define TOKEN_OP   tokenOperator
#define TOKEN_NUM  tokenNumber
#define TOKEN_NAME tokenVariable
#define TOKEN_STRING  tokenString
#define TOKEN_LINE    tokenLine
#define TOKEN_UNKNOWN tokenUnknown
#define TOKEN_COMMENT tokenComment
#define TOKEN_COMMA   tokenComma
#define TOKEN_SEMICOLON tokenSemicolon
#define TOKEN_FUNCTION  tokenFunction

#define TOKEN_ERROR tokenError

#define BAS_FOR 1
#define BAS_IF  2
#define BAS_ASSIGNMENT 3
#define BAS_EXPR 4
#define BAS_ELSE 5
#define BAS_PRINT 6
#define BAS_INPUT 7
#define BAS_GOTO  8
#define BAS_COMMAND 9
#define BAS_DIRECT 100

#define BAS_STORE  101

#define BAS_EXPR_ASSIGN    0
#define BAS_EXPR_PRINT     1
#define BAS_EXPR_INPUT     2
#define BAS_EXPR_IF        3
#define BAS_EXPR_FOR_START 4
#define BAS_EXPR_FOR_TO    5
#define BAS_EXPR_FOR_STEP  6

int read_line(FILE *fp, char *buffer, size_t len);

#define MAXBACK 10

typedef struct {
	int (*read_func)(void *data);
	void *reader_data;
	char buffer[MAXBACK];
	int pos;
} StreamReader;

void stream_reader_init(StreamReader *r, int (*read_func)(void *data), void *reader_data);
int  stream_reader_get(StreamReader *r);
void stream_reader_unget(StreamReader *r, char c);

typedef struct {
	int type;
	char content[256];
}BasToken;

typedef struct {
	StreamReader *reader;
	BasToken before;
	BasToken last;
	int on_hold;
} BasTokenizer;

void bas_token_init(BasTokenizer *bt, StreamReader *reader);
BasToken *bas_token_get(BasTokenizer *bt);
void bas_token_back(BasTokenizer *bt);
BasToken *bas_token_expect(BasTokenizer *bt, int type);
BasToken *bas_token_expect_name(BasTokenizer *bt, char *name);
BasToken *bas_token_expect_operator(BasTokenizer *bt, char *op);

typedef struct BasNode {
	int type;
	int ln;
	void *data;
	struct BasNode *next;
} BasNode;

typedef struct BasTokenItem {
	BasToken token;
	struct BasTokenItem *next;
}BasTokenItem;

typedef struct {
	BasTokenItem *list;
	IndexQueue    istk;
	int           size;
	RpnToken     *tlist;
} BasExpression;

typedef struct BasExpressionList {
	BasExpression *expr;
	int separator;
	struct BasExpressionList *next;
} BasExpressionList;

typedef struct {
	char counter[MLEN];
	double start;
	double stop;
	double step;
	BasNode *block;
}BasForNode;

typedef struct {
	BasExpression *expr;
	BasNode *then_block;
	BasNode *else_block;
} BasIfNode;

typedef struct {
	BasExpressionList *list;
} BasPrintNode;

typedef struct BasVarList {
	char var[MLEN];
	struct BasVarList *next;
} BasVarList;

typedef struct {
	char prompt[256];
	BasVarList *list;
} BasInputNode;

typedef struct {
	char var[MLEN];
	BasExpression *expr;
} BasAssignmentNode;

typedef struct {
	int goline;
} BasGotoNode;

typedef struct {
	char *name;
	enum { cmdNone, cmdFre, cmdCls, cmdList, cmdRun, cmdQuit, cmdColor, cmdLocate } cmd;
} BasCommandDef;

typedef struct {
	BasCommandDef *def;
	BasExpressionList *list;
} BasCommandNode;

int BasBuildExpression(BasExpression *e, BasTokenizer *tokenizer, int type);

int BasBlock(BasNode **link, BasTokenizer *tokenizer, int type);

void BasNodeFree(BasNode **link);

#define MAXVARS 100
extern size_t nvariables;
extern Variable variables[MAXVARS];

size_t define_var(const char *name, size_t n);
TNumber set_var(size_t idx, TNumber value);
TNumber get_var(size_t idx);

void iq_init(IndexQueue *iq, size_t max);
void iq_push(IndexQueue *iq, size_t idx);
int  iq_top(IndexQueue *iq);
int  iq_pop(IndexQueue *iq);
char op_p(RpnToken *token);
char op_a(RpnToken *token);

int BasToRpnToken(BasTokenItem *input, RpnToken *tokenlist, int max);
int reverse_polish_notation(RpnToken *tokenlist, int tn, IndexQueue *rpn, IndexQueue *wstk, IndexQueue *fstk);
void dump_rpn_tokens(RpnToken *tokenlist, int count );
void dump_rpn(IndexQueue *rpn, RpnToken *tokenlist);

int BasRpnExpression(BasExpression *e);

void basexec(BasNode *root, int tab);
int basexec_quit();

void basSetPutchar(int (*func)(int)); 

int basPrintf(const char *format, ...);
extern int (*basGetchar)(void);

void basedit_loop();

#endif
