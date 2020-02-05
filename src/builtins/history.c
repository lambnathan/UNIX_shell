#include <stdio.h>
#include <stdlib.h>

#include "error.h"
#include "shell_builtins.h"

#include <readline/history.h>


static int history_builtin(struct interpreter_state *state, 
                        const char* const *argv, int input_fd, int output_fd,
                        int error_fd)
{
    FILE *error_fp = fdopen(error_fd, "w");
    CHECK(error_fp);
    CHECK(argv && argv[0]);
    if(argv[1]){
        fprintf(error_fp, "%s: too many arguments\n", argv[0]);
        return 1;
    }
    //print the entire history
    //int length = history_length;
    HIST_ENTRY **the_list;
    the_list = history_list();
    if(the_list){
        for(int i = 0; the_list[i]; i++){
            printf("%s %i\n", the_list[i]->line, i + 1);
        }
    }
    fflush(error_fp);
    return 0;

}

DEFINE_BUILTIN_COMMAND("history", history_builtin);