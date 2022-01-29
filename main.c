#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mpc.h"
#include <editline/readline.h>

long eval(mpc_ast_t* t);
long eval_op(long x, char* op, long y);

int main(int argc, char *argv[])
{
  printf("Lispy version 0.0.0.0.1\n");
  printf("Press Ctrl-c para sair\n");
  mpc_result_t r;
	
  mpc_parser_t* Numero = mpc_new("numero");
  mpc_parser_t* Operador = mpc_new("operador");
  mpc_parser_t* Expr = mpc_new("expr");
  mpc_parser_t* Lispy = mpc_new("lispy");

  mpca_lang(MPCA_LANG_DEFAULT,
			" numero   : /-?[0-9]+/ ; "
			" operador : '+' | '-' | '*' | '/' | '%' | '^' ; "
			" expr     : <numero> | '(' <operador> <expr>+ ')' ; "
			" lispy    :  /^/ <operador> <expr>+ /$/ ;  ",
			Numero, Operador, Expr, Lispy, NULL);
  while (1) {
	mpc_ast_t* a;
	char* input = readline("lispy> ");

	add_history(input);
	
	if (mpc_parse("<stdin>", input, Lispy, &r)) {
	  /* load from ast output */
	  a = r.output;
	  long resultado = eval(a);
	  printf("%li\n", resultado);
	  mpc_ast_delete(a);
	} else {
	  mpc_err_print(r.error);
	  mpc_err_delete(r.error);
	}
	free(input);
  }
  mpc_cleanup(4, Numero, Operador, Expr, Lispy);
  return 0;
}

long eval_op(long x, char* op, long y) {
  if (strcmp(op, "+") == 0) { return x + y; }
  if (strcmp(op, "-") == 0) { return x - y; }
  if (strcmp(op, "*") == 0) { return x * y; }
  if (strcmp(op, "/") == 0) { return x / y; }
  if (strcmp(op, "%") == 0) { return x % y; }
  if (strcmp(op, "^") == 0) { return pow(x, y); }
  return 0;
}

long eval(mpc_ast_t* q) {
  long x = 0;
  int i = 0;
  int max = q->children_num;
  mpc_ast_t* t = q;
  /* se for o primeiro com a tag > */
  /* itera até achar a tag operador */
  if(strstr(q->tag, ">")) {
	/* acho que trocar isso daqui pra um do while */
	while (!strstr(t->tag, "operador")) {
	  t = q->children[i];
	  i++;
	  if (i == max)
		break;
	}
  }
  if(strstr(t->tag, "numero")) {
	return atoi(t->contents);
  }
  char* op = t->contents;
  x = eval(q->children[i]);

  int j = i+1;
  while (strstr(q->children[j]->tag, "expr")) {
	x = eval_op(x, op, eval(q->children[j]));
	j++;
  }
  return x;
}
