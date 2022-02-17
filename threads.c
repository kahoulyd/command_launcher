#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include "threads.h"

void perror_r(const char *s, int errnum) {
    char msg[ERR_MSG_SIZE];
    /*
     * Version réentrante pour
     * Obtenir le libellé d'un numéro d'erreur  
     */
	if (strerror_r(errnum, msg, ERR_MSG_SIZE) == -1) {
		perror("strerror_r()");
		return;
	}
	fprintf(stderr, "%s: %s\n", s, msg);
}
