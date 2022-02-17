#ifndef KIT_CLIENT_H
#define KIT_CLIENT_H

/*
 * Traitement des signaux de terminison
 * Reçus par le client
 * Cette fonction retourne 0 en cas de succès
 * Ou -1 en cas d'echec
 */
int sig_catcher_client(void);

// le gestionnaire de signal capturé par le client
void handler_client(int signum);

/*
 * Définir des noms des tubes uniques pour chaque demande
 * Cette fonction retourne 0 en cas de succès
 * Ou -1 en cas d'echec
 */
int setpnames(commande *cmd);

/*
 * Créer les tubes utilisés par une demande
 * Cette fonction retourne 0 en cas de succès
 * Ou -1 en cas d'echec
 */
int mkpipes(commande *cmd);

/*
 * Libérer les ressources utilisées par le client
 */
void clean_client();

/*
 * La fonction à exécuter par le thread
 * qui va ecrire dans entrée standard de la commande
 * Dans le cas d'une commande intéractive
 */
void *write_routine(void *args);

/*
 * La fonction à exécuter par le thread
 * qui va lire la sortie d'erreur de la commande
 * Dans le cas d'erreurs produits
 */
void *error_routine(void *args);

#endif
