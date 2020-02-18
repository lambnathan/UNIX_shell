#ifndef _VARS_H
#define _VARS_H

struct vars_table {
    char** name;
    char** value;
    int used;
    int capacity;
};

struct vars_table *vars_table_new(void);
void vars_set(struct vars_table *var_table, const char *name,
	       const char *val);

const char *vars_get(struct vars_table *var_table, const char *name);

void vars_unset(struct vars_table *var_table, const char *name);

//print all the variables (for testing/debug)
void print_all_vars(struct vars_table *var_table);

#endif