#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mpc.h"
#include <editline/readline.h>
/* enum com os possíveis tipos de lval */
enum { LVAL_NUM, LVAL_ERR, LVAL_SIM, LVAL_SEXPR };
/* enumeração com os possíveis códigos de erro */
enum { LERR_DIV_ZERO, LERR_BAD_OP, LERR_BAD_NUM };
/* estrutura de dados lisp value */
typedef struct {
	int type;
	long num;
	char* err;
	char* sim;
	int contagem;
	struct lval** celula;
} lval;

lval eval(mpc_ast_t* t);
lval eval_op(lval x, char* op, lval y);
lval* lval_num(long x);
lval* lval_err(char* m);
void lval_del(lval* v);
lval* lval_read_num(mpc_ast_t * t);
lval* lval_read(mpc_ast_t* t);
lval* lval_sim(char* s);
lval* lval_add(lval* v, lval* x);
lval* lval_sexpr(void);
void lval_expr_print(lval* v, char open, char close);
lval* lval_eval_sexpr(lval* v);
lval* lval_eval(lval* v);
void lval_print(lval* v);
void lval_println(lval* v);

int main(int argc, char *argv[])
{
	printf("Lispy version 0.0.0.0.1\n");
	printf("Press Ctrl-c para sair\n");
	mpc_result_t r;
	
	mpc_parser_t* Numero = mpc_new("numero");
	mpc_parser_t* Simbolo = mpc_new("simbolo");
	mpc_parser_t* Sexpr = mpc_new("sexpr");
	mpc_parser_t* Expr = mpc_new("expr");
	mpc_parser_t* Lispy = mpc_new("lispy");

	mpca_lang(MPCA_LANG_DEFAULT,
			  " numero   : /-?[0-9]+/ ; "
			  " simbolo  : '+' | '-' | '*' | '/' | '%' | '^' ; "
			  " sexpr    : '(' <expr>* ')' ; "
			  " expr     : <numero> | <simbolo> | <sexpr> ; "
			  " lispy    :  /^/ <expr>* /$/ ;  ",
			  Numero, Simbolo, Sexpr, Expr, Lispy);
	while (1) {
		char* input = readline("lispy> ");

		add_history(input);
	
		if (mpc_parse("<stdin>", input, Lispy, &r)) {
			/* load from ast output */
			lval* x = lval_eval(lval_read(r.output));
			lval_println(x);
			lval_del(x);
			mpc_ast_delete(r.output);
		} else {
			mpc_err_print(r.error);
			mpc_err_delete(r.error);
		}
		free(input);
	}
	mpc_cleanup(5, Numero, Simbolo, Sexpr, Expr, Lispy);
	return 0;
}

lval* lval_pop(lval* v, int i) {
	lval* x = v->celula[i];

	memmove(&v->celula[i], &v->celula[i+1],
			sizeof(lval*) * (v->contagem - i - 1));

	v->contagem--;

	v->celula = realloc(v->celula, sizeof(lval*) * v->contagem);
	return x;
}

lval* lval_take(lval* v, int i) {
  lval* x = lval_pop(v, i);
  lval_del(v);
  return x;
}

lval* builtin_op(lval* a, char* op) {
	for (int i = 0; i < a->contagem; i++) {
	  lval *p = a->celula[i];
		if (p->type != LVAL_NUM) {
			lval_del(a);
			return lval_err("Nào pode ser utilizado sem números!");
		}
	}

	lval *x = lval_pop(a, 0);

	if ((strcmp(op, "-") == 0) && a->contagem == 0) {
		x->num = -x->num;
	}

	while (a->contagem > 0) {
		lval* y = lval_pop(a, 0);
		if (strcmp(op, "+") == 0) {
			x->num += y->num;
		}
		if (strcmp(op, "-") == 0) {
			x->num -= y->num;
		}
		if (strcmp(op, "*") == 0) {
			x->num *= y->num;
		}
		if (strcmp(op, "/") == 0) {
			if(y->num == 0) {
				lval_del(x);
				lval_del(y);
				x = lval_err("Divisão por zero!");
				break;
			}
			x->num /= y->num;
		}
		lval_del(y);
	}
	lval_del(a);
	return x;
}

