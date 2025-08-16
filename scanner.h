#include <stdio.h>

enum TokenKind 
{
    T_EOF, 
    T_NEWLINE, 
    T_OPEN, 
    T_CLOSE, 
    T_SEQ, 
    T_CONJ, 
    T_BACK, 
    T_DISJ, 
    T_PIPE, 
    T_IN, 
    T_APPEND, 
    T_OUT,
    T_WORD
};
  
typedef struct Token
{
    int kind;
    char *text;
    size_t len;
    size_t capacity;
} Token;



int init_scanner(FILE *f);
void free_scanner(void);
int next_token(Token *token);
void free_token(Token *token);