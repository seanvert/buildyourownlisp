#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mpc.h"
#include <editline/readline.h>
/* enum com os possíveis tipos de lval */
enum { LVAL_NUM, LVAL_ERR };
/* enumeração com os possíveis códigos de erro */
enum { LERR_DIV_ZERO, LERR_BAD_OP, LERR_BAD_NUM };
/* estrutura de dados lisp value */
typedef struct {
  int type;
  long num;
  int err;
} lval;
lval eval(mpc_ast_t* t);
lval eval_op(lval x, char* op, lval y);
lval lval_num(long x);
lval lval_err(int x);
void lval_print(lval v);
void lval_println(lval v);

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
	  lval resultado = eval(a);
	  lval_println(resultado);
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

lval eval_op(lval x, char* op, lval y) {
  if (x.type == LVAL_ERR) {
	return x;
  }
  if (y.type == LVAL_ERR) {
	return y;
  }
  if (strcmp(op, "+") == 0)	{
	return lval_num(x.num + y.num);
  }
  if (strcmp(op, "-") == 0)	{
	return lval_num(x.num - y.num);
  }
  if (strcmp(op, "*") == 0)	{
	return lval_num(x.num * y.num);
  }
  if (strcmp(op, "/") == 0)	{
	if (y.num == 0) {
	  return lval_err(LERR_DIV_ZERO);
	} else {
	  return lval_num(x.num / y.num);
	}
  }
  if (strcmp(op, "%") == 0)	{
	return lval_num(x.num % y.num);
  }
  if (strcmp(op, "^") == 0)	{
	return lval_num(pow(x.num, y.num));
  }
  return lval_err(LERR_BAD_OP);
}

lval eval(mpc_ast_t* q) {
  /* TODO essa função está meio bagunçada, depois preciso mexer nela */
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
	/* In this case we use the strtol function to convert from string to long. This allows us to check a special variable errno to ensure the conversion goes correctly. This is a more robust way to convert numbers than our previous method using atoi. */
	errno = 0;
	long x = strtol(t->contents, NULL, 10);
	if (errno != ERANGE) {
	  return lval_num(x);
	} else {
	  lval_err(LERR_BAD_NUM);
	}
  }
  char* op = t->contents;
  lval x = eval(q->children[i]);

  int j = i+1;
  while (strstr(q->children[j]->tag, "expr")) {
	x = eval_op(x, op, eval(q->children[j]));
	j++;
  }
  return x;
}

lval lval_num(long x) {
  lval v;
  v.type = LVAL_NUM;
  v.num = x;
  return v;
}
lval lval_err(int x) {
  lval v;
  v.type = LVAL_ERR;
  v.err = x;
  return v;
}

void lval_print(lval v) {
  switch (v.type) {
  case LVAL_NUM: {
	printf("%li", v.num);
    break;
  }
  case LVAL_ERR: {
    if (v.err == LERR_DIV_ZERO) {
	  printf("Erro: divisão por zero\n");
	}
	if (v.err == LERR_BAD_OP) {
	  printf("Erro: operador inválido\n");
	}
	if (v.err == LERR_BAD_NUM) {
	  printf("Erro: número inválido\n");
	}
    break;
  }
  }
}

/* adiciona uma quebra de linha depois de um lval */
void lval_println(lval v) {
  lval_print(v);
  putchar('\n');
}
