#ifndef _BASIC_H
	#define _BASIC_H

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

#ifdef GRAPHICS_SDL
	#include <strings.h>	/* strcasecmp; isblank comes from ctype.h */
#else
	#define strcasecmp strcmpi
	#define isblank(c) ((c)==' ' || (c)=='\t')
#endif

typedef int TNumber;

#define QUEUEMAX 100
#define MAXTOKENS 100

typedef struct {
    int ndim;
    enum {anone, anumber, astring, achar} type;
    int  *dims;
    void *data;
} TArray;

typedef struct {
  enum { enone, evar, enumber, estring, echar, earray } type;
  union {
    TNumber number;
	int chr;
    int varp;
    char *vstring;
    TArray *array;
  } t;
} Entry;


typedef struct {
  char name[128];
  Entry value;
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
	tokenError,
    operatorUnary
} TokenType;

typedef struct {
  TokenType type;
  TokenType subtype;
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
#define OPERATOR_UNARY operatorUnary

#define TOKEN_ERROR tokenError

#define BAS_FOR        1
#define BAS_IF         2
#define BAS_ASSIGNMENT 3
#define BAS_EXPR       4
#define BAS_ELSE       5
#define BAS_PRINT      6
#define BAS_INPUT      7
#define BAS_GOTO       8
#define BAS_COMMAND    9
#define BAS_GOSUB      10
#define BAS_RETURN     11
#define BAS_DIM        12
#define BAS_DIRECT     100

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
    int subtype;
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

typedef struct BasDimVarList {
    char name[MLEN];
    BasExpressionList *dim_list;
    struct BasDimVarList *next;
} BasDimVarList;

typedef struct {
    BasDimVarList *vars;
} BasDimNode;

typedef struct {
	char var[MLEN];
	BasExpression *expr;
} BasAssignmentNode;

typedef struct {
	int goline;
} BasGotoNode;

typedef struct {
	int goline;
} BasGosubNode;

typedef struct {
	char *name;
	enum { 
		cmdNone, cmdFre,  cmdCls,   cmdList, 
		cmdRun,  cmdQuit, cmdColor, cmdLocate,
	    cmdLoad, cmdSave, cmdFiles, cmdExec,
		cmdLine, cmdBox,  cmdCircle, cmdPoint,
        cmdImage, cmdSprLoad, cmdSprUnload,
        cmdSprShow, cmdSprHide, cmdSprMove,
        cmdOverscan,
    } cmd;
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

void prepare_vars();
int predefine_var(const char *name, size_t n);
int define_var(const char *name, size_t n);
Entry set_var(int idx, Entry value);
Entry get_var(int idx);
TNumber set_number(int idx, TNumber value);
TNumber get_number(int idx);

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
void basSetPutcharRaw(int (*func)(int)); 
void basSetGetchar(int (*func)(void));

int basPrintf(const char *format, ...);
extern int (*basGetchar)(void);

int basReadline(char *buffer, size_t len);

void basedit_loop();
void basedit_list(int start, int end);
int basedit_run(int start);
int basedit_load(char *filename);
int basedit_save(char *filename);

void basedit_files(char *path, char *mask);
void dump(BasNode *, int);

#endif
