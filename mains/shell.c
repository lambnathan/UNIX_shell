#include "error.h"
#include <unistd.h>
#include <stdio.h>
#include <pwd.h>
#include <stdlib.h>

#include <readline/history.h>
#include <readline/readline.h>

#include "parser.h"

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
		printf("%s %s $ ", p->pw_name, good); //prints command prompt
		line = readline(NULL);
		if(!line){
			printf("ctr-d pressed\n");
			
			return 0;
		}
		char subbuff[5];
		memcpy(subbuff, &line[0], 4);
		//printf("subbuff: %s \n", subbuff);
		if(strcmp(subbuff, "exit") == 0){
			printf("you want to exit\n");
			return 0;
		}
		//struct ast_statement_list *statements = parse_input(line);
		printf("You entered: %s \n", line);
		//statements->first
		
		free(line);
	}


	//free memory before exiting
	free(p);


	return 0;
}
