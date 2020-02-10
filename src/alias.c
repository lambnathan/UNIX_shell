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

void alias_unset(struct alias_table *table, const char *name)
{
	TODO("Unset an entry.");
}

const char *alias_get(struct alias_table *table, const char *name)
{
	TODO("Get an entry.");
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