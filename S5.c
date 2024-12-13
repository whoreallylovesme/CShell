#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>

typedef enum Kind 
{
    KIND_SIMPLE, KIND_PIPELINE, 
    KIND_SEQ1, KIND_SEQ2, KIND_REDIRECT
} Kind;

typedef enum OP
{
    OP_SEQ, OP_BACKGROUND, 
    OP_CONJUNCT, OP_DISJUNCT
} OP;

typedef enum RD
{
    RD_INPUT, RD_OUTPUT, RD_APPEND
} RD;

typedef int fd_t[2];

typedef struct Command 
{
    int kind;

    union 
    {
        // Simple
        struct 
        {
            int argc;    
            char **argv;
        };

        // Pipeline
        struct 
        {
            int pipeline_size;
            struct Command *pipeline_commands;
        };

        // Seq
        struct 
        {
            int seq_size;
            struct Command *seq_commands;
            int *seq_operations;
        };

        // Redirect
        struct 
        {
            int rd_mode;
            char *rd_path;
            struct Command *rd_command;
        };
    };
} Command;


int run_command(Command *cmd);


int
run_SIMPLE(Command *cmd) 
{
    if (fork() == 0) {
        (cmd->argv)[cmd->argc] = NULL;
        execvp((cmd->argv)[0], cmd->argv);
        exit(1);
    }

    int st;
    wait(&st);
    return WIFEXITED(st) ? WEXITSTATUS(st) : -1;
}


int
run_PIPELINE(Command *cmd) 
{
    int size = cmd->pipeline_size;
    fd_t *fds = malloc(sizeof(*fds) * (size));

    for (int i = 0; i < size - 1; ++i) {
        pipe(fds[i]); // fds[i] - pipe from i to i + 1
    }

    for (int i = 0; i < size; ++i) {
        if (fork() == 0) {
            if (i == 0) {
                dup2(fds[i][1], 1);
            } else if (i == size - 1) {
                dup2(fds[i - 1][0], 0);
            } else {
                dup2(fds[i - 1][0], 0);
                dup2(fds[i][1], 1);
            }
            for (int j = 0; j < size - 1; ++j) {
                close(fds[j][0]);
                close(fds[j][1]);
            }

            int st = run_command(cmd->pipeline_commands + i);
            free(fds);
            exit(st);
        }
    }

    for (int i = 0; i < size - 1; ++i) {
        close(fds[i][0]);
        close(fds[i][1]);
    }

    free(fds);
    while (wait(NULL) > 0);
    return 0;

}

int
run_SEQ2(Command *cmd) 
{
    // OP_CONJUNCT &&
    // OP_DISJUNCT ||

    int size = cmd->seq_size;
    Command *cmds = cmd->seq_commands;
    int *ops = cmd->seq_operations;

    int success = -1;
    for (int i = 0; i < size; ++i) {
        if (cmds[i].kind != KIND_PIPELINE) {
            return -1;
        }

        // вот тут сложно: 
        // 1) первая команда делается всегда
        // 2) последующие в зависимости от || или && и успеха предыдущего
        if (i != 0) {
            int skip = !((ops[i - 1] == OP_CONJUNCT && success) ||
                    (ops[i - 1] == OP_DISJUNCT && !success));
            if (skip) {
                continue;
            }
        }

        if (fork() == 0) {
            run_PIPELINE(cmds + i);
            exit(0);
        }
        int st;
        wait(&st);
        success = WIFEXITED(st) && WEXITSTATUS(st) == 0;
    }

    return 0;
}




int
run_SEQ1(Command *cmd) 
{
    // OP_SEQ ; 
    // OP_BACKGROUND &

    int size = cmd->seq_size;
    Command *cmds = cmd->seq_commands;
    int *ops = cmd->seq_operations;

    for (int i = 0; i < size; ++i) {
        if (fork() == 0) {
            switch (cmds[i].kind) {
            case KIND_SEQ2:
                if (run_SEQ2(cmds + i) == -1) {
                    exit(-1);
                }
                break;
            case KIND_PIPELINE:
                if (run_PIPELINE(cmds + i) == -1) {
                    exit(-1);
                }
                break;
            default:
                exit(-1);
                break;
            }
            exit(0);
        }

        if (ops[i] == OP_SEQ) {
            int st = 0;
            wait(&st);
            if (st != 0) {
                exit(-1);
            }
        } else if (ops[i] == OP_BACKGROUND){
            continue;
        }            
    }

    while (wait(NULL) > 0);
  
    return 0;
}


int
run_REDIRECT(Command *cmd) 
{
    int fd = -1;
    int st = -1;
    switch (cmd->rd_mode) {
    case RD_INPUT:
        fd = open(cmd->rd_path, O_RDONLY);
        dup2(fd, 0);
        close(fd);
        st = run_command(cmd->rd_command);
        break;
    case RD_OUTPUT:
        fd = open(cmd->rd_path, 
                O_WRONLY | O_CREAT | O_TRUNC, 0666);    
        dup2(fd, 1);
        close(fd);
        st = run_command(cmd->rd_command);
        break;
    case RD_APPEND:
        fd = open(cmd->rd_path, 
                O_WRONLY | O_APPEND | O_CREAT, 0666);
        dup2(fd, 1);
        close(fd);
        st = run_command(cmd->rd_command);
        break; 
    default:
        break;       
    }

    return st;
}

int
run_command(Command *cmd) 
{
    int st;
    switch (cmd->kind) {
    case KIND_SIMPLE:
        st = run_SIMPLE(cmd);
        break;
    case KIND_PIPELINE:
        st = run_PIPELINE(cmd);
        break;
    case KIND_SEQ1: 
        st = run_SEQ1(cmd);
        break;
    case KIND_SEQ2: 
        st = run_SEQ2(cmd);
        break;
    case KIND_REDIRECT:
        st = run_REDIRECT(cmd);
        break;
    default:
        st = -1;
        break;
    }



    return st;
}

int
main(void)
{
    return 0;
}
