#include <stdlib.h>

#include "command.h"
#include "errors.h"
#include "scanner.h"
#include "parser.h"

static Token t;
static Command cmd;

static int skip_empty_lines(void);

/** GRAMMAR (SUITABLE FOR RECURSIVE DESCENT PARSING):
 *      command = seq1 "\n"
 *      seq1 = seq2 { ("&" | ";") (seq2 | <empty>) } + check: empty only at the end
 *      seq2 = pipeline { ("&&" | "||") pipeline }
 *      pipeline = redirect { "|" redirect }
 *      redirect = simple { ("<" | ">" | ">>") WORD }
 *      simple = WORD { WORD } | "(" seq1 ")"
 */

static int command(void);
static int seq1(void);
static int seq2(void);
static int pipeline(void);
static int redirect(void);
static int simple(void);

int
init_parser(FILE *input)
{
    int r;
    if ((r = init_scanner(input)) != 0) {
        return r;
    }
    return 0;
}

void
free_parser(void)
{
    free_token(&t);
}

int
next_command(Command *c)
{
    int r;
    if ((r = next_token(&t)) != 0) {
        return r;
    }

    if ((r = skip_empty_lines()) != 0) {
        return r;
    }

    if (t.kind == T_EOF) {
        return EOF;
    }

    if ((r = command()) != 0) {
        return r;
    }
    *c = cmd;
    return 0;
}

static int
skip_empty_lines(void)
{
    int r;
    while (t.kind == T_NEWLINE) {
        if ((r = next_token(&t)) != 0) {
            return r;
        }
    }
    return 0;
}

static int
command(void)
{
    int r;
    if ((r = seq1()) != 0) {
        return r;
    }
    if (t.kind != T_NEWLINE) {
        return E_NO_NEWLINE;
    }
    return 0;
}

static int
seq_operation(enum TokenKind kind)
{
    switch (kind) {
    case T_CONJ: return OP_CONJUNCT;
    case T_DISJ: return OP_DISJUNCT;
    case T_SEQ: return OP_SEQ;
    case T_BACK: return OP_BACKGROUND; 
    default: abort();
    }
}

static int
seq1(void)
{
    if (!(t.kind == T_WORD || t.kind == T_OPEN)) {
        return E_WORD_OR_OPEN_EXPECTED;
    }

    int r;

    Command c;
    if ((r = init_sequence_command(&c, KIND_SEQ1)) != 0) {
        return r;
    }

    while (t.kind == T_WORD || t.kind == T_OPEN) {

        if ((r = seq2()) != 0) {
            free_command(&c);
            return r;
        }

        if ((r = append_command_to_sequence(&c, &cmd)) != 0) {
            free_command(&c);
            return r;
        }

        if (t.kind == T_SEQ || t.kind == T_BACK) {
        
            if ((r = append_operation_to_sequence(&c, seq_operation(t.kind))) != 0) {
                free_command(&c);
                return r;
            }

            if ((r = next_token(&t)) != 0) {
                free_command(&c);
                return r;
            }

        } else {

            if ((r = append_operation_to_sequence(&c, OP_SEQ)) != 0) {
                free_command(&c);
                return r;
            }

            break;
        }
    }

    cmd = c;
    return 0;
}

static int
seq2(void)
{
    int r;
    if ((r = pipeline()) != 0) {
        return r;
    }

    Command c;
    if ((r = init_sequence_command(&c, KIND_SEQ2)) != 0) {
        return r;
    }
    if ((r = append_command_to_sequence(&c, &cmd)) != 0) { // move semantics
        free_command(&c);
        return r;
    }

    while (t.kind == T_CONJ || t.kind == T_DISJ) {

        if ((r = append_operation_to_sequence(&c, seq_operation(t.kind))) != 0) {
            free_command(&c);
            return r;
        }

        if ((r = next_token(&t)) != 0) {
            return r;
        }

        if ((r = pipeline()) != 0) {
            return r;
        }

        if ((r = append_command_to_sequence(&c, &cmd)) != 0) { // move semantics
            free_command(&c);
            return r;
        }
    }

    if ((r = append_operation_to_sequence(&c, OP_SEQ)) != 0) {
        free_command(&c);
        return r;
    }

    cmd = c;
    return 0;
}

static int
pipeline(void)
{
    int r;
    if ((r = redirect()) != 0) {
        return r;
    }

    Command c;
    if ((r = init_pipeline_command(&c)) != 0) {
        return r;
    }
    if ((r = append_to_pipeline(&c, &cmd)) != 0) { // move semantics
        free_command(&c);
        return r;
    }

    while (t.kind == T_PIPE) {

        if ((r = next_token(&t)) != 0) {
            free_command(&c);
            return r;
        }

        if ((r = redirect()) != 0) {
            free_command(&c);
            return r;
        }

        if ((r = append_to_pipeline(&c, &cmd)) != 0) { // move semantics
            free_command(&c);
            return r;
        }

    }

    // "cmd" is moved inside "c"
    cmd = c;
    return 0;
}

static int
redirect_mode(enum TokenKind kind)
{
    switch (kind) {
        case T_IN: return RD_INPUT;
        case T_OUT: return RD_OUTPUT;
        case T_APPEND: return RD_APPEND;
        default: abort();
    }
}

static int
redirect(void)
{
    int r;
    if ((r = simple()) != 0) {
        return r;
    }

    while (t.kind == T_IN || t.kind == T_OUT || t.kind == T_APPEND) {

        int mode = redirect_mode(t.kind);
    
        if ((r = next_token(&t)) != 0) {
            return r;
        }

        if (t.kind != T_WORD) {
            return E_WORD_EXPECTED_REDIRECT;
        }

        Command c;
        if ((r = init_redirect_command(&c)) != 0) {
            return r;
        }
        if ((r = set_rd_command(&c, &cmd)) != 0) { // move semantics
            free_command(&c);
            return r;
        }
        c.rd_mode = mode;
        c.rd_path = t.text; // move semantics
        cmd = c; // move semantics

        if ((r = next_token(&t)) != 0) {
            return r;
        }
    }

    return 0;
}

static int
simple(void)
{
    int r;

    if (t.kind == T_WORD) {

        Command c;
        if ((r = init_simple_command(&c)) != 0) {
            return r;
        }

        if ((r = append_word_simple_command(&c, t.text)) != 0) { // move semantics
            free_command(&c);
            return r;
        }

        if ((r = next_token(&t)) != 0) {
            free_command(&c);
            return r;
        }

        while (t.kind == T_WORD) {

            if ((r = append_word_simple_command(&c, t.text)) != 0) { // move semantics
                free_command(&c);
                return r;
            }

            if ((r = next_token(&t)) != 0) {
                free_command(&c);
                return r;
            }
        }

        cmd = c; // move semantics
        return 0;
    } else if (t.kind == T_OPEN) {
        
        if ((r = next_token(&t)) != 0) {
            return r;
        }

        if ((r = seq1()) != 0) {
            return r;
        }

        if (t.kind != T_CLOSE) {
            return E_CLOSE_EXPECTED;
        }

        if ((r = next_token(&t)) != 0) {
            return r;
        }

        return 0;
    } else {
        return E_WORD_OR_OPEN_EXPECTED;
    }
}
