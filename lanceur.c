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

#include "log.h"
#include "file_synch.h"
#include "threads.h"
#include "kit_lanceur.h"

int main(void) {
	
	/*
	 * Gestion de fichier log
	 */
	int fdlog;
    fdlog = openlog();
    if (fdlog == -1){
		exit(EXIT_FAILURE);
	}
	
	if (redirectlog(fdlog)){
		exit(EXIT_FAILURE);
	}
	
	if (print_time_log(fdlog) == -1){
		fprintf(stderr,"__print_time_log\n");
		exit(EXIT_FAILURE);
	}
	if (print_log(fdlog, "Démarrage du lanceur de commande\n") == -1){
		fprintf(stderr,"__print_log\n");
		exit(EXIT_FAILURE);
	}
	
	/*
	 * Gestion des signaux d'interruption
	 * SIGINT, SIGQUIT, SIGTSTP, SIGHUP, SIGTERM
	 */
	if (sig_catcher_lanceur()){
		fprintf(stderr,"__sig_catcher_lanceur\n");
		exit(EXIT_FAILURE);
	}
	
	/*
	 * Gérer les zombis à travers un signal SIGCHILD
	 */
	if (gestionnaire_zombis() == -1){
		fprintf(stderr,"__gestionnaire_zombis\n");
		exit(EXIT_FAILURE);
	}
	
    // Créer l'objet de la mémoire partagée
	int shm_fd;
	if ((shm_fd = openfifo(O_NEW_SHM)) == -1) {
		fprintf(stderr,"__openfifo\n");
		exit(EXIT_FAILURE);
	}
	
    // Obtention de la file synchronisée
    struct file_synch *fifo;
    if ((fifo = mapfifo(shm_fd)) == NULL){
		fprintf(stderr,"__mapfifo\n");
		exit(EXIT_FAILURE);
	}
	
	//Initialiser la file synchronisée
	if (initfifo(fifo) == -1){
		fprintf(stderr,"__initfifo\n");
		exit(EXIT_FAILURE);
	}
    
	quitter = false;
    
	while (quitter == false) {
		
		/*
		 * Recevoir s'il existe une commande à exécuter
		 * depuis la file synchronisée
		 */
		commande cmd;
		cmd = defiler(fifo);
		
		//Vérifier si la commande à lancer est bien reçue
		if (cmd.cmd_size == 0) {
			fprintf(stderr,"__defiler\n");
			break;
		}
		
		//Enregistrer la commande sur le fichier log
		if (print_cmd_log(fdlog, cmd) == -1){
			fprintf(stderr,"__print_cmd_log\n");
		}
		
		/*
		 * Créer un thread détaché
		 * Chaque demande d’exécution reçue
		 * Devra être traitée par un thread dédié
		 */
		int errnum;
		pthread_attr_t attr;
		if ((errnum = pthread_attr_init(&attr)) != 0) {
			fprintf(stderr, "pthread_attr_init: %s\n", strerror(errnum));
			exit(EXIT_FAILURE);
		}
		if ((errnum = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED)) != 0) {
			fprintf(stderr, "pthread_attr_setdetachstate: %s\n", strerror(errnum));
			exit(EXIT_FAILURE);
		}
		
		//Lancer le traitement de la demande avec le thread détaché
		pthread_t th_exec;
		if ((errnum = pthread_create(&th_exec, &attr, exec_routine, &cmd) != 0)) {
			fprintf(stderr, "pthread_create: %s\n", strerror(errnum));
			exit(EXIT_FAILURE);
		}
	}
	
	//Détruire les sémaphores qui gèrent la file synchronisée
	if (detsem(fifo) == -1){
		fprintf(stderr,"__detsem\n");
	}
	
	//Supprimer la file synchronisée
	if (suppfifo(shm_fd) == -1){
		fprintf(stderr,"__suppfifo\n");
	}
	
    if (print_time_log(fdlog) == -1){
		fprintf(stderr,"__print_time_log\n");
		exit(EXIT_FAILURE);
	}
	if (print_log(fdlog, "Fermeture du lanceur\n") == -1){
		fprintf(stderr,"__print_log\n");
		exit(EXIT_FAILURE);
	}
    //Fermer le fichier log
	if (closelog(fdlog) == -1){
		fprintf(stderr,"__closelog\n");
	}
	
	//Sortir du thread principal
    pthread_exit(NULL);
}
