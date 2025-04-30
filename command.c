#include <stdio.h>
#include <stdlib.h>

#include "command.h"

enum { BUF_SIZE = 4096 };

int
init_sequence_command(Command *c, int kind)
{
    c->kind = kind;
    c->seq_size = 0;
    c->seq_commands = malloc(sizeof(*c->seq_commands)*BUF_SIZE);
    if (c->seq_commands == NULL) {
        return -1;
    }
    c->seq_operations = malloc(sizeof(*c->seq_operations)*BUF_SIZE);
    if (c->seq_operations == NULL) {
        return -1;
    }
    return 0;
}

int
append_command_to_sequence(Command *c, Command *cmd)
{
    // c->seq_commands = realloc(c->seq_commands, (c->seq_size + 1)*sizeof(Command));
    //if (c->seq_commands == NULL) {
    //    return -1;
    //}
    *(c->seq_commands+c->seq_size) = *cmd;
    c->seq_size++;
    return 0;
}

int
append_operation_to_sequence(Command *c, int op)
{
    //c->seq_operations = realloc(c->seq_operations, (c->seq_size + 1)*sizeof(int));
    //if (c->seq_operations == NULL) {
    //    return -1;
    //}
    *(c->seq_operations+c->seq_size - 1) = op;
    return 0;
}

int
init_pipeline_command(Command *c)
{
    c->kind = KIND_PIPELINE;
    c->pipeline_size = 0;
    c->pipeline_commands = malloc(sizeof(*c->pipeline_commands)*BUF_SIZE);
    if (c->pipeline_commands == NULL) {
        return -1;
    }
    return 0;
}

int
append_to_pipeline(Command *c, Command *cmd)
{
    //c->pipeline_commands = realloc(c->pipeline_commands, (c->pipeline_size+1)*sizeof(Command));
    //if (c->pipeline_commands == NULL) {
    //    return -1;
    //}
    *(c->pipeline_commands + c->pipeline_size) = *cmd;
    c->pipeline_size++;
    return 0;
}

int
init_redirect_command(Command *c)
{
    c->kind = KIND_REDIRECT;
    c->rd_command = malloc(sizeof(*c->rd_command)*BUF_SIZE);
    if (c->rd_command == NULL) {
        return -1;
    }
    c->rd_path = NULL;
    c->rd_mode = -1;
    return 0;
}

int
set_rd_command(Command *c, Command *cmd)
{
    //c->rd_command = realloc(c->rd_command, (sizeof(Command)));
    //if (c->rd_command == NULL) {
    //    return -1;
    //}
    *(c->rd_command) = *cmd;
    return 0;
}

int
init_simple_command(Command *c)
{
    c->kind = KIND_SIMPLE;
    c->argv = malloc(sizeof(*c->argv)*BUF_SIZE);
    if (c->argv == NULL) {
        return -1;
    }
    c->argc = 0;
    return 0;
}

int
append_word_simple_command(Command *c, char *arg)
{
    //c->argv = realloc(c->argv, (c->argc + 2)*sizeof(char*));
    //if (c->argv == NULL) {
    //    return -1;
    //}
    c->argv[c->argc] = arg;
    *(c->argv+c->argc+1) = NULL;
    c->argc++;
    return 0;
}

void free_command(Command *c)
{
    if (c == NULL) {
        return;
    }
    switch (c->kind)
    {
        case KIND_PIPELINE:
            for (int i = 0; i < c->pipeline_size; ++i) {
                free_command(c->pipeline_commands + i);
            }
            free(c->pipeline_commands);
            break;
        case KIND_REDIRECT:
            if (c->rd_path != NULL) {
                free(c->rd_path);
            }
            if (c->rd_command != NULL) {
                free_command(c->rd_command);
                free(c->rd_command);
            }
            break;
        case KIND_SEQ1:
        case KIND_SEQ2:
            for (int i = 0; i < c->seq_size; ++i) {
                free_command(c->seq_commands + i);
            }
            free(c->seq_commands);
            free(c->seq_operations);
            break;
        case KIND_SIMPLE:
            for (int i = 0; i < c->argc; ++i) {
                free(c->argv[i]);
            }
            free(c->argv);
            break;
        default:
            break;
    }
}
