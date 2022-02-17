#ifndef THREADS_H
#define THREADS_H

#define ERR_MSG_SIZE 255

/*
 * Une version réentrante (thread safe) de perror
 */
void perror_r(const char *s, int errnum);

#endif
