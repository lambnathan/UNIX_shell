#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <pwd.h>
#include <stdlib.h>
#include <sys/wait.h>

#include <readline/history.h>
#include <readline/readline.h>

#include "alias.h"
#include "error.h"
#include "parser.h"
#include "shell_builtins.h"
#include "interpreter.h"
#include "vars.h"

//check if entered command was one of the builtins
void check_exit(char** command_args, int *status, int *executed);
void check_history(char** command_args, int* status, int *executed);
void check_echo(char** command_args, int* status, int* executed);
void check_cd(char** command_args, int* status, int *executed);
void check_pwd(char** command_args, int* status, int *executed);

void check_alias(char** command_args, struct alias_table *table, int* status, int *executed);//handled slightly differently than rest

char** get_command_args(struct ast_argument_list *arglist, struct vars_table *var_table); //gets the arguemnts for the command


/*
 * command_args holds the command and its arguements, alias_table and var_table are tables for aliases and variables,
 * pipe_in and pipe_out are input file descriptors for pipes, redirect_file is the path for file redirection,
 * and red_fd is the number corresponding to the redirection file descriptor
 * (0 for input, 1 for output, 2 for appending, negative for no file redirection)
 */
int dispatch_command(char** command_args, struct alias_table *table, struct vars_table *var_table, 
					int *pipe_in, int *pipe_out, char *in_file, char *out_file, char *append_file);

char* get_file_path(struct ast_argument *arg, struct vars_table *var_table);

void search_aliases(struct alias_table *table, char** command_args); //check if entered command is in alias table
void assign_vars(struct vars_table *var_table, struct alias_table *table, struct ast_assignment_list *aslist);

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
		//check if the command is to do variable assignment
		if(statements->first->pipeline->first->assignments != NULL && statements->first->pipeline->first->arglist == NULL 
				&& statements->first->pipeline->first->input_file == NULL && statements->first->pipeline->first->output_file == NULL
				&& statements->first->pipeline->first->append_file == NULL){
			//do variable assignment
			assign_vars(var_table, table, statements->first->pipeline->first->assignments);
			free(line);
			free(prompt);
			free(output);
			good = 0;
			continue;
		}
		// int size = statements->first->pipeline->first->arglist->first->parts->first->string->size;
		// char* command = malloc(sizeof(char) * (size + 1));
		// memcpy(command, statements->first->pipeline->first->arglist->first->parts->first->string->data, size);
		// command[size] = '\0';

		int* pipe_in = NULL;
		int* pipe_out = NULL;
		char* in_file = NULL;
		char *out_file = NULL;
		char *append_file = NULL;
		struct ast_pipeline *pipeline = statements->first->pipeline;
		while(pipeline != NULL){
			char** command_args = get_command_args(pipeline->first->arglist, var_table);

			//check if there is pipeline
			if(pipeline->rest != NULL){
				pipe_out = malloc(sizeof(int) * 2);
				if(pipe(pipe_out) < 0){
					printf("pipe error\n");
					good = -1;
					break;
				}
			}

			//check if there is file redirection
			if(pipeline->first->input_file != NULL){
				in_file = get_file_path(pipeline->first->input_file, var_table);
			}
			if(pipeline->first->output_file != NULL){
				out_file = get_file_path(pipeline->first->output_file, var_table);
			}
			if(pipeline->first->append_file != NULL){
				append_file = get_file_path(pipeline->first->append_file, var_table);
			}

			good = dispatch_command(command_args, table, var_table, pipe_in, pipe_out, in_file, out_file, append_file);

			pipe_in = pipe_out;
			pipe_out = NULL;
			pipeline = pipeline->rest;
			//free(out_file);
			//free(in_file);
			//free(append_file);
		}
		free(pipe_out);
		free(pipeline);

		//test print all the vars
		// printf("VAR ASSIGNMENTS:\n");
		// print_all_vars(var_table);

		ast_statement_list_free(statements);
		free(line); //by freeing line, we are also freeing output in the case of history expansion
		free(prompt);
		//free(command_args);
	}

	//free memory before exiting
	free(user_name);
	free(p);
	return 0;
}

//check if the command was "exit"
void check_exit(char** command_args, int* status, int* executed){
	if(strcmp(command_args[0], "exit") == 0){
		struct builtin_command *cmd = builtin_command_get("exit");
		int retrn = cmd->function(interpreter_new(true), (const char* const*)command_args, 0, 1, 2);
		*status = retrn;
		*executed = 1;
	}
	else{
		return;
	}
}