lval* lval_eval_sexpr(lval* v) {
	for (int i = 0; i < v->contagem; i++) {
		v->celula[i] = lval_eval(v->celula[i]);
	}

	for (int i = 0; i < v->contagem; i++) {
	  lval* p = v->celula[i];
		if (p->type == LVAL_ERR) {
			return lval_take(v, i);
		}
	}

	if (v->contagem == 0) {
		return v;
	}

	if (v->contagem == 1) {
		return lval_take(v,0);
	}

	lval* f = lval_pop(v, 0);
	if (f->type != LVAL_SIM) {
		lval_del(f);
		lval_del(v);
		return lval_err("Expressão S não começa com um símbolo!");
	}

	lval* result = builtin_op(v, f->sim);
	lval_del(f);
	return result;
}

lval* lval_eval(lval* v) {
	if (v->type == LVAL_SEXPR) {
		return lval_eval_sexpr(v);
	}
	return v;
}

lval* lval_num(long x) {
	lval* v = malloc(sizeof(lval));
	v->type = LVAL_NUM;
	v->num = x;
	return v;
}

lval* lval_read_num(mpc_ast_t * t) {
	errno = 0;
	long x = strtol(t->contents, NULL, 10);
	if (errno != ERANGE) {
		return lval_num(x);
	} else {
		lval_err("Número inválido");
	}
}

lval* lval_read(mpc_ast_t* t) {
	if(strstr(t->tag, "numero")) {
		return lval_read_num(t);
	}
	if(strstr(t->tag, "simbolo")) {
		return lval_sim(t->contents);
	}
	lval* x = NULL;
	if(strcmp(t->tag, ">") == 0) {
		x = lval_sexpr();
	}
	if(strstr(t->tag, "sexpr")) {
		x = lval_sexpr();
	}
	for (int i = 0; i < t->children_num; i++) {
		if(strcmp(t->children[i]->contents, "(") == 0) {
			continue;
		}
		if(strcmp(t->children[i]->contents, ")") == 0) {
			continue;
		}
		if(strcmp(t->children[i]->tag, "regex") == 0) {
			continue;
		}
		x = lval_add(x, lval_read(t->children[i]));
	}
	return x;
}

lval* lval_add(lval* v, lval* x) {
	v->contagem++;
	v->celula = realloc(v->celula, sizeof(lval*) * v->contagem);
	v->celula[v->contagem - 1] = x;
	return v;
}

lval* lval_err(char* m) {
	lval* v = malloc(sizeof(lval));
	v->type = LVAL_ERR;
	v->err = malloc(strlen(m) + 1);
	strcpy(v->err, m);
	return v;
}

lval* lval_sim(char* s) {
	lval* v = malloc(sizeof(lval));
	v->type = LVAL_SIM;
	v->sim = malloc(strlen(s) + 1);
	strcpy(v->sim, s);
	return v;
}

lval* lval_sexpr(void) {
	lval* v = malloc(sizeof(lval));
	v->type = LVAL_SEXPR;
	v->contagem = 0;
	v->celula = NULL;
	return v;
}

void lval_expr_print(lval* v, char open, char close) {
	putchar(open);
	for (int i = 0; i < v->contagem; i++) {
		lval_print(v->celula[i]);
		if (i != (v->contagem - 1))  {
			putchar(' ');
		}
	}
	putchar(close);
}

void lval_print(lval* v) {
	switch (v->type) {
	case LVAL_NUM: {
		printf("%li", v->num);
		break;
	}
	case LVAL_ERR: {
		printf("Erro: %s", v->err);
		break;
	}
	case LVAL_SIM: {
		printf("%s", v-> sim);
		break;
	}
	case LVAL_SEXPR: {
		lval_expr_print(v, '(', ')');
		break;
	}
	}
}

/* adiciona uma quebra de linha depois de um lval */
void lval_println(lval* v) {
	lval_print(v);
	putchar('\n');
}

void lval_del(lval* v) {
	switch (v->type) {
	case LVAL_NUM: {
		break;
	}
	case LVAL_ERR: {
		free(v->err);
		break;
	}
	case LVAL_SIM: {
		free(v->sim);
		break;
	}
	case LVAL_SEXPR: {
		for (int i = 0; i < v->contagem; ++i) {
			lval_del(v->celula[i]);
		}
		free(v->celula);
		break;
	}
	}
	free(v);
}
