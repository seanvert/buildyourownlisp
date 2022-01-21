#include <stdio.h>
#include <stdlib.h>

#include <editline/readline.h>

static char input[2048];

int main(int argc, char *argv[])
{
  printf("Lispy version 0.0.0.0.1");
  printf("Press Ctrl-c para sair\n");
  while (1) {
	char* input = readline("lispy> ");

	add_history(input);

	printf("no you are an %s\n", input);

	free(input);
  }
  return 0;
}

