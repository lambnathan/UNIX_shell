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
#include "vars.h"

//check if entered command was one of the builtins
void check_exit(char** command_args, int *status);
void check_history(char** command_args, int* status);
void check_echo(char** command_args, int* status);
void check_cd(char** command_args, int* status);
void check_pwd(char** command_args, int* status);

void check_alias(char** command_args, struct alias_table *table, int* status);//handled slightly differently than rest

char** get_command_args(char* command, struct ast_argument_list *arglist); //gets the arguemnts for the command

void search_aliases(struct alias_table *table, char** command_args); //check if entered command is in alias table
void assign_vars(struct vars_table *var_table, struct ast_assignment_list *aslist);

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

	//create new variable table
	struct vars_table *var_table = vars_table_new();
	var_table->used = 0;
	var_table->capacity = 2;
	var_table->name = malloc(sizeof(char*) * var_table->capacity);
	var_table->value = malloc(sizeof(char*) * var_table->capacity);


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
		if(statements->first->pipeline->first->assignments != NULL && statements->first->pipeline->first->arglist == NULL 
				&& statements->first->pipeline->first->input_file == NULL && statements->first->pipeline->first->output_file == NULL
				&& statements->first->pipeline->first->append_file == NULL){
			//do variable assignment
			assign_vars(var_table, statements->first->pipeline->first->assignments);
			free(line);
			free(prompt);
			free(output);
			continue;
		}
		int size = statements->first->pipeline->first->arglist->first->parts->first->string->size;
		char* command = malloc(sizeof(char) * (size + 1));
		memcpy(command, statements->first->pipeline->first->arglist->first->parts->first->string->data, size);
		command[size] = '\0';
		char** command_args = get_command_args(command, statements->first->pipeline->first->arglist);
		
		//CHECK ALIAS TABLE
		search_aliases(table, command_args);

		//test print all the vars
		print_all_vars(var_table);

		//check for all of the builtin commands
		//if a command is matched, it is executed inside the respectvie function, and then returns
		check_exit(command_args, &good);
		check_history(command_args, &good);
		check_echo(command_args, &good);
		check_cd(command_args, &good);
		check_pwd(command_args, &good);
		check_alias(command_args, table, &good);

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

//check if the user entered the alias or unalias command
void check_alias(char** command_args, struct alias_table *table, int* status){
	if(strcmp(command_args[0], "alias") == 0 || strcmp(command_args[0], "unalias") == 0){
		int retrn = alias_builtin(command_args, table);
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

/*
 *check if the first argument is aliased to a command, and if so, replace it with the actual command
*/
void search_aliases(struct alias_table *table, char** command_args){
	if(table->used == 0){//no aliases have been entered
		return;
	}
	else{
		const char* to_replace = alias_get(table, command_args[0]);
		if(to_replace != NULL){ //alias for user input does exist
			free(command_args[0]);
			command_args[0] = (char*)to_replace;
			return;
		}

		// for(int i = 0; i < table->used; i++){
		// 	if(strcmp(table->name[i], command_args[0]) == 0){
		// 		free(command_args[0]);
		// 		command_args[0] = table->value[i];
		// 		return;
		// 	}
		// }
	}
}

//aassign variables in shell
void assign_vars(struct vars_table *var_table, struct ast_assignment_list *aslist){
	int name_size = aslist->first->name->size;
	char* name = malloc(sizeof(char) * (name_size + 1));
	memcpy(name, aslist->first->name->data, name_size);
	name[name_size] = '\0';

	//check if var assined already exists. if it does, remove it first
	if(vars_get(var_table, name) != NULL){//var already exists, so have to remove previous one first
		vars_unset(var_table, name);
	}

	int val_size = aslist->first->value->parts->first->string->size;
	char* value = malloc(sizeof(char) * (val_size + 1));
	memcpy(value, aslist->first->value->parts->first->string->data, val_size);
	value[val_size] = '\0';
	vars_set(var_table, name, value);



	// if(command_args[1] == NULL){ //only 1 command
	// 		for(int i = 0; command_args[0][i] != '\0'; i++){ //check if the command is variable assignment
	// 			if(command_args[0][i] == '='){
	// 				//parse the assignment
	// 				int name_size = i + 1;
	// 				char name[name_size];
	// 				memcpy(name, command_args[0], i);
	// 				name[i] = '\0';

	// 				//check if var assined already exists. if it does, remove it first
	// 				if(vars_get(var_table, name) != NULL){//var already exists, so have to remove previous one first
	// 					vars_unset(var_table, name);
	// 				}

	// 				int k;
	// 				for(k = i + 1; command_args[0][k] != '\0'; k++){}
	// 				int replacement_size = k-i;
	// 				char replacement[replacement_size];
	// 				memcpy(replacement, &command_args[i+1], k-i-1);
	// 				replacement[k-i-1] = '\0';
	// 				vars_set(var_table, name, replacement);
	// 			}
	// 		}
	// 	}
}
