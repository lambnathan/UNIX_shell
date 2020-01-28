#include "error.h"
#include <unistd.h>
#include <stdio.h>
#include <pwd.h>
#include <stdlib.h>

int main(int argc, char *argv[]){
	// TODO: WRITE A MAIN 
	//prrompt user for a command
	//should countinute to prompt until user types CTRL-D at an empty prompt, or uses the exit built-in to exit
	unsigned int uid = getuid();
	struct passwd *p = getpwuid(uid);
	//printf("The username is: %s", p->pw_name);
	for(;;){
		printf("%s :~) $ ", p->pw_name); //prints command prompt
		//char input[100];
		//fgets(input, 100, stdin);
		char* line = malloc(100);
		line = fgets(line, 100, stdin);
		if(line == NULL){
			printf("ctrl-D pressed");
			break;
		}
		//free(line);
		
	}


	//free memory before exiting
	free(p);


	return 0;
}
