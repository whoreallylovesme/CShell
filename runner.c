#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>


#include "command.h"
#include "runner.h"

typedef int fd_t[2];


static int
run_SIMPLE(Command *cmd) 
{
    if (cmd->argc <= 0) {
        return 1;
    }

    if (fork() == 0) {
        (cmd->argv)[cmd->argc] = NULL;
        execvp((cmd->argv)[0], cmd->argv);
        exit(1);
    }

    int st;
    wait(&st);

    // free_command(cmd);

    return !(WIFEXITED(st) && WEXITSTATUS(st) == 0);
}


static int
run_PIPELINE(Command *cmd) 
{
    int size = cmd->pipeline_size;
    if (size == 1) {
        int st;
        if (fork() == 0) {
            st = run_command(cmd->pipeline_commands);
            exit(st);
        }
        wait(&st);
        return !(WIFEXITED(st) && WEXITSTATUS(st) == 0);
    } else {
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

        while (wait(0) > 0);
        return 0;
    }
    return 0;

}


static int
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
            return 1;
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
            int st = run_PIPELINE(cmds + i);
            exit(st);
        }
        int st;
        wait(&st);
        success = WIFEXITED(st) && WEXITSTATUS(st) == 0;
    }

    return 0;
}



static int
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
                if (run_SEQ2(cmds + i) != 0) {
                    exit(1);
                }
                break;
            case KIND_PIPELINE:
                if (run_PIPELINE(cmds + i) != 0) {
                    exit(1);
                }
                break;
            default:
                exit(1);
                break;
            }
            exit(0);
        }

        if (ops[i] == OP_SEQ) {
            int st = 0;
            wait(&st);
            if (!(WIFEXITED(st) && WEXITSTATUS(st) == 0)) {
                return 1;
            }
        } else if (ops[i] == OP_BACKGROUND){
            continue;
        }            
    }

    // while (wait(NULL) > 0);
  
    return 0;
}




static int
run_REDIRECT(Command *cmd) 
{
    int fd = -1;
    int st = -1;
    switch (cmd->rd_mode) {
    case RD_INPUT:
        fd = open(cmd->rd_path, O_RDONLY);
        if (fork() == 0) {
            dup2(fd, 0);
            // close(fd);
            st = run_command(cmd->rd_command);
            exit(st);
        }
        break;
    case RD_OUTPUT:
        fd = open(cmd->rd_path, 
                O_WRONLY | O_CREAT | O_TRUNC, 0666);  
        if (fork() == 0) {  
            dup2(fd, 1);
            // close(fd);
            st = run_command(cmd->rd_command);
            exit(st);
        }
        break;
    case RD_APPEND:
        fd = open(cmd->rd_path, 
                O_WRONLY | O_APPEND | O_CREAT, 0666);
        if (fork() == 0) {
            dup2(fd, 1);
            // close(fd);
            st = run_command(cmd->rd_command);
            exit(st);
        }
        break; 
    default:
        break;       
    }
    close(fd);
    wait(&st);
    return !(WIFEXITED(st) && WEXITSTATUS(st) == 0);
}


int
run_command(Command *cmd) 
{
    // printf("RUNNING\n");

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
        st = 1;
        break;
    }

    // free_command(cmd);



    return st;
}