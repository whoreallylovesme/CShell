enum
{
    E_NO_NEWLINE,
    E_WORD_OR_OPEN_EXPECTED,
    E_WORD_EXPECTED_REDIRECT,
    E_CLOSE_EXPECTED,
    SUCCESS,
    ENUM_ERRORS_END
};

const char *error_message(int error);


