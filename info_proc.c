#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define MAX_SIZE 255
void caracteristique(pid_t pid);

int main(int argc, char * argv[]){
	if (argc!= 2){
		perror("./exo02_2 [pid]");
		exit(EXIT_FAILURE);
	}
	int pid = atoi(argv[1]);
	caracteristique(pid);
	
	return EXIT_SUCCESS;	
}
void caracteristique(pid_t pid){
	char cmd[MAX_SIZE];
	if (snprintf(cmd, MAX_SIZE, "/proc/%d/status", pid) >= MAX_SIZE){
        perror("caracteristique");
        exit(EXIT_FAILURE);
    };
	FILE *status = fopen(cmd, "r");
	if (status == NULL){
        perror("fopen");
        exit(EXIT_FAILURE);
    }
    char *c[4] = {"Name", "State", "Tgid", "PPid"};
	char str[MAX_SIZE];
	while (fgets(str, MAX_SIZE, status) != NULL){
		for (int i = 0; i < 4; i++){
			if (strncmp(str, c[i], strlen(c[i])) == 0){
				printf("[%d] %s",pid,str);
			}
		}
	}
	fclose(status);
}