//check if the command was history
void check_history(char** command_args, int* status, int* executed){
	if(strcmp(command_args[0], "history") == 0){
		struct builtin_command *cmd = builtin_command_get("history");
		int retrn = cmd->function(interpreter_new(true), (const char* const*)command_args, 0, 1, 2);
		*status = retrn;
		*executed = 1;
	}
	else{
		return;
	}
}

//check if the echo command was given
void check_echo(char** command_args, int* status, int* executed){
	if(strcmp(command_args[0], "echo") == 0){
		struct builtin_command *cmd = builtin_command_get("echo");
		int retrn = cmd->function(interpreter_new(true), (const char* const*)command_args, 0, 1, 2);
		*status = retrn;
		*executed = 1;
	}
	else{
		return;
	}
}

//check if the cd command was given
void check_cd(char** command_args, int* status, int* executed){
	if(strcmp(command_args[0], "cd") == 0){
		struct builtin_command *cmd = builtin_command_get("cd");
		int retrn = cmd->function(interpreter_new(true), (const char* const*)command_args, 0, 1, 2);
		*status = retrn;
		*executed = 1;
	}
	else{
		return;
	}
}

//check if the user entered the pwd command
void check_pwd(char** command_args, int* status, int* executed){
	if(strcmp(command_args[0], "pwd") == 0){
		struct builtin_command *cmd = builtin_command_get("pwd");
		int retrn = cmd->function(interpreter_new(true), (const char* const*)command_args, 0, 1, 2);
		*status = retrn;
		*executed = 1;
	}
	else{
		return;
	}
}

//check if the user entered the alias or unalias command
void check_alias(char** command_args, struct alias_table *table, int* status, int* executed){
	if(strcmp(command_args[0], "alias") == 0 || strcmp(command_args[0], "unalias") == 0){
		int retrn = alias_builtin(command_args, table);
		*status = retrn;
		*executed = 1;
	}
	else{
		return;
	}
}


