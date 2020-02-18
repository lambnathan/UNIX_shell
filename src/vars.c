#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "vars.h"
#include "error.h"

struct vars_table *vars_table_new(void)
{
	return checked_calloc(sizeof(struct vars_table), 1);
}

void vars_set(struct vars_table *var_table, const char *name,
	       const char *val){
    int name_size;
	int replacement_size;
	for(name_size = 0; name[name_size] != '\0'; name_size++){}
	for(replacement_size = 0; val[replacement_size] != '\0'; replacement_size++){}
	char* new_name = malloc(sizeof(char) * (name_size + 1));
	char* new_replacement = malloc(sizeof(char) * (replacement_size + 1));
	memcpy(new_name, name, name_size);
	memcpy(new_replacement, val, replacement_size);
	new_name[name_size] = '\0';
	new_replacement[replacement_size] = '\0';
	if(var_table->capacity == var_table->used){
		var_table->capacity *= 2;
		var_table->name = realloc(var_table->name, sizeof(char*) * var_table->capacity);
		var_table->value = realloc(var_table->value,  sizeof(char*) * var_table->capacity);
	}
	var_table->name[var_table->used] = new_name;
	var_table->value[var_table->used] = new_replacement;
	var_table->used++;
}

//remove a variable (not a direct command, only have to do when reassigning variable)
void vars_unset(struct vars_table *var_table, const char *name){
	//find index of alias to remove
	int i = 0;
	for(i = 0; strcmp(var_table->name[i], name) != 0; i++){}
	free(var_table->name[i]);
	free(var_table->value[i]);
	for(int j = i; j < var_table->used - 1; j++){//move elements to fill in the space
		var_table->name[j] = var_table->name[j+1];
		var_table->value[j] = var_table->value[j+1];
	}
	var_table->used--;
}

const char *vars_get(struct vars_table *var_table, const char *name)
{
	for(int i = 0; i < var_table->used; i++){
		if(strcmp(var_table->name[i], name) == 0){
			return var_table->value[i];
		}
	}
	return NULL;
}

//print all the variables (for testing/debug)
void print_all_vars(struct vars_table *var_table){
    if(var_table->used == 0){ //no vars; return
		return;
	}
	for(int i = 0; i < var_table->used; i++){
		printf("%s=%s\n", var_table->name[i], var_table->value[i]);
	}
}