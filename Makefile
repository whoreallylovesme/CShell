CFLAGS=-O2 -ftrapv -fsanitize=undefined -Wall -Werror -Wformat-security -Wignored-qualifiers -Winit-self -Wswitch-default -Wfloat-equal -Wshadow -Wpointer-arith -Wtype-limits -Wempty-body -Wlogical-op -Wstrict-prototypes -Wold-style-declaration -Wold-style-definition -Wmissing-parameter-type -Wmissing-field-initializers -Wnested-externs -Wno-pointer-sign -Wcast-qual -Wwrite-strings -std=gnu11 -g -lm

solution: errors.o parser.o scanner.o command.o runner.o

errors.o: errors.h

parser.o: parser.h

scanner.o: scanner.h

command.o: command.h

runner.o: runner.h

clean:
	rm -rf *.o solution

.PHONY: clean
