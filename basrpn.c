#include "basic.h"

#define MAXTOKENS 100

size_t rpn_contents[MAXTOKENS];
size_t wstk_contents[MAXTOKENS];
size_t fstk_contents[MAXTOKENS];

RpnToken tokenlist[MAXTOKENS];

IndexQueue rpn ={rpn_contents,0,0},
		   wstk={wstk_contents,0,0},
		   fstk={fstk_contents,0,0};

int BasRpnExpression(BasExpression *e)
{
	int c;
	int r = BasToRpnToken(e->list, tokenlist, MAXTOKENS);
	if (r<1)
		return 0;
	e->tlist = (RpnToken *)malloc(sizeof(RpnToken)*(r+2));
	if (!e->tlist)
		return -4;
	e->size = r;
	memcpy(e->tlist, tokenlist, sizeof(RpnToken)*r);

//    dump_rpn_tokens(e->tlist, e->size);

	c = reverse_polish_notation(e->tlist,e->size,&rpn,&wstk,&fstk);
	if (c < 1)
		return 0;
	e->istk.contents = (size_t *)malloc(sizeof(size_t)*(c+2));
	if (!e->istk.contents)
		return -4;
	e->istk.size = c;
	e->istk.max  = c;
	memcpy(e->istk.contents, rpn.contents, sizeof(size_t)*c);

//    dump_rpn(&e->istk, e->tlist);

	return 0;
}

int BasToRpnToken(BasTokenItem *input, RpnToken *tokenlist, int max)
{
	int c=0;
	while (input) {
		if (c==max)
			return -1;
		tokenlist[c].type = input->token.type;
		tokenlist[c].size = strlen(input->token.content);
		tokenlist[c].content = input->token.content;
		tokenlist[c].extra = 0;
		c++;
		input = input->next;
	}
	return c;
}

void dump_rpn_tokens(RpnToken *tokenlist, int count )
{
  int i;
  int c;
  printf("Tokens read: %u\n", count);
  for(i = 0; i < count; i++ ) {
    switch( tokenlist[i].type ) {
      case tokenNone:
        printf("Type: None ");
        break;
      case tokenString:
        printf("Type: String ");
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
    printf("Content: ");
    for(c=0; c < tokenlist[i].size; c++) {
      printf("%c", tokenlist[i].content[c]);
    }
    printf("\n");
  }

}

void dump_rpn(IndexQueue *rpn, RpnToken *tokenlist)
{
  int j;
  int c;
  for(j = 0; j < rpn->size; j++ ) {
    int i = rpn->contents[j];
    switch( tokenlist[i].type ) {
      case tokenNone:
        printf("Type: None ");
        break;
      case tokenNumber:
        printf("Type: Number ");
        break;
      case tokenString:
	printf("Type: String ");
	break;
      case tokenVariable:
        printf("Type: Variable ");
        break;
      case tokenOperator:
        printf("Type: Operator ");
        break;
      case tokenFunction:
        printf("Type: Function %u", tokenlist[i].type);
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

    printf("Content: ");
    for(c=0; c < tokenlist[i].size; c++) {
      printf("%c", tokenlist[i].content[c]);
    }
    printf("\n");
    
  }
}

int reverse_polish_notation(RpnToken *tokenlist, int tn, IndexQueue *rpn, IndexQueue *wstk, IndexQueue *fstk)
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
      case tokenString:
      case tokenNumber:
      case tokenVariable:
        iq_push(rpn,i);
        cf = iq_top( fstk );
        if( cf>=0 && tokenlist[cf].type == tokenFunction ) {
        	if (tokenlist[cf].extra==0) tokenlist[cf].extra=1;
        }
        break;
      case tokenFunction:
        cf = iq_top( fstk );
        if( cf>=0 && tokenlist[cf].type == tokenFunction ) {
          if (tokenlist[cf].extra==0) tokenlist[cf].extra=1;
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
          if( (a1 == 'l' && p1 <= p2) || (a1 == 'r' && p1 < p2) ) {
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
	  default:
		break;
    }
  }
  while ( (i = iq_pop(wstk)) >= 0 ) {
    iq_push(rpn,i);
  }
  return rpn->size;
}

char op_p(RpnToken *token)
{
  char p = 0;
  if( token->type != tokenOperator )
    return 0;

  switch( token->content[0] ) {
    case '<':
    case '>':
    case '=':
      p = 1;
    break;
    case '-':
    case '+':
      p = 2;
    break;
	case '%':
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

char op_a(RpnToken *token)
{
  char a = 'l';
  if( token->type != tokenOperator )
    return 0;

  switch( token->content[0] ) {
    case '^':
      a = 'r';
    break;
//    case '=':
//      a = 'r';
  }

  return a;
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


