#include "command.h"
#include <stdlib.h>
enum { BUF_SIZE = 4096 };



void 
free_command(Command *cmd)
{
    switch (cmd->kind) {
    case KIND_SIMPLE:
        for (int i = 0; i < cmd->argc; ++i) {
            free((cmd->argv)[i]);
        }
        free(cmd->argv);
        break;
    case KIND_PIPELINE:
        for (int i = 0; i < cmd->pipeline_size; ++i) {
            free_command(cmd->pipeline_commands + i);
        }
        free(cmd->pipeline_commands);
        break;
    case KIND_SEQ1:
    case KIND_SEQ2:
        for (int i = 0; i < cmd->seq_size; ++i) {
            free_command(cmd->seq_commands + i);
        }
        free(cmd->seq_commands);
        free(cmd->seq_operations);
        break;
    case KIND_REDIRECT:
        free_command(cmd->rd_command);
        free(cmd->rd_path);
        free(cmd->rd_command);
        break;
    default:
        break;
    }
}





int
init_sequence_command(Command *cmd, int knd)
{
    if (knd != KIND_SEQ1 && knd != KIND_SEQ2) {
        return 1;
    }
    cmd->kind = knd;
    cmd->seq_size = 0;
    cmd->seq_commands = 
            malloc(sizeof(*cmd->seq_commands) * BUF_SIZE);
    cmd->seq_operations = 
            malloc(sizeof(*cmd->seq_operations) * BUF_SIZE);
    return 0;
}

int 
init_pipeline_command(Command *cmd)
{
    cmd->kind = KIND_PIPELINE;
    cmd->pipeline_size = 0;
    cmd->pipeline_commands = 
            malloc(sizeof(*cmd->pipeline_commands) * BUF_SIZE);
    return 0;
}


int 
init_redirect_command(Command *cmd)
{
    cmd->kind = KIND_REDIRECT;
    cmd->rd_mode = -1;
    cmd->rd_path = NULL;
    cmd->rd_command = 
            malloc(sizeof(*cmd->rd_command));
    return 0;
}


int 
init_simple_command(Command *cmd)
{
    cmd->kind = KIND_SIMPLE;
    cmd->argv = 
            malloc(sizeof(*cmd->argv) * BUF_SIZE);
    cmd->argc = 0;
    return 0;
}

int 
append_word_simple_command(Command *cmd, char *txt)
{
    if (cmd->kind != KIND_SIMPLE) {
        return 1;
    }
    (cmd->argv)[cmd->argc++] = txt;
    (cmd->argv)[cmd->argc] = NULL;
    return 0;
}

int 
append_to_pipeline(Command *dst, Command *src)
{
    if (dst->kind != KIND_PIPELINE) {
        return 1;
    }
    (dst->pipeline_commands)[dst->pipeline_size++] = *src;
    return 0;    
}

int 
append_command_to_sequence(Command *dst, Command *src)
{
    if (dst->kind != KIND_SEQ1 && 
            dst->kind != KIND_SEQ2) {
        return 1;
    }    
    (dst->seq_commands)[dst->seq_size++] = *src;
    return 0;
}

int 
append_operation_to_sequence(Command *cmd, int knd)
{
    if (cmd->kind != KIND_SEQ1 && 
            cmd->kind != KIND_SEQ2) {
        return 1;
    } 
    (cmd->seq_operations)[cmd->seq_size - 1] = knd;
    return 0;
}

int 
set_rd_command(Command *dst, Command *src)
{
    if (dst->kind != KIND_REDIRECT) {
        return 1;
    }     
    *(dst->rd_command) = *src;
    return 0;  
}