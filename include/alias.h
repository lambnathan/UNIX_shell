#ifndef _ALIAS_H
#define _ALIAS_H

struct alias_table{
	char** name; //array of the old strings
	char ** value; //array of the new strings, with order relative to their old counterparts
	int used; //keeps track of number of aliases
	int capacity; //keeps track of available memory slots in table
};

struct alias_table *alias_table_new(void);
void alias_table_free(struct alias_table *table);
void alias_set(struct alias_table *table, const char *name,
	       const char *replacement);
void alias_unset(struct alias_table *table, const char *name);
const char *alias_get(struct alias_table *table, const char *name);

int alias_builtin(char** command_args, struct alias_table *table);
void print_all_aliases(struct alias_table *table);

#endif /* _ALIAS_H */
