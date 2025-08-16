enum CommandKind
{
    KIND_SIMPLE, KIND_PIPELINE, 
    KIND_SEQ1, KIND_SEQ2, KIND_REDIRECT
};

enum CommandOP
{
    OP_SEQ, OP_BACKGROUND, 
    OP_CONJUNCT, OP_DISJUNCT
};

enum CommandRD
{
    RD_INPUT, RD_OUTPUT, RD_APPEND
};

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

void free_command(Command *cmd);
int next_command(Command *cmd);

int init_sequence_command(Command *cmd, int knd);
int init_pipeline_command(Command *cmd);
int init_redirect_command(Command *cmd);
int init_simple_command(Command *cmd);

int append_word_simple_command(Command *cmd, char *txt);
int append_to_pipeline(Command *src, Command *dst);
int append_command_to_sequence(Command *src, Command *dst);
int append_operation_to_sequence(Command *cmd, int knd);
int set_rd_command(Command *src, Command *dst);