#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include "basic.h"

size_t nvariables=0;
Variable variables[MAXVARS];

int branch = -1;

int basexec_quit()
{
	return branch==-2;
}

Entry eval_rpn(RpnToken *tokenlist, IndexQueue *rpn);

Entry result;

char basoutbuffer[2048];

int (*basPutchar)(int) = putchar;
int (*basGetchar)(void) = getchar;

void basSetPutchar(int (*func)(int)) 
{
	basPutchar = func;
}

void basSetGetchar(int (*func)(void)) 
{
	basGetchar = func;
}


int basPrintf(const char *format, ...)
{
	int cnt;
	va_list argptr;
	char *strp = basoutbuffer;

	va_start(argptr, format);
	cnt = vsprintf(basoutbuffer, format, argptr);
	va_end(argptr);

	while(*strp) {
		basPutchar(*strp++);
	}

	return cnt;
}

int basReadline(char *buffer, size_t len)
{
        size_t count=0;
        for(;;) {
                int ch = basGetchar();
                if(ch==EOF || ch=='\n')
                        break;
                else
                if(count<len)
                        buffer[count++]=ch;
        }
        buffer[count]='\0';
        return count;
}

void exec_expression(BasExpression *expr)
{
	BasTokenItem *ti;
	
	result.type = enone;
	result.t.number = 0;

	if (!expr)
		return;
	
	ti = expr->list;
	if (ti && ti->token.type == tokenString) {
		result.type = estring;
		result.t.vstring = ti->token.content;
		return;
	}

	if (expr->istk.size && expr->istk.contents) {
		result = eval_rpn(expr->tlist, &expr->istk);
	}
}

void exec_for(BasForNode *x, int tab)
{
	double counter;
	int i = define_var(x->counter, sizeof(x->counter));
	
	set_var(i, x->start);

	for (counter=x->start; counter<=x->stop; counter+=x->step) {
		set_var(i, counter);
		basexec(x->block, tab+1);
		if (branch != -1)
			return;
	}
	set_var(i, counter);
}

void exec_if(BasIfNode *x, int tab)
{
	int i=0;
	exec_expression(x->expr);
	if (result.type == enumber) {
		i = result.t.number;
	}

	if (i) {
		basexec(x->then_block, tab+1);
		return;
	}

	if (!x->else_block)
		return;

	basexec(x->else_block, tab+1);
}

void exec_token(BasToken *t) 
{
	switch(t->type)
	{
		case TOKEN_OP:     basPrintf("%s", t->content);   break;
		case TOKEN_NAME:   basPrintf("%s", t->content); break;
		case TOKEN_NUM:    basPrintf("%s", t->content);  break;
		case TOKEN_STRING: basPrintf("\"%s\"",  t->content);  break;
		case tokenFunction: basPrintf("F[%s]", t->content); break;
		default: basPrintf("TOKEN[%s]", t->content);        break;
	}
}

void exec_print(BasPrintNode *x, int tab)
{
	int nl = 1;
	BasExpressionList *l = x->list;

	while(l) {
		nl = 0;
		if (l->next == NULL && (!l->expr || l->expr->size == 0))
		       break;

		exec_expression(l->expr);

	
		if (result.type == estring) {
			basPrintf("%s", result.t.vstring);
		}
		else if(result.type == enumber) {
			basPrintf("%d", result.t.number);
		}
		else if(result.type == echar) {
			basPutchar(result.t.chr);
		}
	
		switch(l->separator) {
			case TOKEN_COMMA:     basPrintf("\t"); break;
			case TOKEN_SEMICOLON: break;
			default: nl = 1;
		}
		l=l->next;
	}
	if (nl)
		basPrintf("\n");
}

Entry cmdArgs[8];
int cmdCount = 0;

void command_args(BasCommandNode *x)
{
	BasExpressionList *l = x->list;
	int i = 0;

	while(l && i<8) {
		exec_expression(l->expr);
		cmdArgs[i] = result;

		l=l->next;
		i++;
	}

	cmdCount = i;
}


TNumber cmdNumber(int i, int d)
{
	if (i<cmdCount) {
		if(cmdArgs[i].type == enumber) {
			return cmdArgs[i].t.number;
		}
	}
	return d;
}

char *cmdString(int i)
{
	if (i<cmdCount) {
		if(cmdArgs[i].type == estring) {
			return cmdArgs[i].t.vstring;
		}
	}

	return NULL;
}



void exec_command(BasCommandNode *x)
{
	command_args(x);

	switch (x->def->cmd)
	{
		case cmdFre:   basPrintf("%u Bytes\n", coreleft()); break;
		case cmdCls:   tt_clear(); break;
		case cmdColor: tt_color(cmdNumber(0,0)); if (cmdCount > 1) tt_bgcolor(cmdNumber(1,0)); break;
		case cmdLocate: tt_locate(cmdNumber(0,0), cmdNumber(1,0)); break;
		case cmdQuit:  branch=-2;  break;
		case cmdList:  basedit_list(cmdNumber(0, 0), cmdNumber(1, -1)); break;
		case cmdRun:   basedit_run(cmdNumber(0, 0)); break;
		case cmdLoad:  basedit_load(cmdString(0)); break;
	}
}

char input_buffer[128];
void exec_input(BasInputNode *x, int tab)
{
	BasVarList *v;	
	if (x->prompt)
		basPrintf("%s", x->prompt);

	v = x->list;
	while(v) {
		int j;
		int r = 0;
		if (basReadline(input_buffer, 127)) {
			r = atoi(input_buffer);
		}

		j = define_var(v->var, strlen(v->var));
		set_var(j, r);

		v = v->next;
	}
}

