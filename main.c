#include <stdio.h>
#include <stdlib.h>
#include "mpc.h"
#include <editline/readline.h>

long eval(mpc_ast_t* t);
long eval_op(long x, char* op, long y);

int main(int argc, char *argv[])
{
  printf("Lispy version 0.0.0.0.1");
  printf("Press Ctrl-c para sair\n");
  	mpc_result_t r;
	
  mpc_parser_t* Numero = mpc_new("numero");
  mpc_parser_t* Operador = mpc_new("operador");
  mpc_parser_t* Expr = mpc_new("expr");
  mpc_parser_t* Lispy = mpc_new("lispy");

  mpca_lang(MPCA_LANG_DEFAULT,
			" numero   : /-?[0-9]+/ ; "
			" operador : '+' | '-' | '*' | '/' | '%' ; "
			" expr     : <numero> | '(' <operador> <expr>+ ')' ; "
			" lispy    : /^/ '(' <operador> <expr>+ ')' /$/ ;            ",
			Numero, Operador, Expr, Lispy, NULL);
  while (1) {
	mpc_ast_t* a;
	mpc_ast_t* c0;
	char* input = readline("lispy> ");

	add_history(input);
	
	if (mpc_parse("<stdin>", input, Lispy, &r)) {
	  /* load from ast output */
	  a = r.output;
	  printf("Tag: %s\n", a->tag);
	  printf("Contents: %s\n", a->contents);
	  printf("Number of Children: %i\n", a->children_num);

	  c0 = a->children[0];
	  printf("First Children Tag: %s\n", c0->tag);
	  printf("First Children Contents: %s\n", c0->contents);
	  printf("First Children Children_num: %i\n", c0->children_num);
	  mpc_ast_print(r.output);
	  mpc_ast_delete(r.output);
	} else {
	  mpc_err_print(r.error);
	  mpc_err_delete(r.error);
	}
	free(input);
  }
  mpc_cleanup(4, Numero, Operador, Expr, Lispy);
  return 0;
}

long eval(mpc_ast_t* t) {
  if(strstr(t->tag, "numero")) {
	return atoi(t->contents);
  }
  /* TODO não sei se este pedaço vai funcionar direito */
  char* op = t->children[1]->contents;

  long x = eval(t->children[2]);

  int i = 3;
  while (strstr(t->children[i]->tag, "expr")) {
	x = eval_op(x, op, eval(t->children[i]));
	i++;
  }
  return x;
}

long eval_op(long x, char* op, long y) {
  if (strcmp(op, "+") == 0) { return x + y; }
  if (strcmp(op, "-") == 0) { return x - y; }
  if (strcmp(op, "*") == 0) { return x * y; }
  if (strcmp(op, "/") == 0) { return x / y; }
  return 0;
}
