#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

typedef enum {tokenNone,tokenNumber, tokenVariable, tokenOperator, tokenFunction, tokenComma, tokenLeft, tokenRight} TokenType;

typedef struct {
  TokenType type;
  const char *content;
  unsigned char extra;
  size_t size;
} Token;

#define QUEUEMAX 100

typedef struct {
  size_t contents[QUEUEMAX];
  size_t size;
  size_t max;
} IndexQueue;

typedef struct {
  enum { evar, enumber } type;
  union {
    double number;
    int varp;
  } t;
} Entry;


typedef struct {
  char name[128];
  double value;
} Variable;

#define MAXVARS 100
size_t nvariables = 0;
Variable variables[MAXVARS];

size_t define_var(const char *name, size_t n);
double set_var(size_t idx, double value);
double get_var(size_t idx);

void iq_init(IndexQueue *iq, size_t max);
void iq_push(IndexQueue *iq, size_t idx);
int  iq_top(IndexQueue *iq);
int  iq_pop(IndexQueue *iq);

int tokenize( const char *input, Token *tokenlist, int max );
int reverse_polish_notation(Token *tokenlist, int tn, IndexQueue *rpn, IndexQueue *wstk, IndexQueue *fstk);
double eval_rpn(Token *tokenlist, IndexQueue *rpn);
void dump_tokens(Token *tokenlist, int count );
void dump_rpn(IndexQueue *rpn, Token *tokenlist);
char op_p(Token *token);
char op_a(Token *token);


#define MAXTOKENS 100

Token tokenlist[MAXTOKENS];

int read_line(char *buffer, int len) {
  int c=0;
  int ch;
  while( (ch=getchar()) != EOF && ch != '\n') {
    if(c<len) {
      buffer[c++] = ch;
    }
  }
  buffer[c] = '\0';
  return c;
}

char buffer[128];

IndexQueue rpn,wstk,fstk;

main(int argc, char **argv)
{
  int i,c;

  while( read_line(buffer,127) ) {
    i = tokenize(buffer,tokenlist, MAXTOKENS);
    dump_tokens(tokenlist, i);
    c = reverse_polish_notation(tokenlist,i,&rpn,&wstk,&fstk);
    printf("IndexedLength: %d\n",c);
    dump_rpn(&rpn,tokenlist);
    printf("===========================================\n");
    printf("Result: %f\n", eval_rpn(tokenlist,&rpn) );
    printf("===========================================\n");
    for(i = 0; i < nvariables; i++ ) {
      printf("%s => %f\n", variables[i].name, variables[i].value );
    }
    printf("===========================================\n");
  }
  return 0;
}



int tokenize( const char *input, Token *tokenlist, int max )
{
  int count = 0;
  int pos = 0;
  tokenlist[count].type = tokenNone;
  tokenlist[count].size = 0;
  while(input[pos] && count < max)
  {
    char ch=input[pos];
    if( ch >= '0' && ch <='9' || ch=='.' ) {
      if( tokenlist[count].type == tokenNumber || tokenlist[count].type == tokenVariable ) {
        tokenlist[count].size++;
      }
      else {
       if( tokenlist[count].type != tokenNone )
        count++;
       tokenlist[count].type = tokenNumber;
       tokenlist[count].content = &input[pos];
       tokenlist[count].size    = 1;
      }
    }
    else if(ch == ',') {
      if( tokenlist[count].type != tokenNone )
        count++;
      tokenlist[count].type = tokenComma;
      tokenlist[count].content = &input[pos];
      tokenlist[count].size    = 1;
    }
    else if(ch == '(') {
      if( tokenlist[count].type == tokenVariable )
        tokenlist[count].type = tokenFunction;
      if( tokenlist[count].type != tokenNone )
        count++;
      tokenlist[count].type = tokenLeft;
      tokenlist[count].content = &input[pos];
      tokenlist[count].size    = 1;
    }
    else if(ch == ')') {
      if( tokenlist[count].type != tokenNone )
        count++;
      tokenlist[count].type = tokenRight;
      tokenlist[count].content = &input[pos];
      tokenlist[count].size    = 1;
    }
    else if( ch == '+' || ch == '-' || ch == '*' || ch == '^' || ch == '/' || ch == '=' ) {
      if( tokenlist[count].type != tokenNone )
        count++;
      tokenlist[count].type = tokenOperator;
      tokenlist[count].content = &input[pos];
      tokenlist[count].size    = 1;
    }
    else if( ch >= 'a' && ch <= 'z' || ch >= 'A' && ch <= 'Z' || ch == '_' ) {
      if( tokenlist[count].type == tokenVariable ) {
        tokenlist[count].size++;
      }
      else {
       if( tokenlist[count].type != tokenNone )
        count++;
       tokenlist[count].type = tokenVariable;
       tokenlist[count].content = &input[pos];
       tokenlist[count].size    = 1;
      }
    }
    else {
      if( tokenlist[count].type != tokenNone )
        count++;
      tokenlist[count].type = tokenNone;
    }
    pos++;
  }
  if( tokenlist[count].type != tokenNone )
   count++;
  return count <= max ? count : max;
}


