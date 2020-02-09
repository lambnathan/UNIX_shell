#include <stdio.h>

#include "error.h"
#include "shell_builtins.h"

static int echo_builtin(struct interpreter_state *state, 
                        const char* const *argv, int input_fd, int output_fd,
                        int error_fd)
{
    FILE *error_fp = fdopen(error_fd, "w");
    CHECK(error_fp);
    CHECK(argv && argv[0]);

    for(int i = 1; argv[i] != NULL; i++){
        if(argv[i+1] == NULL){ //if on last arg, dont print space at end
            printf("%s", argv[i]);
        }
        else{
            printf("%s ", argv[i]);
        }
    }
    printf("\n");
    fflush(error_fp);
    return 0; //should always return 0
}

DEFINE_BUILTIN_COMMAND("echo", echo_builtin);