/*
 *function that takes in the command typed and creates a null terminated
   array of strings of all of the arguemnts (including the command itself)
   it also replaces any of the arguements with a $ infront of them with their correspsonding shell 
   parameters or variables. if one doesn't exist in the shell it searches the environment.
   // if one is not found in the environment, a blank string is used instead
*/
char** get_command_args(struct ast_argument_list *arglist, struct vars_table *var_table){
	int capacity = 2;
	int used = 0;
	char **arr = malloc(sizeof(char*)*capacity);
	char empty_string[1] = "";
	while(arglist != NULL){
		if(arglist->first->parts->rest != NULL){ //we have expansion with {}
			int param_size = arglist->first->parts->first->parameter->size;
			char* arg  = malloc(sizeof(char) * (param_size + 1));
			memcpy(arg, arglist->first->parts->first->parameter->data, param_size);
			arg[param_size] = '\0';
			const char* to_replace = vars_get(var_table, arg); //check if it needs to be replaced
			char* actual = (char*)to_replace;
			char* final_arg;
			if(capacity == used){
				capacity *= 2;
				arr = realloc(arr, sizeof(char*) * capacity);
			}
			if(to_replace == NULL){ //exists in the shell 
				//get size of replacement
				int replace_size;
				for(replace_size = 0; actual[replace_size] != '\0'; replace_size++){}
				int combined_size = replace_size + arglist->first->parts->rest->first->string->size;
				final_arg = malloc(sizeof(char) * (combined_size + 1));
				memcpy(final_arg, actual, replace_size);
				strcat(final_arg, arglist->first->parts->rest->first->string->data);
				final_arg[combined_size] = '\0';
				arr[used] = final_arg;
				used++;
				arglist = arglist->rest;
			}
			else if(getenv(arg) != NULL){ //exists in the environment
				char* env_param = getenv(arg);
				int replace_size;
				for(replace_size = 0; env_param[replace_size] != '\0'; replace_size++){}
				int combined_size = replace_size + arglist->first->parts->rest->first->string->size;
				final_arg = malloc(sizeof(char) * (combined_size + 1));
				memcpy(final_arg, env_param, replace_size);
				strcat(final_arg, arglist->first->parts->rest->first->string->data);
				final_arg[combined_size] = '\0';
				arr[used] = final_arg;
				used++;
				arglist = arglist->rest;
			}
			else{//doesn't exist in shell or env, so only do the part outside the brackets
				int part_size = arglist->first->parts->rest->first->string->size;
				final_arg = malloc(sizeof(char) * (part_size + 1));
				memcpy(final_arg, arglist->first->parts->rest->first->string->data, part_size);
				final_arg[part_size] = '\0';
				arr[used] = final_arg;
				used++;
				arglist = arglist->rest;
			}
		}
		else if(arglist->first->parts->first->parameter != NULL){ //shell parameter
			int param_size = arglist->first->parts->first->parameter->size;
			char* arg  = malloc(sizeof(char) * (param_size + 1));
			memcpy(arg, arglist->first->parts->first->parameter->data, param_size);
			arg[param_size] = '\0';
			if(capacity == used){
				capacity *= 2;
				arr = realloc(arr, sizeof(char*) * capacity);
			}
			//should first be replaced by shell local parameter
			//if none found, then replaced by environment variable
			//none for either, empty string
			const char* to_replace = vars_get(var_table, arg);
			if(to_replace != NULL){ //replace with empty string
				arr[used] = (char*)to_replace;
				used++;
				arglist = arglist->rest;
			}
			else if(getenv(arg) != NULL){//it exists in the environment
				char* env_arg = getenv(arg);
				int i;
				for(i = 0; env_arg[i] != '\0'; i++){}
				char* replace = malloc(sizeof(char) * (i+1));
				memcpy(replace, env_arg, i);
				replace[i] = '\0';
				arr[used] = replace;
				used++;
				arglist = arglist->rest;
			}
			else{
				arr[used] = empty_string;
				used++;
				arglist = arglist->rest;
			}

		}
		else{ //normal arguments
			int size = arglist->first->parts->first->string->size;
			char* arg  = malloc(sizeof(char) * (size + 1));
			memcpy(arg, arglist->first->parts->first->string->data, size);
			arg[size] = '\0';
			if(capacity == used){
				capacity *= 2;
				arr = realloc(arr, sizeof(char*) * capacity);
			}
			arr[used] = arg;
			used++;
			arglist = arglist->rest;
		}
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
void assign_vars(struct vars_table *var_table, struct alias_table *table, struct ast_assignment_list *aslist){
	char empty_string[] = "";
	int name_size = aslist->first->name->size;
	char* name = malloc(sizeof(char) * (name_size + 1));
	memcpy(name, aslist->first->name->data, name_size);
	name[name_size] = '\0';

	//check if var assined already exists in shell. if it does, remove it first
	if(vars_get(var_table, name) != NULL){//var already exists, so have to remove previous one first
		vars_unset(var_table, name);
	}

	int val_size;
	char* value = NULL;
	if( aslist->first->value->parts->first->string == NULL){//assignment from other parameter
		val_size = aslist->first->value->parts->first->parameter->size;
		value = malloc(sizeof(char) * (val_size + 1));
		memcpy(value, aslist->first->value->parts->first->parameter->data, val_size);
		//at this point, value holds the name of the parameter that still needs to be dereferenced
		const char* new_value = vars_get(var_table, value);
		if(new_value != NULL){ //paramter exists in shell environment
			free(value);
			int i;
			for(i = 0; new_value[i] != '\0'; i++){}
			value = malloc(sizeof(char) * (i+1));
			memcpy(value, new_value, i);
			value[i] = '\0';
		}
		else if(getenv(value) != NULL){ //var exists in the environment
			new_value = getenv(value);
			free(value);
			int i;
			for(i = 0; new_value[i] != '\0'; i++){}
			value = malloc(sizeof(char) * (i+1));
			memcpy(value, new_value, i);
			value[i] = '\0';
		}
		else{
			free(value);
			value = empty_string;
		}
	}
	else if(aslist->first->value->parts->first->parameter == NULL){//assignment like normal
		val_size = aslist->first->value->parts->first->string->size;
		value = malloc(sizeof(char) * (val_size + 1));
		memcpy(value, aslist->first->value->parts->first->string->data, val_size);
		value[val_size] = '\0';
	}
	//check if value to assign is an alias
	const char* to_replace = alias_get(table, value);
	if(to_replace != NULL){
		vars_set(var_table, name, (char*)to_replace);
	}
	else{//not an alias
		//check if name is in the environment
		if(getenv(name) != NULL){
			setenv(name, value, 1); //update environment
		}
		else{
			vars_set(var_table, name, value); //update local environment
		}
	}
}

int dispatch_command(char** command_args, struct alias_table *table, struct vars_table *var_table, 
					int *pipe_in, int *pipe_out, char *in_file, char *out_file, char *append_file){
	//first check alias table and replace any aliases if they exist
	search_aliases(table, command_args);

	//check for all of the builtin commands
	//if a command is matched, it is executed inside the respectvie function, and then returns
	int builtin_executed = 0;
	int builtin_return_status = 0;
	check_exit(command_args, &builtin_return_status, &builtin_executed);
	check_history(command_args, &builtin_return_status, &builtin_executed);
	check_echo(command_args, &builtin_return_status, &builtin_executed);
	check_cd(command_args, &builtin_return_status, &builtin_executed);
	check_pwd(command_args, &builtin_return_status, &builtin_executed);
	check_alias(command_args, table, &builtin_return_status, &builtin_executed);

	if(builtin_executed != 0){ //a builtin was exectued, can return (dont have to worry about external commands and pipes)
		return builtin_return_status;
	}

	//otherwise, use external commands
	//call fork(), the execve to execute the command in the child process
	pid_t cpid;
	int wstatus;
	cpid = fork();
	if(cpid == -1){ //error forking
		printf("Error\n");
		return 1;
	}
	else if(cpid == 0){ //code executed by the child
		if(pipe_in != NULL){ //get input from pipe
			dup2(pipe_in[0], STDIN_FILENO); //read
		}
		if(pipe_out != NULL){ //write output to pipe
			close(pipe_out[0]); //close read end of pipe
			dup2(pipe_out[1], STDOUT_FILENO);
		}

		int in, out;
		if(in_file != NULL){ //input from file
			in = open(in_file, O_RDONLY);
			if(in < 0){
				if(errno == ENOENT){
					printf("%s: no such file or directory\n", in_file);
					exit(in);
				}
				else if(errno == EACCES){
					printf("%s: Permission denied\n", in_file);
					exit(in);
				}
				else{
					printf("%s: Error\n", in_file);
					exit(in);
				}
			}
			
			dup2(in, STDIN_FILENO); //replace standard input with file input
		}
		if(out_file != NULL){//output to file
			out = open(out_file, O_WRONLY | O_CREAT | O_TRUNC, 0666);
			if(out < 0){
				if(errno == ENOENT){
					printf("%s: no such file or directory\n", out_file);
					exit(out);
				}
				else if(errno == EACCES){
					printf("%s: Permission denied\n", out_file);
					exit(out);
				}
				else{
					printf("%s: Error\n", out_file);
					exit(out);
				}
			}
			dup2(out, STDOUT_FILENO);
		}
		if(append_file != NULL){//append outout to file
			out = open(append_file, O_RDWR | O_APPEND | O_CREAT, 0666);
			if(out < 0){
				if(errno == ENOENT){
					printf("%s: no such file or directory\n", append_file);
					exit(out);
				}
				else if(errno == EACCES){
					printf("%s: Permission denied\n", append_file);
					exit(out);
				}
				else{
					printf("%s: Error\n", append_file);
					exit(out);
				}
			}
			dup2(out, STDOUT_FILENO);
		}

		int return_status = execvp(command_args[0], command_args);
		if(errno == EACCES){
			printf("%s: permission denied\n", command_args[0]);
		}
		else{
			printf("%s: no such file or directory\n", command_args[0]);
		}
		fflush(stdout);
		fflush(stdin);
		exit(return_status); //terminate child process
	}
	else{ //code executed by parent; should wait until the child is done
		if(pipe_out != NULL){ //close write end of pipe if necessary
			close(pipe_out[1]);
		}
		int dead_child = wait(&wstatus);
		if(dead_child == -1){//error has occured
			return -1;
		}
		//CHECK EXIT STATUS OF WSTATUS
		return wstatus;
	}
}

char* get_file_path(struct ast_argument *arg, struct vars_table *var_table){
	if(arg->parts->first->parameter != NULL){ //get value from parameter
		//check in shell first, then the environment
		char* param = malloc(sizeof(char) * (arg->parts->first->parameter->size + 1));
		memcpy(param, arg->parts->first->parameter->data, arg->parts->first->parameter->size);
		param[arg->parts->first->parameter->size] = '\0';
		const char* to_replace = vars_get(var_table, param); //check if it needs to be replaced
		if(to_replace != NULL){ //the aparmeter exists in the shell
			return (char*)to_replace;
		}
		else if(getenv(param) != NULL){//exists in the environment
			char* env_arg = getenv(param);
			int i;
			for(i = 0; env_arg[i] != '\0'; i++){}
			char* replace = malloc(sizeof(char) * (i+1));
			memcpy(replace, env_arg, i);
			replace[i] = '\0';
			return replace;
		}
		else{
			char* empty = malloc(sizeof(char));
			empty[0] = '\0';
			return empty;
		}
	}
	else{//get value from normal string
		char* path = malloc(sizeof(char) * (arg->parts->first->string->size + 1));
		memcpy(path, arg->parts->first->string->data, arg->parts->first->string->size);
		path[arg->parts->first->string->size] = '\0';
		return path;
	}
}
