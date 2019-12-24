#include "../src/parser.c"

int counter(char *, char);

char* getToken(char *, int *);

int parse_simplecommand(char *, simple_command *);

int parse_command(char *, int, command *);
