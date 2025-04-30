#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>

#include "command.h"
#include "runner.h"

int
command_simple(Command *cmd)
{
    int status;
    if (fork() == 0) {
        execvp(*(cmd->argv), cmd->argv);
        exit(1);
    }       
    wait(&status);
    if (WIFEXITED(status) != 0 && WEXITSTATUS(status) == 0) {
        return 0;
    }
    return 1;
}

int
command_redirect(Command *cmd)
{
    int fd;
    int status;
    switch (cmd->rd_mode)
    {
        case RD_INPUT:
            fd = open(cmd->rd_path, O_RDONLY);
            if (fork() == 0) {
                dup2(fd, 0);
                status = run_command(cmd->rd_command);
                if (status) {
                    exit(1);
                }
                exit(0);
            }
            close(fd);
            wait(&status);
            if (WIFEXITED(status) != 0 && WEXITSTATUS(status) == 0) {
                return 0;
            }
            return 1;
            break;
        case RD_OUTPUT:
            fd = open(cmd->rd_path, O_CREAT | O_TRUNC | O_WRONLY, 0666);
            if (fork() == 0) {
                dup2(fd, 1);
                status = run_command(cmd->rd_command);
                if (status) {
                    exit(1);
                }
                exit(0);
            }
            close(fd);
            wait(&status);
            if (WIFEXITED(status) != 0 && WEXITSTATUS(status) == 0) {
                return 0;
            }
            return 1;
            break;
        case RD_APPEND:
            fd = open(cmd->rd_path, O_CREAT | O_WRONLY | O_APPEND, 0666);
            if (fork() == 0) {
                dup2(fd, 1);
                status = run_command(cmd->rd_command);
                if (status) {
                    exit(1);
                }
                exit(0);
            }
            close(fd);
            wait(&status);
            if (WIFEXITED(status) != 0 && WEXITSTATUS(status) == 0) {
                return 0;
            }       
            return 1;
        default:
            return 1;
    }
    return 0;
}

int
command_pipeline(Command *cmd)
{
    int status = 0;
    if (cmd->pipeline_size == 1) {
        if (fork() == 0) {
            status = run_command(cmd->pipeline_commands);
            if (status) {
                exit(1);
            }
            exit(0);
        }
        wait(&status);
        if (WIFEXITED(status) != 0 && WEXITSTATUS(status) == 0) {
            return 0;
        }
        return 1;
    } else if (cmd->pipeline_size >= 2) {
        int pipes[cmd->pipeline_size - 1][2];
        for (int i = 0; i < cmd->pipeline_size - 1; ++i) {
            pipe(pipes[i]);
        }
        for (int i = 0; i < cmd->pipeline_size; ++i) {
            if (fork() == 0) {
                if (i == 0) {
                    dup2(pipes[i][1], 1);
                    close(pipes[i][0]);
                    close(pipes[i][1]);
                } else if (i == cmd->pipeline_size - 1) {
                    dup2(pipes[i-1][0], 0);
                    close(pipes[i-1][0]);
                    close(pipes[i-1][1]);
                } else {
                    dup2(pipes[i-1][0], 0);
                    close(pipes[i-1][0]);
                    close(pipes[i-1][1]);
                    dup2(pipes[i][1], 1);
                    close(pipes[i][0]);
                    close(pipes[i][1]);
                }
                status = run_command(cmd->pipeline_commands+i);
                exit(0);
            }
        }
        for (int i = 0; i < cmd->pipeline_size - 1; ++i) {
            close(pipes[i][0]);
            close(pipes[i][1]);
        }
        while (wait(NULL) != -1) {
        }
        return 0;
    }
    return 0;
}

int
command_seq2(Command *cmd)
{
    int status = 0;
    if (cmd->seq_size == 1) {
        if (fork() == 0) {
            status = run_command(cmd->seq_commands);
            if (status) {
                exit(1);
            }
            exit(0);
        }
        wait(&status);
        if (WIFEXITED(status) != 0 && WEXITSTATUS(status) == 0) {
            return 0;
        }
        return 1;
    }
    int op = 0;
    int k = 0;
    for (int i = 0; i < cmd->seq_size; ++i) {
        switch ((cmd->seq_operations)[k]) 
        {
            case OP_DISJUNCT:
                if (WIFEXITED(status) != 0 && WEXITSTATUS(status) == 0 && i != 0) {
                    if (op == 1) {
                        k++;
                    }
                    op = !op;
                    break;
                }
                if (fork() == 0) {
                    status = command_pipeline((cmd->seq_commands)+i);
                    if (status) {
                        exit(1);
                    }
                    exit(0);
                }
                wait(&status);
                if (op == 1) {
                    k++;
                }
                op = !op;
                break;
            case OP_CONJUNCT:
                if (WIFEXITED(status) == 0 || WEXITSTATUS(status) != 0) {
                    return 0;
                }
                if (fork() == 0) {
                    status = command_pipeline((cmd->seq_commands)+i);
                    if (status) {
                        exit(1);
                    }
                    exit(0);
                }
                wait(&status);
                if (op == 1) {
                    k++;
                }
                op = !op;
                if (WIFEXITED(status) == 0 || WEXITSTATUS(status) != 0) {
                    return 0;
                }
                break;
            default:
                return 1;
        }
    }
    return 0;
}

int
command_seq1(Command *cmd)
{
    int status;
    for (int i = 0; i < cmd->seq_size; ++i) {
        switch (cmd->seq_operations[i])
        {
            case OP_BACKGROUND:
                switch ((cmd->seq_commands+i)->kind)
                {
                    case KIND_SEQ2:
                        if (fork() == 0) {
                            command_seq2(cmd->seq_commands+i);
                            exit(0);
                        }
                        break;
                    case KIND_PIPELINE:
                        if (fork() == 0) {
                            command_pipeline(cmd->seq_commands+i);
                            exit(0);
                        }
                        break;
                    default:
                        return 1;
                }
                break;
            case OP_SEQ:
                switch ((cmd->seq_commands+i)->kind)
                {
                    case KIND_SEQ2:
                        if (fork() == 0) {
                            command_seq2(cmd->seq_commands+i);
                            exit(0);
                        }
                        wait(&status);
                        break;
                    case KIND_PIPELINE:
                        if (fork() == 0) {
                            command_pipeline(cmd->seq_commands+i);
                            exit(0);
                        }
                        wait(&status);
                        break;
                    default:
                        return 1;
                }
                break;
            default:
                return 1;
        }
    }
    while (wait(NULL) != -1) {
    }
    return 0;
}

int
run_command(Command *cmd)
{
    int status;
    switch (cmd->kind)
    {
        case KIND_SIMPLE:
            status = command_simple(cmd);
            break;
        case KIND_PIPELINE:
            status = command_pipeline(cmd);
            break;
        case KIND_REDIRECT:
            status = command_redirect(cmd);
            break;
        case KIND_SEQ1:
            status = command_seq1(cmd);
            break;
        case KIND_SEQ2:
            status = command_seq2(cmd);
            break;
        default:
            return 1;
    }
    return status;
}

