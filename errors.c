#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "errors.h"

static const char *error_messages[] = {
    [SUCCESS] = "Success",
    [E_NO_NEWLINE] = "No newline at the end of command",
    [E_WORD_EXPECTED_REDIRECT] = "Word is expected after the redirect operation",
    [E_CLOSE_EXPECTED] = "Close brace is expected",
    [E_WORD_OR_OPEN_EXPECTED] = "Word or open brace is expected as a beginning of a simple command",
};

const char *
error_message(int error)
{
    if (error == -1) {
        return strerror(errno);
    } else if (error > 0  && error < ENUM_ERRORS_END) {
        return error_messages[error];
    } else {
        abort();
    }
}
