#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

#include "scanner.h"

static int c;
static FILE *input;

static int append_word(Token *, int);
static int init_token(Token *, enum TokenKind);

enum { INIT_CAPACITY = 16 };

int
init_scanner(FILE *f)
{
    input = f;
    c = fgetc(input);
    return 0;
}

void
free_scanner(void)
{
}

int
next_token(Token *token)
{
    while (!ferror(input) && c != EOF && isspace(c) && c != '\n') {
        c = fgetc(input);
    }
    if (ferror(input)) {
        return -1;
    }
    switch (c) {
    case EOF:
        return init_token(token, T_EOF);
    case '\n':
        // no read!
        return init_token(token, T_NEWLINE);
    case '(':
        c = fgetc(input);
        return init_token(token, T_OPEN);
    case ')':
        c = fgetc(input);
        return init_token(token, T_CLOSE);
    case ';':
        c = fgetc(input);
        return init_token(token, T_SEQ);
    case '&':
        c = fgetc(input);
        if (!ferror(input) && c == '&') {
            c = fgetc(input);
            return init_token(token, T_CONJ);
        } else {
            return init_token(token, T_BACK);
        }
    case '|':
        c = fgetc(input);
        if (!ferror(input) && c == '|') {
            c = fgetc(input);
            return init_token(token, T_DISJ);
        } else {
            return init_token(token, T_PIPE);
        }
    case '<':
        c = fgetc(input);
        return init_token(token, T_IN);
    case '>':
        c = fgetc(input);
        if (!ferror(input) && c == '>') {
            c = fgetc(input);
            return init_token(token, T_APPEND);
        } else {
            return init_token(token, T_OUT);
        }
    default:
        int r;
        if ((r = init_token(token, T_WORD)) != 0) {
            return r;
        }
        if ((r = append_word(token, c)) != 0) {
            free_token(token);
            return r;
        }
        while (c = fgetc(input), !ferror(input) && c != EOF && !isspace(c) && !strchr("&|;()<>\n", c)) {
            if ((r = append_word(token, c)) != 0) {
                free_token(token);
                return r;
            }
        }
        return 0;
    }
}

static int
init_token(Token *token, enum TokenKind kind)
{
    token->kind = kind;
    token->text = 0;
    token->len = 0;
    token->capacity = 0;
    return 0;
}

static int
append_word(Token *token, int ch)
{
    if (token->capacity == 0) {
        token->capacity = INIT_CAPACITY;
        char *t = malloc(token->capacity);
        if (!t) {
            return -1;
        }
        token->text = t;
    } else if (token->len == token->capacity - 1) {
        size_t cap = token->capacity * 2;
        char *t = realloc(token->text, cap);
        if (!t) {
            return -1;
        }
        token->text = t;
        token->capacity = cap;
    }
    token->text[token->len++] = ch;
    token->text[token->len] = '\0';
    return 0;
}

void
free_token(Token *token)
{
    free(token->text);
}
