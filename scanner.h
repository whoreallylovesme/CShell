#include <stdio.h>

int init_scanner(FILE *);

enum TokenKind
{
    T_CONJ,
    T_DISJ,
    T_SEQ,
    T_BACK,
    T_EOF,
    T_NEWLINE,
    T_WORD,
    T_OPEN,
    T_PIPE,
    T_IN,
    T_OUT,
    T_APPEND,
    T_CLOSE
};

struct Tok {
    int kind;
    char *text;
    size_t len;
    size_t capacity;
};

typedef struct Tok Token;

void free_token(Token *);

int next_token(Token *);

