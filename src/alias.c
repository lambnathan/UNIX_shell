#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "alias.h"
#include "error.h"

struct alias_table *alias_table_new(void)
{
	return checked_calloc(sizeof(struct alias_table), 1);
}

void alias_table_free(struct alias_table *table)
{
	TODO("Free any memory allocated for the table.");
}

void alias_set(struct alias_table *table, const char *name,
	       const char *replacement)
{
	int name_size;
	int replacement_size;
	for(name_size = 0; name[name_size] != '\0'; name_size++){}
	for(replacement_size = 0; replacement[replacement_size] != '\0'; replacement_size++){}
	char* new_name = malloc(sizeof(char) * (name_size + 1));
	char* new_replacement = malloc(sizeof(char) * (replacement_size + 1));
	memcpy(new_name, name, name_size);
	memcpy(new_replacement, replacement, replacement_size);
	new_name[name_size] = '\0';
	new_replacement[replacement_size] = '\0';
	if(table->capacity == table->used){
		table->capacity *= 2;
		table->name = realloc(table->name, sizeof(char*) * table->capacity);
		table->value = realloc(table->value,  sizeof(char*) * table->capacity);
	}
	table->name[table->used] = new_name;
	table->value[table->used] = new_replacement;
	table->used++;
}

//remove an alias
void alias_unset(struct alias_table *table, const char *name){
	//find index of alias to remove
	int i = 0;
	for(i = 0; strcmp(table->name[i], name) != 0; i++){}
	free(table->name[i]);
	free(table->value[i]);
	for(int j = i; j < table->used - 1; j++){//move elements to fill in the space
		table->name[j] = table->name[j+1];
		table->value[j] = table->value[j+1];
	}
	table->used--;
}

const char *alias_get(struct alias_table *table, const char *name)
{
	for(int i = 0; i < table->used; i++){
		if(strcmp(table->name[i], name) == 0){
			return table->value[i];
		}
	}
	return NULL;
}


int alias_builtin(char** command_args, struct alias_table *table){
	if(command_args[1] == NULL && strcmp(command_args[0], "alias") == 0){ //print all aliases
		print_all_aliases(table);
		return 0;
	}
	else if(strcmp(command_args[0], "alias") == 0){ //add aliases to the table
		for(int i = 1; command_args[i] != NULL; i++){
			//parse the alias
			char* alias = command_args[i];
			int j;
			for(j = 0; alias[j] != '='; j++){}
			int name_size = j + 1;
			char name[name_size];
			memcpy(name, alias, j);
			name[j] = '\0';

			if(alias_get(table, name) != NULL){//alias already exists, so have to remove previous one first
				alias_unset(table, name);
			}

			int k;
			for(k = j + 1; alias[k] != '\0'; k++){}
			int replacement_size = k-j;
			char replacement[replacement_size];
			memcpy(replacement, &alias[j+1], k-j-1);
			replacement[k-j-1] = '\0';
			alias_set(table, name, replacement);
			//free(name);
			//free(replacement);
		}
	}
	else if(strcmp(command_args[0], "unalias") == 0){
		if(command_args[1] != NULL){
			for(int i = 1; command_args[i] != NULL; i++){
				if(alias_get(table, command_args[i]) != NULL){
					alias_unset(table, command_args[i]);
				}
			}
			return 0;
		}
		else{ //not enough arguments
			printf("usage: unalias <alias...>\n");
			return 1;
		}
	}
	return 0;
}

//function to print all the current aliases
void print_all_aliases(struct alias_table *table){
	if(table->used == 0){ //no aliases; return
		return;
	}
	for(int i = 0; i < table->used; i++){
		printf("%s=%s\n", table->name[i], table->value[i]);
	}
}