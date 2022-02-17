#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>

#include "log.h"
#include "file_synch.h"
#include "threads.h"
#include "kit_lanceur.h"

void handler_lanceur(int signum) {
	if (signum < 0) {
		fprintf(stderr, "Mauvais numéro de signal\n");
		exit(EXIT_FAILURE);
	}
	fprintf(stderr, "Lanceur interrompu par un signal %d\n", signum);
	quitter = true;
}

int sig_catcher_lanceur(void) {
	struct sigaction action;
	action.sa_handler = handler_lanceur;
	action.sa_flags = 0;
	if (sigfillset(&action.sa_mask) == -1) {
		perror("sigfilltset");
		return -1;
	}
	if (sigaction(SIGINT, &action, NULL) == -1) {
		perror("sigaction SIGINT");
		return -1;
	}
	if (sigaction(SIGQUIT, &action, NULL) == -1) {
		perror("sigaction SIGQUIT");
		return -1;
	}
	if (sigaction(SIGTSTP, &action, NULL) == -1) {
		perror("sigaction SIGSTP");
		return -1;
	}
	if (sigaction(SIGHUP, &action, NULL) == -1) {
		perror("sigaction SIGHUP");
		return -1;
	}
	if (sigaction(SIGTERM, &action, NULL) == -1) {
		perror("sigaction SIGTERM");
		return -1;
	}

	return 0;
}

int gestionnaire_zombis(){
	struct sigaction action;
	int errno;
    action.sa_handler = SIG_DFL;
    action.sa_flags = SA_NOCLDWAIT;
    if (sigfillset(&action.sa_mask) == -1) {
        perror_r("sigfilltset", errno);
        return -1;
    }
    if (sigaction(SIGCHLD, &action, NULL) == -1) {
        perror_r("sigfilltset", errno);
        return -1;
    }
    return 0;
}
int redirect(char *pipeName, int flags, int newfd) {
	int fd = open(pipeName, flags);
	if (fd == -1) {
		perror_r("open", errno);
		return -1;
	}
	if (dup2(fd, newfd) == -1) {
		perror_r("dup2", errno);
		return -1;
	}
	if (close(fd) == -1) {
		perror_r("close", errno);
		return -1;
	}
	return 0;
}

void *exec_routine(void *args) {
    int errno;
    
    commande cmd = *((commande *)args);
    
    char *cmdline[cmd.cmd_size + 1];
    
    //Lire les args de la commande reçue
    for (int i = 0; i < cmd.cmd_size; i++) {
        cmdline[i] = cmd.cmd_txt[i];
    }
    
    cmdline[cmd.cmd_size] = NULL;
	
	//Se dupliquer pour effectuer un appel à exec
    switch (fork()) {
		case -1:
			perror_r("fork", errno);
			pthread_exit(NULL);
			break;
		case 0:
			/*
			 * Rediriger les entrées/sorties du processus fils
			 * Vers le client
			 */
			if (redirect(cmd.pipe_in, O_RDONLY, STDIN_FILENO) == -1){
				fprintf(stderr,"__redirect pipe_in\n");
				exit(EXIT_FAILURE);
			}
			if (redirect(cmd.pipe_err, O_WRONLY, STDERR_FILENO) ==-1){
				fprintf(stderr,"__redirect pipe_err\n");
				exit(EXIT_FAILURE);
			}
			if (redirect(cmd.pipe_out, O_WRONLY, STDOUT_FILENO) ==-1){
				fprintf(stderr,"__redirect pipe_out\n");
				exit(EXIT_FAILURE);
			}
			
			execvp(cmdline[0], cmdline);
			perror("execvp");
			
			/*
			 * Ecrire dans le fichier log
			 * En  cas d'échec de traitement de la commande
			 */
			int fdlog;
			fdlog = openlog();
			
			if (fdlog == -1){
				exit(EXIT_FAILURE);
			}
			
			//Rediriger la sortie d'erreur standard vers le fichier log
			redirectlog(fdlog);
			if (redirectlog(fdlog)){
				exit(EXIT_FAILURE);
			}
			
			//Rédiger le message d'erreur
			if (print_time_log(fdlog) == -1){
				fprintf(stderr,"__print_time_log\n");
				exit(EXIT_FAILURE);
			}
			if (print_log(fdlog, "Echec de traitement de la commande ") == -1){
				fprintf(stderr,"__print_log\n");
			}
			for (int i = 0; i < cmd.cmd_size; i++) {
				if (print_log(fdlog, cmdline[i]) == -1){
					fprintf(stderr,"__print_log\n");
					break;
				}
				if (print_log(fdlog, " ") == -1){
					fprintf(stderr,"__print_log\n");
					break;
				}
			}
			perror("execvp");
			
			//Fermer le fichier log
			if (closelog(fdlog) == -1){
				fprintf(stderr,"__closelog\n");
			}
			exit(EXIT_FAILURE);
			break;
		default:
			break;
    }
	
	//Terminer le thread appelant 
    pthread_exit(NULL);
}



