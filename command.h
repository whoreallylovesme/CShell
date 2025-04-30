enum 
{
    KIND_SIMPLE,
    KIND_REDIRECT,
    KIND_PIPELINE,
    KIND_SEQ1,
    KIND_SEQ2,
    RD_INPUT,
    RD_OUTPUT,
    RD_APPEND,
    OP_BACKGROUND,
    OP_DISJUNCT,
    OP_CONJUNCT,
    OP_SEQ,
};

struct Com {
    int kind;
    union {
        struct {
            int pipeline_size;
            struct Com *pipeline_commands;
        };
        struct {
            int rd_mode;
            char *rd_path;
            struct Com *rd_command;
        };
        struct {
            int seq_size;
            int *seq_operations;
            struct Com *seq_commands;
        };
        struct {
            int argc;
            char **argv;
        };
    };
};

typedef struct Com Command;

void free_command(Command *);

int next_command(Command *);

int init_pipeline_command(Command *);

int append_to_pipeline(Command *, Command *);

int init_redirect_command(Command *);

int set_rd_command(Command *, Command *);

int init_simple_command(Command *);

int append_word_simple_command(Command *, char *);

int init_sequence_command(Command *, int);

int append_command_to_sequence(Command *, Command *);

int append_operation_to_sequence(Command *, int);


