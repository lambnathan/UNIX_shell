#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "error.h"
#include "shell_builtins.h"

static int pwd_builtin(struct interpreter_state *state, 
                        const char* const *argv, int input_fd, int output_fd,
                        int error_fd)
{
    FILE *error_fp = fdopen(error_fd, "w");
    CHECK(error_fp);
    CHECK(argv && argv[0]);

    if(argv[1] != NULL){
        fprintf(error_fp, "%s: too many arguments\n", argv[0]);
		return 1;
    }
    else{
        char cwd[1000];
        if(getcwd(cwd, sizeof(cwd)) != NULL){
            printf("%s\n", cwd);
            return 0;
        }
        else{
            fprintf(error_fp, "%s: something went wrong\n", argv[0]);
            return 1;
        }
        
    }

}

DEFINE_BUILTIN_COMMAND("pwd", pwd_builtin);