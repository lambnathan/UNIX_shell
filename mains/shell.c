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

void check_exit(struct ast_statement_list *statements, int *status);
void check_history(struct ast_statement_list *statements, int* status);
void check_echo(struct ast_statement_list *statements, int* status);
char** get_command_args(char* command, struct ast_argument_list *arglist);

int main(int argc, char *argv[]){
	unsigned int uid = getuid();
	struct passwd *p = getpwuid(uid);
	//printf("The username is: %s", p->pw_name);
	char smile[] = ":) $ ";
	char frown[] = ":( $ ";
	char* user_name = p->pw_name;
	strcat(user_name, " ");
	int good = 0; //good; no command has returned error yet (return value)

	char* line;
	using_history();
	//main shell loop
	for(;;){
		//create the prompt
		char* prompt = malloc(sizeof(char)*(strlen(user_name) + 5));
		strcpy(prompt, user_name);
		if(good == 0){
			strcat(prompt, smile); 
		} 
		else{
			strcat(prompt, frown);
		}

		line = readline(prompt);
		if(!line){
			const char* const argv[] = {"exit", NULL}; //constant array of constant argument strings
			builtin_command_get("exit")->function(interpreter_new(false), argv, 0, 1, 2);
		}

		
		char *output = NULL;
		int r = history_expand(line, &output);
		if(r == 0){//no expansion took place

		}
		else if(r == 1){//expansion took place
			printf("%s\n", output);
			add_history(line);
			strcpy(line, output);
			free(output);
			//line = output; //set line = output, and then will rin through that command below
		}
		else if(r == -1){//error in expansion
			fprintf(stderr, "There was an error in expansion: %s\n", output);
			good = 0;
			free(line);
			free(prompt);
			free(output);
			continue;
		}
		else if(r == 2){//returned line should be displayed but not executed (:p)
			printf("%s\n", output);
			free(line);
			free(prompt);
			free(output);
			continue;
		}
		add_history(line);
		
		struct ast_statement_list *statements = parse_input(line);
		//struct ast_argument_list *arglist = statements->first->pipeline->first->arglist;

		//check for all of the builtin commands
		//if a command is matched, it is executed inside the respectvie function, and then returns
		check_exit(statements, &good);
		check_history(statements, &good);
		check_echo(statements, &good);

		ast_statement_list_free(statements);
		free(line);
		free(prompt);
	}
	//free memory before exiting
	free(user_name);
	free(p);
	return 0;
}

//check if the command was "exit"
void check_exit(struct ast_statement_list *statements, int* status){
	//printf("in check exit\n");
	int length_of_arg = statements->first->pipeline->first->arglist->first->parts->first->string->size;
	if(length_of_arg != 4){
		return;
	}
	char subbuff[5];
	memcpy(subbuff, statements->first->pipeline->first->arglist->first->parts->first->string->data, 4);
	subbuff[4] = '\0';
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
		struct builtin_command *cmd = builtin_command_get("exit");
		int retrn = cmd->function(interpreter_new(true), (const char* const*)args, 0, 1, 2);
		*status = retrn;

		free(args);
	}
	return;
}

//check if the command was history
void check_history(struct ast_statement_list *statements, int* status){
	int length_of_args = statements->first->pipeline->first->arglist->first->parts->first->string->size;
	if(length_of_args != 7){ //invalid lenth to be history command
		return;
	}
	char subbuff[8];
	memcpy(subbuff, statements->first->pipeline->first->arglist->first->parts->first->string->data, 7);
	subbuff[7] = '\0';
	char test[] = "history";
	int i;
	for(i = 0; i < 7; i++){
		if(subbuff[i] != test[i]){//command not equal to history
			break;
		}
	}
	if(i == 7){
		char** args = get_command_args("history", statements->first->pipeline->first->arglist);
		struct builtin_command *cmd = builtin_command_get("history");
		int retrn = cmd->function(interpreter_new(true), (const char* const*)args, 0, 1, 2);
		*status = retrn;
		free(args);
	}
	return;
}

//check if the echo command was given
void check_echo(struct ast_statement_list *statements, int* status){
	int length_of_args = statements->first->pipeline->first->arglist->first->parts->first->string->size;
	if(length_of_args != 4){
		return;
	}
	char subbuff[5];
	memcpy(subbuff, statements->first->pipeline->first->arglist->first->parts->first->string->data, 4);
	subbuff[4] = '\0';
	char test[] = "echo";
	int i;
	for(i = 0; i < 4; i++){
		if(subbuff[i] != test[i]){
			break;
		}
	}
	if(i == 4){
		char** args = get_command_args("echo", statements->first->pipeline->first->arglist);
		struct builtin_command *cmd = builtin_command_get("echo");
		int retrn = cmd->function(interpreter_new(true), (const char* const*)args, 0, 1, 2);
		*status = retrn;
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