void exec_assignment(BasAssignmentNode *x, int tab)
{

	int i = define_var(x->var, strlen(x->var));
	exec_expression(x->expr);
	if (result.type==enumber) 
		set_var(i, result.t.number);
	else
	      	set_var(i, 0);	


}


BasNode *find_node(BasNode *node, int line)
{
	while(node) {
		if (node->ln == line)
			return node;

		node = node->next;
	}
	return NULL;
}

void basexec(BasNode *root, int tab)
{
	if (root!=NULL) {
		BasNode *cur = root;

		while(cur) {
			if (kbhit()) {
				int k=getch();
				if (k==0)
					getch();
				if (k==27) {
					branch=0x7FFF;
					break;
				}
			}

			switch(cur->type)
			{
				case BAS_FOR:
					exec_for((BasForNode *)cur->data, tab);
					break;
				case BAS_IF:
					exec_if((BasIfNode *)cur->data, tab);
					break;
				case BAS_INPUT:
					exec_input((BasInputNode *)cur->data, tab);
					break;
				case BAS_PRINT:
					exec_print((BasPrintNode *)cur->data, tab);
					break;
				case BAS_ASSIGNMENT:
					exec_assignment((BasAssignmentNode *)cur->data, tab);
					break;
				case BAS_COMMAND:
					exec_command((BasCommandNode *)cur->data);
					break;
				case BAS_GOTO:
					branch = ((BasGotoNode *)cur->data)->goline;
					break;
			}

			if (branch != -1) {
				cur = find_node(root, branch);
				if (!cur)
					return;
				branch = -1;
			}
			else {
				cur = cur->next;
			}
		}
	}
}

/**************************************************************/

TNumber aton(const char *src)
{
  int nf = 0;
  int s  = 0;
  int ac = 0;
  int t;
  while(*src && nf>=0) {
    switch(*src)
    {
      case '-': if(!nf) {
                  s=1;
                  nf=1;
                }
                else {
                  nf=-1;
                }
                break;
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
        if(nf==2)
          nf=-1;
        else
          nf=1;
        t=ac*10+(*src)-'0';
        if( t >= 0 && t < 0x7FFF ) {
          ac = t;
        } 
      break;
      case ' ': if(nf) nf=2;
    }
    src++;
  }
  if(nf<0)
    return 0;
  if(s)
    ac=-ac;
  return ac;
}

TNumber antof(const char *src, size_t n)
{
  char buffer[20];
  strncpy(buffer,src,n);
  buffer[n] = '\0';
  return aton(buffer);
}


int sp = 0; 
Entry stack[MAXTOKENS];

Entry call_function(RpnToken *token, Entry *args, size_t f, size_t n)
{
  char buffer[20];
  TNumber v1 = 0;
  TNumber v2 = 0;

  Entry re;
  re.type = enumber;
  re.t.number = 0;
  strncpy(buffer,token->content,token->size);
  buffer[token->size] = '\0';

  if (n>0) {
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
     //sin(v1);
  }
  else if(!strcmp(buffer,"cos") ) {
  
  }
  else if(!strcmp(buffer,"tan") ) {
    
  }
  else if(!strcmp(buffer,"pi") ) {
    re.t.number = 3.14;
  }
  else if(!strcmp(buffer, "chr")) {
	re.type = echar;
	re.t.chr = v1;  
  }
  else if(!strcmp(buffer, "rnd")) {
	re.type = enumber;
	re.t.number = v1 != 0 ? rand()%v1 : rand();  
  }
  return re;
}

TNumber operation(RpnToken *token, Entry *e1, Entry *e2)
{
  TNumber r = 0;
  TNumber v1 = 0;
  TNumber v2 = 0;

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
    case '=': r = v1==v2; break;
    case '>': r = v1>v2; break;
    case '<': r = v1<v2; break;

    case '%': r = v1%v2; break;
    //case '^': r = pow(v1,v2); break;
  }

  //printf("DEBUG: %d %c %d => %d\n",v1,token->content[0],v2,r);
  return r;
}


Entry eval_rpn(RpnToken *tokenlist, IndexQueue *rpn)
{
  int j;
  Entry v1,v2;
  TNumber r;
  Entry re;
  
  sp=0;
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
        re = call_function(&tokenlist[i],stack,sp-tokenlist[i].extra,tokenlist[i].extra);
        sp = sp-tokenlist[i].extra;
		stack[sp] = re;
//        stack[sp].type = enumber;
//        stack[sp].t.number = r;
        sp++;
        //printf("Type: Function (nargs: %u) ", tokenlist[i].extra );
        break;
      default:
        //printf("Type: unknown ");
	break; 
    }
    if (sp<0) {
		re.type = enumber;
		re.t.number = 0;
	    return re;
	}
  }

  re.type = enumber;
  re.t.number = 0;

  if( sp > 0 ) {
    sp--;
    if( stack[sp].type == enumber )
      re.t.number = stack[sp].t.number;
    if( stack[sp].type == evar )
      re.t.number = get_var(stack[sp].t.varp);
	else
	  return stack[sp];
  }
 
  return re;
}


Variable variables[MAXVARS];

size_t define_var(const char *name, size_t n)
{
  char buffer[128];
  int i=0;

  if(n>127) n=127;

  strncpy(buffer,name,n);
  buffer[n] = '\0';

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


TNumber set_var(size_t idx, TNumber value)
{
  if(idx<nvariables) {
    variables[idx].value = value;
    return variables[idx].value;
  }
  return 0.0;
}

TNumber get_var(size_t idx) 
{
  if(idx<nvariables) {
    return variables[idx].value;
  }
  return 0.0;
}
