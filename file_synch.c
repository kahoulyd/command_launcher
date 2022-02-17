#ifdef _XOPEN_SOURCE
#undef _XOPEN_SOURCE
#define _XOPEN_SOURCE 500
#endif

#include <errno.h>
#include <fcntl.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include "file_synch.h"

int enfiler(commande cmd, struct file_synch *file) {
	// Effectuer la synchronisation
	if (sem_wait(&file->vide) == -1) {
		perror("sem_wait");
		return -1;
	}

	if (sem_wait(&file->mutex) == -1) {
		perror("sem_wait");
		return -1;
	}

	file->buffer[file->tete] = cmd;
	file->tete = (file->tete + 1) % MAX_CMD;

	if (sem_post(&file->mutex) == -1) {
		perror("sem_post");
		return -1;
	}
	if (sem_post(&file->plein) == -1) {
		perror("sem_post");
		return -1;
	}
	return 0;
}

commande defiler(struct file_synch *file) {
	commande cmd;
	// Effectuer la synchronisation
	if (sem_wait(&file->plein) == -1) {
		perror("sem_wait");
		cmd.cmd_size = 0;
		return cmd;
	}
	
	/*
	 * Le mutex est inutile
	 * Parce que il y a un  seul lanceur
	 * Qui peut accéder à cette section  critique
	 */
	if (sem_wait(&file->mutex) == -1) {
		perror("sem_wait");
		cmd.cmd_size = 0;
		return cmd;
	}

	cmd = file->buffer[file->queue];
	file->queue = (file->queue + 1) % MAX_CMD;

	if (sem_post(&file->mutex) == -1) {
		perror("sem_post");
		cmd.cmd_size = 0;
		return cmd;
	}
	if (sem_post(&file->vide) == -1) {
		perror("sem_post");
		cmd.cmd_size = 0;
		return cmd;
	}
	return cmd;
}

int initfifo(struct file_synch *file) {
	if (sem_init(&file->mutex, 1, 1) == -1) {
		perror("sem_init");
		return -1;
	}

	if (sem_init(&file->vide, 1, MAX_CMD) == -1) {
		perror("sem_init");
		return -1;
	}

	if (sem_init(&file->plein, 1, 0) == -1) {
		perror("sem_init");
		return -1;
	}

	file->tete = 0;
	file->queue = 0;

	return 0;
}

int openfifo(int mode) {
    int shm_fd;
    switch (mode) {
		case O_NEW_SHM:
			shm_fd = shm_open(NOM_SHM, O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
			if (shm_fd == -1) {
				perror("shm_open");
				return -1;
			}

			if (ftruncate(shm_fd, TAILLE_SHM) == -1) {
				perror("ftruncate");
				return -1;
			}
			return shm_fd;
			
		case O_EXISTED_SHM:
			shm_fd = shm_open(NOM_SHM, O_RDWR, S_IRUSR | S_IWUSR);
			
			if (shm_fd == -1) {
				perror("shm_open");
				return -1;
			}
			return shm_fd;
		default:
			fprintf( stderr, "openfifo : mauvais argument %d\n", mode);
			return -1;
	}
    
}

struct file_synch *mapfifo(int shm_fd) {
	struct file_synch *fifo = NULL;
	fifo = (struct file_synch *)mmap(NULL, TAILLE_SHM, PROT_READ | PROT_WRITE,
								   MAP_SHARED, shm_fd, 0);
	if (fifo == MAP_FAILED) {
		perror("mmap");
		return NULL;
	}
	return fifo;
}

int suppfifo(int shm_fd) {
    if (shm_unlink(NOM_SHM) == -1) {
        perror("shm_unlink");
        return -1;
    }
    if (close(shm_fd) == -1) {
        perror("close");
        return -1;
    }
    return 0;
}

int detsem(struct file_synch *fifo) {
	if (sem_destroy(&fifo->mutex) == -1) {
		perror("sem_destroy");
		return -1;
	}
	if (sem_destroy(&fifo->plein) == -1) {
		perror("sem_destroy");
		return -1;
	}
	if (sem_destroy(&fifo->vide) == -1) {
		perror("sem_destroy");
		return -1;
	}
	return 0;
}
