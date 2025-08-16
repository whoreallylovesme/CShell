#include <stdio.h>

#include "command.h"
#include "errors.h"
#include "parser.h"
#include "runner.h"


#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>


int
init_empty_command(Command *c)
{
    c->kind = -999;
    return 0;
}

int
main(int argc, char *argv[])
{
    int r;
    while (1) {
        if ((r = init_parser(stdin)) != 0) {
            fprintf(stderr, "%s\n", error_message(r));
            return 0;
        }

        Command c;
        // init_empty_command(&c);
        if ((r = next_command(&c)) == EOF && feof(stdin)) {
            free_parser();
            break;
        } else if (r != 0) {
            fprintf(stderr, "%s\n", error_message(r));
            free_parser();
            // printf("HERE RN\n");
            // break;
            continue;
        }

        
        // if (c.kind == -999) {
        //     break;
        // }
        
        if (fork() == 0) {
            int st = run_command(&c);
            exit(st);
        }
        // int st;
        // wait(&st);

        // if (!(WIFEXITED(st) && WEXITSTATUS(st) == 0)) {
        //     free_command(&c);
        //     free_parser();
        //     break;
        // }
    

        free_command(&c);
        free_parser();
    }

    // printf("HERE\n");
    while (wait(NULL) > 0);
    return 0;
}
