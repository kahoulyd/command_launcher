#ifndef FILE_SYNCH
#define FILE_SYNCH

#define MAX_ARGS_NB 7
#define MAX_STRING_SIZE 255
#define TUBE_NAME_SIZE 255
#define BUF_SIZE 255
#define MAX_CMD 10
#define TAILLE_SHM (sizeof(struct file_synch))
#define NOM_SHM "/file_synchronisee_pse1819"
#define O_NEW_SHM 0
#define O_EXISTED_SHM 1


/*
 * Définition d'un nouveau type commande
 * Il s'agit de la demande envoyée dans la file synchronisée
 */
typedef struct {
	char pipe_in[TUBE_NAME_SIZE];
	char pipe_out[TUBE_NAME_SIZE];
	char pipe_err[TUBE_NAME_SIZE];
	int cmd_size;	//Nombre des options de la commande envoyée (nom de cmd inclus)									
	char cmd_txt[MAX_ARGS_NB][MAX_STRING_SIZE];
} commande;

/* 
 * Définition de la file synchronisée
 */
struct file_synch {
	sem_t mutex;
	sem_t vide;
	sem_t plein;
	int tete;                	// Position d'ajout dans le tampon
	int queue;               	// Position de suppression dans le tampon
	commande buffer[MAX_CMD]; 	// Le tampon contenant les données
};

/*
 * Ajouter une nouvelle demande dans la file par le client
 * Ou attendre si la file est pleine
 * Cette fonction retourne 0 en cas de succès
 * Ou -1 en cas d'echec
 */
int enfiler(commande cmd, struct file_synch *file);

/* 
 * Retirer une demande de la file par le lanceur
 * Ou attendre si la file est vide
 * Cette fonction retourne une demande vide en  cas d'echec
 */
commande defiler(struct file_synch *file);

/*
 * Définir les valeurs par default de la file synchronisée
 * Cette fonction retourne 0 en cas de succès
 * Ou -1 en cas d'echec
 */ 
int initfifo(struct file_synch *file);

/*
 * Créer ou ouvrir un objet de mémoire partagée
 * Selon le int mode
 * O_NEW_SHM : pour Créer un nouveau objet mémoire partagée
 * (cas lanceur)
 * O_EXISTED_SHM : pour ouvrir un objet mémoire partagée déjà existant
 * (cas client)
 * Cette fonction retourne un un descripteur de fichier associé à l’objet en cas de succès
 * Ou -1 en cas d'echec
 */ 
int openfifo(int mode);

/*
 * Établir une projection en mémoire
 * Cette fonction  renvoie un pointeur sur la file synchronisée en cas de succès
 * Ou NULL en cas d'echec
 */
struct file_synch *mapfifo(int shm_fd);

/*
 * Supprimer la mémoire partagée
 * Cette fonction retourne 0 en cas de succès
 * Ou -1 en cas d'echec
 */
int suppfifo(int shm_fd);

/*
 * Detruire les sémaphores utilisés dans la file synchronisée
 * Cette fonction retourne 0 en cas de succès
 * Ou -1 en cas d'echec
 */
int detsem(struct file_synch *fifo);

#endif
