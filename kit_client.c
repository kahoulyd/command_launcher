#ifdef _XOPEN_SOURCE
#undef _XOPEN_SOURCE
#define _XOPEN_SOURCE 500
#endif

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <semaphore.h>
#include <sys/stat.h>
#include <unistd.h>

#include "file_synch.h"
#include "threads.h"
#include "kit_client.h"

void handler_client(int signum) {
	if (signum < 0) {
		fprintf(stderr, "Mauvais numéro de signal\n");
	}
	fprintf(stderr, "\nClient interrompu par un signal %d\n", signum);
	clean_client();
	exit(EXIT_SUCCESS);
}

int sig_catcher_client(void) {
    struct sigaction action;
    
    action.sa_handler = handler_client;
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
        perror("sigaction SIGTSTP");
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

int setpnames(commande *cmd){
	pid_t my_pid = getpid();

	if ((snprintf(cmd->pipe_in, TUBE_NAME_SIZE, "tube_in_%d", my_pid)) >=
		TUBE_NAME_SIZE) {
		fprintf(stderr, "tube_in : pas assez d'espace TUBE_NAME_SIZE (%d)\n", TUBE_NAME_SIZE);
		return -1;
	}

	if ((snprintf(cmd->pipe_out, TUBE_NAME_SIZE, "tube_out_%d", my_pid)) >=
		TUBE_NAME_SIZE) {
		fprintf(stderr, "tube_out : pas assez d'espace TUBE_NAME_SIZE (%d)\n", TUBE_NAME_SIZE);
		return -1;
	}
	if ((snprintf(cmd->pipe_err, TUBE_NAME_SIZE, "tube_err_%d", my_pid)) >=
		BUF_SIZE) {
		fprintf(stderr, "tube_err : pas assez d'espace TUBE_NAME_SIZE (%d)\n", TUBE_NAME_SIZE);
		return -1;
	}
	return 0;
}

int mkpipes(commande *cmd) {
	if (mkfifo(cmd->pipe_in, S_IRUSR | S_IWUSR) == -1) {
		perror("mkfifo");
		return -1;
	}

	if (mkfifo(cmd->pipe_out, S_IRUSR | S_IWUSR) == -1) {
		perror("mkfifo");
		return -1;
	}

	if (mkfifo(cmd->pipe_err, S_IRUSR | S_IWUSR) == -1) {
		perror("mkfifo");
		return -1;
	}
	return 0;
}

void clean_client() {
	int fdmax = getdtablesize();
	for (int i = 0; i <= fdmax; ++i) {
		close(i);
	}
	return;
}

void *write_routine(void *args) {
	
	int errnum;
	if ((errnum = pthread_detach(pthread_self())) != 0) {
		fprintf(stderr, "pthread_detach: %s\n", strerror(errnum));
		exit(EXIT_FAILURE);
	}
	int *fd = (int *)args;
	int errno;
	ssize_t n;
	char in[BUF_SIZE];

	while ((n = read(STDIN_FILENO, in, BUF_SIZE)) > 0) {
		if (write(*fd, in, (size_t)n) == -1) {
			perror_r("write", errno);
			exit(EXIT_FAILURE);
		}
	}

	if (n == -1) {
		perror_r("read", errno);
		exit(EXIT_FAILURE);
	}
	if (close(*fd) == -1) {
		perror_r("close", errno);
		exit(EXIT_FAILURE);
	}
	pthread_exit(NULL);
}

void *error_routine(void *args) {
	int errnum;
	if ((errnum = pthread_detach(pthread_self())) != 0) {
		fprintf(stderr, "pthread_detach: %s\n", strerror(errnum));
		exit(EXIT_FAILURE);
	}
	
	int errno;
	int *fd = (int *)args;
	ssize_t n;
	char in[BUF_SIZE];

	while ((n = read(*fd, in, BUF_SIZE)) > 0) {
		if (write(STDERR_FILENO, in, (size_t)n) == -1) {
			perror_r("write", errno);
			exit(EXIT_FAILURE);
		}
	}

	if (n == -1) {
		perror_r("read", errno);
		exit(EXIT_FAILURE);
	}
	if (close(*fd) == -1) {
		perror_r("close", errno);
		exit(EXIT_FAILURE);
	}
	pthread_exit(NULL);
}


