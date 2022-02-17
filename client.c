#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include "file_synch.h"
#include "threads.h"
#include "kit_client.h"

int main(int argc, char const *argv[]) {
	//Vérification des arguments
	if (argc < 2) {
		fprintf(stderr, "Usage: %s cmd_name [cmd_arg]+\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	if (argc > MAX_CMD+2) {
		fprintf(stderr, "Usage: le nombre maximum d'arguments est %d\n", MAX_ARGS_NB);
		exit(EXIT_FAILURE);
	}

	/*
	 * Gestion des signaux d'interruption
	 * SIGINT, SIGQUIT, SIGTSTP, SIGHUP, SIGTERM
	 */
	if (sig_catcher_client() == -1) {
		fprintf(stderr,"__sig_catcher_client\n");
		exit(EXIT_FAILURE);
	}

	// Créer les tubes nommés nécessaires pour effectuer une demande
	commande cmd;
	if (setpnames(&cmd) == -1){
		fprintf(stderr,"__setpnames\n");
		exit(EXIT_FAILURE);
	}
	if (mkpipes(&cmd) == -1){
		fprintf(stderr,"mkpipes\n");
		exit(EXIT_FAILURE);
	}

	// Définir la taille de la commande
	cmd.cmd_size = argc - 1;
	
	// Remplir la commande
	for (int i = 0; i < cmd.cmd_size; i++) {
		if ((strlen(argv[i + 1]) + 1) > MAX_STRING_SIZE){
			fprintf(stderr,"Taille de la commande depassée\n");
			clean_client();
			exit(EXIT_FAILURE);
		}
		strncpy(cmd.cmd_txt[i], argv[i + 1], strlen(argv[i + 1]) + 1);
	}
	
	// Ouverture de l'objet de la mémoire partagée
	int shm_fd;
	if ((shm_fd = openfifo(O_EXISTED_SHM)) == -1) {
		fprintf(stderr,"__openfifo\n");
		exit(EXIT_FAILURE);
	}
    
    // Obtention de la file synchronisée
    struct file_synch *fifo;
    if ((fifo = mapfifo(shm_fd)) == NULL){
		fprintf(stderr,"__mapfifo\n");
		exit(EXIT_FAILURE);
	}

	// Envoyer la demande dans la file synchronisée
	if (enfiler(cmd, fifo) == -1){
		fprintf(stderr,"__enfiler\n");
		exit(EXIT_FAILURE);
	}
	
	/*
	 * Ouvrir le tube nommé qui pourra être utilisé
	 * en tant qu’entrée standard de la commande à exécuter
	 */
	int cmd_stdin;
	cmd_stdin = open(cmd.pipe_in, O_WRONLY);
	if (cmd_stdin == -1) {
		perror("open");
		clean_client();
		exit(EXIT_FAILURE);
    }
    
    /*
     * Supprimer le tube tout de suite pour ne pas oublier
     * Le tube ne sera réellement supprimé que quand plus
     * processus ne l'utilisera.
     */
    if (unlink(cmd.pipe_in) == -1) {
		perror("unlink");
		exit(EXIT_FAILURE);
	}
    
	/*
	 * Ecrire dans entrée standard de la commande à exécuter
	 * (Le cas d'une commande intéractive)
	 */
	pthread_t th_write;
	int errnum;
	if ((errnum = pthread_create(&th_write, NULL, write_routine, &cmd_stdin) != 0)) {
		fprintf(stderr, "pthread_create: %s\n", strerror(errnum));
		clean_client();
		exit(EXIT_FAILURE);
	}
	
	/*
	 * Ouvrir le tube nommé qui pourra être utilisé
	 * en tant que sortie d'erreurs de la commande à exécuter
	 */
	int cmd_stderr;
	cmd_stderr = open(cmd.pipe_err, O_RDONLY);
	if (cmd_stderr == -1) {
		perror("open");
		clean_client();
		exit(EXIT_FAILURE);
	}
	
	//Supprimer le tube tout de suite pour ne pas oublier
	if (unlink(cmd.pipe_err) == -1) {
		perror("unlink");
		exit(EXIT_FAILURE);
	}
    
	/*
	 * Lire la sortie d'erreurs de la commande
	 * En cas d'erreurs produits au moment de l'éxécution
	 */
	pthread_t th_err;
	if ((errnum = pthread_create(&th_err, NULL, error_routine, &cmd_stderr) != 0)) {
		fprintf(stderr, "pthread_create: %s\n", strerror(errnum));
		clean_client();
		exit(EXIT_FAILURE);
	}
	
	/*
	 * Ouvrir le tube nommé qui pourra être utilisé
	 * en tant que sortie standard de la commande à exécuter
	 */
	int cmd_stdout;
	cmd_stdout = open(cmd.pipe_out, O_RDONLY);
	
	if (cmd_stdout == -1) {
		perror("open");
		clean_client();
		exit(EXIT_FAILURE);
	}
	
	//Supprimer le tube tout de suite pour ne pas oublier
	if (unlink(cmd.pipe_out) == -1) {
		perror("unlink3");
		exit(EXIT_FAILURE);
	}

	/*
	 * Lire le résultat de de la commande à exécuter
	 * En cas de réussir
	 */
	char reponse[BUF_SIZE];
	ssize_t n;
	
	while ((n = read(cmd_stdout, reponse, sizeof(reponse))) > 0) {
		
		if (write(STDOUT_FILENO, reponse, (size_t)n) == -1) {
			perror("write");
			clean_client();
			exit(EXIT_FAILURE);
		}
	}
	
	if (n == -1) {
		perror("read");
		clean_client();
		exit(EXIT_FAILURE);
	}
	
	//Libérer les ressources occupés par le client
	clean_client();
	
    return EXIT_SUCCESS;
}
