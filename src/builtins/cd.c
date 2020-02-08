#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "error.h"
#include "shell_builtins.h"

static int cd_builtin(struct interpreter_state *state, 
                        const char* const *argv, int input_fd, int output_fd,
                        int error_fd)
{
    FILE *error_fp = fdopen(error_fd, "w");
    CHECK(error_fp);
    CHECK(argv && argv[0]);

    if(argv[1] == NULL){//change to root directory
        char* home = "HOME";
        char* path = getenv(home);
        int result = chdir(path);
        return result;
    }
    else if(argv[1]){
        if(argv[2]){
            fprintf(error_fp, "%s: too many arguments\n", argv[0]);
            return 1;
        }
        else{
            int result = chdir(argv[1]);
            return result;
        }
    }

    fflush(error_fp);
    return 0;

}

DEFINE_BUILTIN_COMMAND("cd", cd_builtin);