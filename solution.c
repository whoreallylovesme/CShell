#include <stdio.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>

#include "command.h"
#include "runner.h"
#include "errors.h"
#include "parser.h"
#include "scanner.h"

int
main(void)
{
    int r;
    int status;
    while (1) {
        if ((r = init_parser(stdin)) != 0) {
            fprintf(stderr, "%s\n", error_message(r));
            return 0;
        }
        
		Command c;
        if ((r = next_command(&c)) == EOF && feof(stdin)) {
            free_parser();
            break;
        } else if (r != 0) {
            fprintf(stderr, "%s\n", error_message(r));

            free_parser();
            continue;
        }
		if (fork() == 0) {
			status = run_command(&c);
			if (status != 0) {
				exit(1);
			}
			exit(0);
		}
		free_command(&c);
		free_parser();
    }
	
	while (wait(NULL) != -1) {
	}	
    return 0;
}
