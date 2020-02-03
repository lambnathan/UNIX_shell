#include "error.h"
#include <unistd.h>
#include <stdio.h>
#include <pwd.h>
#include <stdlib.h>

#include <readline/history.h>
#include <readline/readline.h>

#include "parser.h"
#include "shell_builtins.h"
#include "interpreter.h"

void check_exit(struct ast_statement_list *statements);
char** get_command_args(char* command, struct ast_argument_list *arglist);

int main(int argc, char *argv[]){
	// TODO: WRITE A MAIN 
	//prrompt user for a command
	//should countinute to prompt until user types CTRL-D at an empty prompt, or uses the exit built-in to exit
	unsigned int uid = getuid();
	struct passwd *p = getpwuid(uid);
	//printf("The username is: %s", p->pw_name);
	char good[] = ":)";
	//char bad[] = ":(";

	char* line;
	using_history();
	for(;;){
		printf("%s %s ", p->pw_name, good); //prints command prompt
		line = readline("$ ");
		if(!line){
			printf("ctr-d pressed\n");
			const char* const argv[] = {"exit", NULL}; //constant array of constant argument strings
			builtin_command_get("exit")->function(interpreter_new(false), argv, 0, 1, 2);
		}

		struct ast_statement_list *statements = parse_input(line);
		//struct ast_argument_list *arglist = statements->first->pipeline->first->arglist;
		check_exit(statements);

		ast_statement_list_free(statements);
		free(line);
	}
	//free memory before exiting
	free(p);
	return 0;
}

//check if the command was "exit"
void check_exit(struct ast_statement_list *statements){
	//printf("in check exit\n");
	int length_of_arg = statements->first->pipeline->first->arglist->first->parts->first->string->size;
	if(length_of_arg != 4){
		return;
	}
	char subbuff[5];
	memcpy(subbuff, statements->first->pipeline->first->arglist->first->parts->first->string->data, 4);
	subbuff[5] = '\0';
	char test[] = "exit";
	int i;
	for(i = 0; i < 4; i++){
		if(subbuff[i] != test[i]){
			break;
		}
	}
	if(i == 4){
		//printf("you want to exit\n");
		char** args = get_command_args("exit", statements->first->pipeline->first->arglist);
		builtin_command_get("exit")->function(interpreter_new(true), (const char* const*)args, 0, 1, 2);
		free(args);
	}
	return;
}


/*
 *function that takes in the command typed and creates a null terminated
   array of strings of all of the arguemnts (including the command itself)
*/
char** get_command_args(char* command, struct ast_argument_list *arglist){
	int capacity = 2;
	int used = 1;
	char **arr = malloc(sizeof(char*)*capacity);
	arr[0] = command;
	while(arglist->rest != NULL){
		char* arg = arglist->rest->first->parts->first->string->data;
		if(capacity == used){
			capacity *= 2;
			arr = realloc(arr, sizeof(char*) * capacity);
		}
		arr[used] = arg;
		used++;
		arglist = arglist->rest;
	}
	//add in terminating NULL
	if(capacity == used){
			capacity *= 2;
			arr = realloc(arr, sizeof(char*) * capacity);
	}
	arr[used] = NULL;
	return arr;
}
