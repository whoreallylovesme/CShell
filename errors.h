typedef enum ERRORS
{
    SUCCESS,
    E_NO_NEWLINE,
    E_WORD_EXPECTED_REDIRECT,
    E_CLOSE_EXPECTED,
    E_WORD_OR_OPEN_EXPECTED
} ERRORS;

enum { ENUM_ERRORS_END = 6 };

const char *error_message(int error);