void dump_rpn(IndexQueue *rpn, Token *tokenlist)
{
  int j;
  for(j = 0; j < rpn->size; j++ ) {
    int i = rpn->contents[j];
    switch( tokenlist[i].type ) {
      case tokenNone:
        printf("Type: None ");
        break;
      case tokenNumber:
        printf("Type: Number ");
        break;
      case tokenVariable:
        printf("Type: Variable ");
        break;
      case tokenOperator:
        printf("Type: Operator ");
        break;
      case tokenFunction:
        printf("Type: Function ");
        break;
      case tokenComma:
        printf("Type: Comma ");
        break;
      case tokenLeft: 
        printf("Type: Left ");
        break;
      case tokenRight:
        printf("Type: Right ");
        break;
      default:
        printf("Type: unknown ");
    }
    int c;
    printf("Content: ");
    for(c=0; c < tokenlist[i].size; c++) {
      printf("%c", tokenlist[i].content[c]);
    }
    printf("\n");
    
  }
}

void dump_tokens(Token *tokenlist, int count )
{
  int i;
  printf("Tokens read: %u\n", count);
  for(i = 0; i < count; i++ ) {
    switch( tokenlist[i].type ) {
      case tokenNone:
        printf("Type: None ");
        break;
      case tokenNumber:
        printf("Type: Number ");
        break;
      case tokenVariable:
        printf("Type: Variable ");
        break;
      case tokenOperator:
        printf("Type: Operator ");
        break;
      case tokenFunction:
        printf("Type: Function ");
        break;
      case tokenComma:
        printf("Type: Comma ");
        break;
      case tokenLeft: 
        printf("Type: Left ");
        break;
      case tokenRight:
        printf("Type: Right ");
        break;
      default:
        printf("Type: unknown ");
    }
    int c;
    printf("Content: ");
    for(c=0; c < tokenlist[i].size; c++) {
      printf("%c", tokenlist[i].content[c]);
    }
    printf("\n");
  }

}



/*Stack*/
void iq_init(IndexQueue *iq, size_t max)
{
  iq->max = max;
  iq->size = 0;
}

void iq_push(IndexQueue *iq, size_t idx)
{
  iq->contents[iq->size++]=idx;
}

int iq_top(IndexQueue *iq)
{
  return iq->size > 0 ? iq->contents[iq->size-1] : -1; 
}

int iq_pop(IndexQueue *iq)
{
  return iq->size > 0 ? iq->contents[--iq->size] : -1; 
}



