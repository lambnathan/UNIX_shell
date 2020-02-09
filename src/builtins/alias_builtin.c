#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "alias.h"
#include "error.h"
#include "shell_builtins.h"

static int alias_builtin(struct interpreter_state *state, 
                        const char* const *argv, int input_fd, int output_fd,
                        int error_fd)
{
    FILE *error_fp = fdopen(error_fd, "w");
    CHECK(error_fp);
    CHECK(argv && argv[0]);

    if(argv[1] == NULL){//print out all aliases

    }
    else{

    }
    return 0;
}

DEFINE_BUILTIN_COMMAND("alias", alias_builtin);
DEFINE_BUILTIN_COMMAND("unalias", alias_builtin);