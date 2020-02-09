#include <unistd.h>
#include <stdio.h>
#include <pwd.h>
#include <stdlib.h>

#include <readline/history.h>
#include <readline/readline.h>

#include "alias.h"
#include "error.h"
#include "parser.h"
#include "shell_builtins.h"
#include "interpreter.h"

void check_exit(char** command_args, int *status);
void check_history(char** command_args, int* status);
void check_echo(char** command_args, int* status);
void check_cd(char** command_args, int* status);
void check_pwd(char** command_args, int* status);

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

	struct alias_table *table =  alias_table_new(); //create a new alias table
	table->used = 0;
	table->capacity = 2;
	table->name = malloc(sizeof(char*) * table->capacity);
	table->value = malloc(sizeof(char*) * table->capacity);


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

		line = readline(prompt); //prompt user
		
		if(line == (char*)NULL){
			const char* const argv[] = {"exit", NULL}; //constant array of constant argument strings
			builtin_command_get("exit")->function(interpreter_new(false), argv, 0, 1, 2);
		}
		else if(line[0] == '\0'){//enter pressed with no other input
			free(line);
			continue;
		}
		else if(line[0] == ' '){
			int i;
			for(i = 0; line[i] == ' '; i++){} //tests if only spaces were entered
			if(line[i] == '\0'){
				free(line);
				continue;
			}
		}

		//per bash manual, history expansion is performed immediately after a complete 
		//line is read, before it is broken into words
		char *output = NULL;
		int r = history_expand(line, &output);
		if(r == 0){//no expansion took place

		}
		else if(r == 1){//expansion took place
			printf("%s\n", output);
			free(line);
			line = output; //free lines memory then reassign line's memory address to outputs
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

		//parse input and get arguments
		struct ast_statement_list *statements = parse_input(line);
		int size = statements->first->pipeline->first->arglist->first->parts->first->string->size;
		char* command = malloc(sizeof(char) * (size + 1));
		memcpy(command, statements->first->pipeline->first->arglist->first->parts->first->string->data, size);
		command[size] = '\0';
		char** command_args = get_command_args(command, statements->first->pipeline->first->arglist);

		//CHECK ALIAS TABLE
		

		//check for all of the builtin commands
		//if a command is matched, it is executed inside the respectvie function, and then returns
		check_exit(command_args, &good);
		check_history(command_args, &good);
		check_echo(command_args, &good);
		check_cd(command_args, &good);
		check_pwd(command_args, &good);

		ast_statement_list_free(statements);
		free(line); //by freeing line, we are also freeing output in the case of history expansion
		free(prompt);
		free(command_args);
	}
	//free memory before exiting
	free(user_name);
	free(p);
	return 0;
}

//check if the command was "exit"
void check_exit(char** command_args, int* status){
	if(strcmp(command_args[0], "exit") == 0){
		struct builtin_command *cmd = builtin_command_get("exit");
		int retrn = cmd->function(interpreter_new(true), (const char* const*)command_args, 0, 1, 2);
		*status = retrn;
	}
	else{
		return;
	}
}

//check if the command was history
void check_history(char** command_args, int* status){
	if(strcmp(command_args[0], "history") == 0){
		struct builtin_command *cmd = builtin_command_get("history");
		int retrn = cmd->function(interpreter_new(true), (const char* const*)command_args, 0, 1, 2);
		*status = retrn;
	}
	else{
		return;
	}
}

//check if the echo command was given
void check_echo(char** command_args, int* status){
	if(strcmp(command_args[0], "echo") == 0){
		struct builtin_command *cmd = builtin_command_get("echo");
		int retrn = cmd->function(interpreter_new(true), (const char* const*)command_args, 0, 1, 2);
		*status = retrn;
	}
	else{
		return;
	}
}

//check if the cd command was given
void check_cd(char** command_args, int* status){
	if(strcmp(command_args[0], "cd") == 0){
		struct builtin_command *cmd = builtin_command_get("cd");
		int retrn = cmd->function(interpreter_new(true), (const char* const*)command_args, 0, 1, 2);
		*status = retrn;
	}
	else{
		return;
	}
}

//check if the user entered the pwd command
void check_pwd(char** command_args, int* status){
	if(strcmp(command_args[0], "pwd") == 0){
		struct builtin_command *cmd = builtin_command_get("pwd");
		int retrn = cmd->function(interpreter_new(true), (const char* const*)command_args, 0, 1, 2);
		*status = retrn;
	}
	else{
		return;
	}
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
		int size = arglist->rest->first->parts->first->string->size;
		char* arg  = malloc(sizeof(char) * (size + 1));
		memcpy(arg, arglist->rest->first->parts->first->string->data, size);
		arg[size] = '\0';
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