int reverse_polish_notation(Token *tokenlist, int tn, IndexQueue *rpn, IndexQueue *wstk, IndexQueue *fstk)
{
  int i;
  int top;
  char p1,p2,a1;
  int cf;
  iq_init(rpn, tn);
  iq_init(wstk,tn);
  iq_init(fstk,tn);
  for( i = 0; i< tn; i ++)
  {
    switch(tokenlist[i].type)
    {
      case tokenNumber:
      case tokenVariable:
        iq_push(rpn,i);
        cf = iq_top( fstk );
        if( cf>=0 && tokenlist[cf].type == tokenFunction ) {
		  if (tokenlist[cf].extra==0)
          		tokenlist[cf].extra=1;
        }
        break;
      case tokenFunction:
        cf = iq_top( fstk );
        if( cf>=0 && tokenlist[cf].type == tokenFunction ) {
		  if (tokenlist[cf].extra==0)
          		tokenlist[cf].extra=1;
        }
        tokenlist[i].extra = 0;
        iq_push(fstk,i);
        iq_push(wstk,i);
      break;
      case tokenComma:
        cf = iq_top( fstk );
        if( cf>=0 && tokenlist[cf].type == tokenFunction ) {
          	tokenlist[cf].extra++;
        }

        top = iq_top(wstk);
        while( top >= 0 && tokenlist[top].type != tokenLeft ) {
          if( tokenlist[top].type == tokenFunction ) {
            iq_pop(fstk);
          }
          iq_push( rpn, iq_pop(wstk) );
          top = iq_top(wstk);
        }
        break;
      case tokenOperator:
        p1 = op_p( &tokenlist[i] );
        a1 = op_a( &tokenlist[i] );
        top = iq_top(wstk);
        
        while( top >= 0 && tokenlist[top].type == tokenOperator ) {
          p2 = op_p( &tokenlist[top] );
          //char a2 = op_a( tokenlist[top] );
          if( a1 == 'l' && p1 <= p2 || a1 == 'r' && p1 < p2 ) {
            iq_push(rpn, iq_pop(wstk) );
          }
          else {
            break;
          }
          top = iq_top(wstk);
        } // while
        iq_push(wstk, i);
        break;
      case tokenLeft:
        iq_push(wstk, i);
        break;
      case tokenRight:
        top = iq_top(wstk);
        while( top >= 0 && tokenlist[top].type != tokenLeft ) {
          if( tokenlist[top].type == tokenFunction ) {
            iq_pop(fstk);
          }
          iq_push(rpn, iq_pop(wstk) );
          top = iq_top(wstk);
        } // while

        if( top >= 0 && tokenlist[top].type == tokenLeft ) {
          iq_pop( wstk );
          top = iq_top( wstk );
        } else {
          return -1;
        }

        if( top >=0 && tokenlist[top].type == tokenFunction ) {
          iq_pop(fstk);
          iq_push( rpn, iq_pop(wstk) );
        }
        break;
    }
  }
  while ( (i = iq_pop(wstk)) >= 0 ) {
    iq_push(rpn,i);
  }
  return rpn->size;
}

char op_p(Token *token)
{
  if( token->type != tokenOperator )
    return 0;
  char p = 0;
  switch( token->content[0] ) {
    case '=':
      p = 1;
    break;
    case '-':
    case '+':
      p = 2;
    break;
    case '*':
    case '/':
      p = 3;
    break;
    case '^':
      p = 4;
    break;
  }
  return p;
}

char op_a(Token *token)
{
  if( token->type != tokenOperator )
    return 0;
  char a = 'l';
  switch( token->content[0] ) {
    case '^':
      a = 'r';
    break;
    case '=':
      a = 'r';
  }

  return a;
}

double antof(const char *src, size_t n)
{
  char buffer[20];
  strncpy(buffer,src,n);
  buffer[n] = '\0';
  return atof(buffer);
}


size_t sp = 0; 
Entry stack[MAXTOKENS];

