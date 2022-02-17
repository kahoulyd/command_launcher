#ifndef KIT_CLIENT_H
#define KIT_CLIENT_H

static volatile sig_atomic_t quitter; // pour terminer le lanceur proprement

/*
 * Traitement des signaux de terminison
 * Reçus par le lanceur
 * Cette fonction retourne 0 en cas de succès
 * Ou -1 en cas d'echec
 */
int sig_catcher_lanceur(void);

/*
 * le gestionnaire de signal de terminison capturé par le lanceur
 */
void handler_lanceur(int signum);

/*
 * Un chasseur de zombis
 * Cette fonction retourne 0 en cas de succès
 * Ou -1 en cas d'echec
 */
int gestionnaire_zombis();

/*
 * La routine à exécuter par chaque thread
 * Pour le traitement de la demande reçue 
 */
void *exec_routine(void *args);

/*
 * Ouvrir le tube pipeName avec flags et le rediriger vers newfd
 * Utulisée pour rediriger les entrées/sorties du processus fils effectuant exec
 * Cette fonction retourne 0 en cas de succès
 * Ou -1 en cas d'echec
 */
int redirect(char *pipeName, int flags, int newfd);



#endif

