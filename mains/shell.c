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
			builtin_command_get("exit")->function(interpreter_new(true), argv, 0, 1, 2);
		}
		char subbuff[5];
		memcpy(subbuff, &line[0], 4);
		subbuff[5] = '\0';
		if(strcmp(subbuff, "exit") == 0){
			printf("you want to exit\n");
			const char* const argv[] = {"exit", NULL}; //constant array of constant argument strings
			builtin_command_get("exit")->function(interpreter_new(true), argv, 0, 1, 2);
		}

		struct ast_statement_list *statements = parse_input(line);
		//struct ast_argument_list *arglist = statements->first->pipeline->first->arglist;

		ast_statement_list_free(statements);
		
		free(line);
	}


	//free memory before exiting
	free(p);


	return 0;
}