double call_function(Token *token, Entry *args, size_t f, size_t n)
{
  char buffer[20];
  double v1 = 0;
  double v2 = 0;

  strncpy(buffer,token->content,token->size);
  buffer[token->size] = '\0';

  if(n>0) {
    if( args[f].type == enumber ) {
      v1 = args[f].t.number;
    }
    else {
      v1 = get_var(args[f].t.varp);
    }
  }

  if(n>1) {
    if( args[f+1].type == enumber ) {
      v2 = args[f+1].t.number;
    }
    else {
      v2 = get_var(args[f+1].t.varp);
    }
  }

  if(!strcmp(buffer,"sin") ) {
    return sin(v1);
  }
  else if(!strcmp(buffer,"cos") ) {
    return cos(v1);
  }
  else if(!strcmp(buffer,"tan") ) {
    return tan(v1);
  }
  else if(!strcmp(buffer,"pi") ) {
    return 3.14;
  }
  return 0.0;
}

double operation(Token *token, Entry *e1, Entry *e2)
{
  double r = 0;
  double v1 = 0;
  double v2 = 0;

  if( e1->type == enumber ) {
    v1 = e1->t.number;
  }
  else {
    v1 = get_var(e1->t.varp);
  }

  if( e2->type == enumber ) {
    v2 = e2->t.number;
  }
  else {
    v2 = get_var(e2->t.varp);
  }

  switch(token->content[0])
  {
    case '+': r = v1+v2; break;
    case '-': r = v1-v2; break;
    case '*': r = v1*v2; break;
    case '/': r = v1/v2; break;
    case '=': r = v2;
      if( e1->type == evar ) {
        set_var(e1->t.varp, r);
        v1 = get_var(e1->t.varp);
      }
    break;
    case '%': r = fmod(v1,v2); break;
    case '^': r = pow(v1,v2); break;
  }

  printf("DEBUG: %f %c %f => %f\n",v1,token->content[0],v2,r);
  return r;
}


double eval_rpn(Token *tokenlist, IndexQueue *rpn)
{
  int j;
  Entry v1,v2;
  double r;
  for(j = 0; j < rpn->size; j++ ) {
    int i = rpn->contents[j];
    switch( tokenlist[i].type ) {
      case tokenNumber:
        stack[sp].type = enumber;
        stack[sp].t.number = antof(tokenlist[i].content,tokenlist[i].size);
        sp++;
        break;
      case tokenVariable:
        stack[sp].type = evar;
        stack[sp].t.varp = define_var(tokenlist[i].content,tokenlist[i].size);
        sp++;
        break;
      case tokenOperator:
        v2 = stack[--sp];
        v1 = stack[--sp];
        stack[sp].type = enumber;
        stack[sp].t.number = operation(&tokenlist[i],&v1,&v2);
        sp++;
        break;
      case tokenFunction:
        r = call_function(&tokenlist[i],stack,sp-tokenlist[i].extra,tokenlist[i].extra);
        sp = sp-tokenlist[i].extra;
        stack[sp].type = enumber;
        stack[sp].t.number = r;
        sp++;
        printf("Type: Function (nargs: %u) ", tokenlist[i].extra );
        break;
      default:
        printf("Type: unknown "); 
    }
	printf("SP %u\n", sp);
  }

  if( sp > 0 ) {
    sp--;
    if( stack[sp].type == enumber )
      return stack[sp].t.number;
    else
      return 0;
  }
  return 0;
}


size_t define_var(const char *name, size_t n)
{
  char buffer[128];

  if(n>127) n=127;

  strncpy(buffer,name,n);
  buffer[n] = '\0';
  int i=0;
  for(i=0;i<nvariables;i++)
  {
    if( !strcmp(buffer,variables[i].name) ) {
      return i;
    }
  }
  strcpy(variables[i].name,buffer);
  variables[i].value = 0;
  nvariables++;
  return i;
}


double set_var(size_t idx, double value)
{
  if(idx<nvariables) {
    variables[idx].value = value;
    return variables[idx].value;
  }
  return 0.0;
}

double get_var(size_t idx) 
{
  if(idx<nvariables) {
    return variables[idx].value;
  }
  return 0.0;
}
